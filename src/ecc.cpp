#include "ecc.h"
#include <stdexcept>
#include <vector>
#include <cstring>

namespace ECC {

    bool generate_public_key(const Context& ctx, const Types::PrivateKey& priv_key, Types::PublicKeyCompressed& pub_key) {
        Int pk;
        pk.Set32Bytes(const_cast<unsigned char*>(priv_key.data()));
        Point p = ctx.get()->ComputePublicKey(&pk);
        ctx.get()->GetPublicKeyHex(true, p, reinterpret_cast<char*>(pub_key.data()));
        // Wait, GetPublicKeyHex(true, p, dst) writes 66 chars of hex!
        // But Types::PublicKeyCompressed is 33 BYTES, not 66 chars of hex string!
        // I need to use GetPublicKeyRaw instead.
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
        Point p = pubkey_;
        ctx_.get()->GetPublicKeyRaw(true, p, reinterpret_cast<char*>(pub_key.data()));
        return true;
    }

}
