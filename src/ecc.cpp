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

    bool batch_generate_255(const PointWrapper& base, const std::vector<::Point>& g_table, std::vector<::Point>& out_points, const Context& ctx) {
        const ::Point& P = base.get_raw();
        out_points[0] = P;
        
        Int dx[256];
        Int dy[256];
        for (int i = 1; i < 256; ++i) {
            dy[i].ModSub(const_cast<Int*>(&g_table[i].y), const_cast<Int*>(&P.y));
            dx[i].ModSub(const_cast<Int*>(&g_table[i].x), const_cast<Int*>(&P.x));
        }

        Int prod[256];
        prod[1].Set(&dx[1]);
        for (int i = 2; i < 256; ++i) {
            prod[i].ModMulK1(&prod[i-1], &dx[i]);
        }

        Int inv;
        inv.Set(&prod[255]);
        inv.ModInv(); // Only 1 inversion!

        Int inv_dx[256];
        for (int i = 255; i > 1; --i) {
            inv_dx[i].ModMulK1(&inv, &prod[i-1]);
            Int tmp;
            tmp.ModMulK1(&inv, &dx[i]);
            inv.Set(&tmp);
        }
        inv_dx[1].Set(&inv);

        for (int i = 1; i < 256; ++i) {
            Int s;
            s.ModMulK1(&dy[i], &inv_dx[i]); // s = dy * (1/dx)

            Int s2;
            s2.ModSquareK1(&s);

            ::Point r;
            r.z.SetInt32(1);

            r.x.ModSub(&s2, const_cast<Int*>(&P.x));
            r.x.ModSub(const_cast<Int*>(&g_table[i].x));

            r.y.ModSub(const_cast<Int*>(&g_table[i].x), &r.x);
            r.y.ModMulK1(&s);
            r.y.ModSub(const_cast<Int*>(&g_table[i].y));

            out_points[i].Set(r);
        }

        return true;
    }

}
