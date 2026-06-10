#ifndef BITCOIN_PUZZLE_SCANNER_ECC_H
#define BITCOIN_PUZZLE_SCANNER_ECC_H

#include "types.h"
#include "secp256k1/SECP256k1.h"
#include "secp256k1/Point.h"
#include "secp256k1/Int.h"
#include <memory>

namespace ECC {

    class Context {
    public:
        Context() {
            ctx_ = new Secp256K1();
            ctx_->Init();
        }
        ~Context() {
            delete ctx_;
        }

        // Prevent copying
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        Secp256K1* get() const { return ctx_; }

    private:
        Secp256K1* ctx_;
    };

    // Generate a compressed public key from a private key
    bool generate_public_key(const Context& ctx, const Types::PrivateKey& priv_key, Types::PublicKeyCompressed& pub_key);

    class PointWrapper {
    public:
        PointWrapper(const Context& ctx);
        ~PointWrapper() = default;

        bool init_from_private_key(const Types::PrivateKey& priv_key);
        bool add_generator();
        bool serialize_compressed(Types::PublicKeyCompressed& pub_key) const;

    private:
        const Context& ctx_;
        Point pubkey_;
    };
    
    // Alias to match old API
    using Point = PointWrapper;

}

#endif // BITCOIN_PUZZLE_SCANNER_ECC_H
