#ifndef BITCOIN_PUZZLE_SCANNER_ECC_H
#define BITCOIN_PUZZLE_SCANNER_ECC_H

#include "types.h"
#include <secp256k1.h>
#include <memory>

namespace ECC {

    class Context {
    public:
        Context();
        ~Context();

        // Prevent copying
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        secp256k1_context* get() const { return ctx_; }

    private:
        secp256k1_context* ctx_;
    };

    // Generate a compressed public key from a private key
    bool generate_public_key(const Context& ctx, const Types::PrivateKey& priv_key, Types::PublicKeyCompressed& pub_key);

    // Incremental point addition: P' = P + G
    // This requires manipulating the internal representation or using secp256k1_ec_pubkey_t
    // We will use secp256k1_ec_pubkey_t for the intermediate state to allow addition
    class Point {
    public:
        Point(const Context& ctx);
        ~Point() = default;

        // Initialize from a private key (computes P = k*G)
        bool init_from_private_key(const Types::PrivateKey& priv_key);

        // Add the generator point G to this point (P = P + G)
        bool add_generator();

        // Serialize to compressed format
        bool serialize_compressed(Types::PublicKeyCompressed& pub_key) const;

    private:
        const Context& ctx_;
        secp256k1_pubkey pubkey_;
        secp256k1_pubkey generator_;
        bool generator_initialized_;

        void init_generator();
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_ECC_H
