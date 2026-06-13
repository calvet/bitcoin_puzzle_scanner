#include "ecc.h"
#include <stdexcept>
#include <vector>
#include <cstring>

namespace ECC {

    bool generate_public_key(const Context& ctx, const Types::PrivateKey& priv_key, Types::PublicKeyCompressed& pub_key) {
        Int pk;
        pk.Set32Bytes(const_cast<unsigned char*>(priv_key.data()));
        ::Point p = ctx.get()->ComputePublicKey(&pk);
        ctx.get()->GetPublicKeyRaw(true, p, reinterpret_cast<char*>(pub_key.data()));
        return true;
    }

    PointWrapper::PointWrapper(const Context& ctx) : ctx_(ctx) {
        pubkey_.Clear();
    }

    bool PointWrapper::init_from_private_key(const Types::PrivateKey& priv_key) {
        Int pk;
        pk.Set32Bytes(const_cast<unsigned char*>(priv_key.data()));
        pubkey_ = ctx_.get()->ComputePublicKey(&pk);
        return true;
    }

    bool PointWrapper::add_generator() {
        // NextKey does AddDirect(key, G)
        pubkey_ = ctx_.get()->NextKey(pubkey_);
        return true;
    }

    bool PointWrapper::serialize_compressed(Types::PublicKeyCompressed& pub_key) const {
        ::Point p = pubkey_;
        ctx_.get()->GetPublicKeyRaw(true, p, reinterpret_cast<char*>(pub_key.data()));
        return true;
    }

    bool batch_add_4G(PointWrapper& p0, PointWrapper& p1, PointWrapper& p2, PointWrapper& p3, const Context& ctx) {
        // Compute 4G
        ::Point G2 = ctx.get()->DoubleDirect(ctx.get()->G);
        ::Point G4 = ctx.get()->DoubleDirect(G2);

        ::Point* pts[4] = { &p0.get_raw(), &p1.get_raw(), &p2.get_raw(), &p3.get_raw() };
        Int dx[4], dy[4];

        for (int i = 0; i < 4; ++i) {
            dy[i].ModSub(&G4.y, &pts[i]->y);
            dx[i].ModSub(&G4.x, &pts[i]->x);
        }

        // Montgomery Batch Inversion of dx[0..3]
        Int prod[4];
        prod[0].Set(&dx[0]);
        prod[1].ModMulK1(&prod[0], &dx[1]);
        prod[2].ModMulK1(&prod[1], &dx[2]);
        prod[3].ModMulK1(&prod[2], &dx[3]);

        Int inv;
        inv.Set(&prod[3]);
        inv.ModInv(); // Only 1 inversion!

        Int inv_dx[4];
        // inv_dx[3] = inv * prod[2]
        inv_dx[3].ModMulK1(&inv, &prod[2]);
        
        // inv = inv * dx[3]
        Int tmp;
        tmp.ModMulK1(&inv, &dx[3]);
        inv.Set(&tmp);

        // inv_dx[2] = inv * prod[1]
        inv_dx[2].ModMulK1(&inv, &prod[1]);
        
        // inv = inv * dx[2]
        tmp.ModMulK1(&inv, &dx[2]);
        inv.Set(&tmp);

        // inv_dx[1] = inv * prod[0]
        inv_dx[1].ModMulK1(&inv, &prod[0]);
        
        // inv_dx[0] = inv = 1 / dx[0]
        inv_dx[0].Set(&inv);

        // Now compute the new points
        for (int i = 0; i < 4; ++i) {
            Int s;
            s.ModMulK1(&dy[i], &inv_dx[i]); // s = dy * (1/dx)

            Int s2;
            s2.ModSquareK1(&s);

            ::Point r;
            r.z.SetInt32(1);

            r.x.ModSub(&s2, &pts[i]->x);
            r.x.ModSub(&G4.x);

            r.y.ModSub(&G4.x, &r.x);
            r.y.ModMulK1(&s);
            r.y.ModSub(&G4.y);

            pts[i]->Set(r); // Copy r back to pts[i]
        }

        return true;
    }

}
