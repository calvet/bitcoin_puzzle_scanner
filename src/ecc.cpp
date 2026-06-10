#include "ecc.h"
#include <stdexcept>
#include <vector>

namespace ECC {

    Context::Context() {
        ctx_ = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        if (!ctx_) {
            throw std::runtime_error("Failed to create secp256k1 context");
        }
    }

    Context::~Context() {
        secp256k1_context_destroy(ctx_);
    }

    bool generate_public_key(const Context& ctx, const Types::PrivateKey& priv_key, Types::PublicKeyCompressed& pub_key) {
        secp256k1_pubkey c_pubkey;
        if (!secp256k1_ec_pubkey_create(ctx.get(), &c_pubkey, priv_key.data())) {
            return false;
        }

        size_t outputlen = Types::PublicKeyCompressed{}.size();
        int flags = SECP256K1_EC_COMPRESSED;
        return secp256k1_ec_pubkey_serialize(ctx.get(), pub_key.data(), &outputlen, &c_pubkey, flags);
    }

    Point::Point(const Context& ctx) : ctx_(ctx), generator_initialized_(false) {
        // Initialize pubkey_ to an invalid state or zero
        memset(&pubkey_, 0, sizeof(pubkey_));
        init_generator();
    }

    void Point::init_generator() {
        if (generator_initialized_) return;

        // The generator point G is represented by a specific private key (1)
        // We create a public key from this private key to get G
        Types::PrivateKey priv_key_one;
        priv_key_one.fill(0);
        priv_key_one[31] = 1; // Private key 1

        if (!secp256k1_ec_pubkey_create(ctx_.get(), &generator_, priv_key_one.data())) {
            throw std::runtime_error("Failed to create generator public key");
        }
        generator_initialized_ = true;
    }

    bool Point::init_from_private_key(const Types::PrivateKey& priv_key) {
        return secp256k1_ec_pubkey_create(ctx_.get(), &pubkey_, priv_key.data());
    }

    bool Point::add_generator() {
        // Add the generator point to the current public key
        // secp256k1_ec_pubkey_tweak_add adds a scalar to the private key corresponding to a public key.
        // We need to add a point (G) to a point (P).
        // The secp256k1 library provides secp256k1_ec_pubkey_combine for adding multiple public keys.
        // However, for P + G, we can use secp256k1_ec_pubkey_tweak_add if we consider G as P + (1*G)
        // A more direct way for P + G is to use secp256k1_ec_pubkey_combine with P and G.

        // For incremental point walking (P = P + G), we can use secp256k1_ec_pubkey_combine.
        // This function takes an array of public keys and combines them.
        // We will combine the current pubkey_ with the generator_.

        const secp256k1_pubkey* pubkeys[2] = {&pubkey_, &generator_};
        secp256k1_pubkey result_pubkey;

        if (!secp256k1_ec_pubkey_combine(ctx_.get(), &result_pubkey, pubkeys, 2)) {
            return false;
        }
        pubkey_ = result_pubkey;
        return true;
    }

    bool Point::serialize_compressed(Types::PublicKeyCompressed& pub_key) const {
        size_t outputlen = Types::PublicKeyCompressed{}.size();
        int flags = SECP256K1_EC_COMPRESSED;
        return secp256k1_ec_pubkey_serialize(ctx_.get(), pub_key.data(), &outputlen, &pubkey_, flags);
    }

}
