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

        ::Point& get_raw() { return pubkey_; }
        const ::Point& get_raw() const { return pubkey_; }

    private:
        const Context& ctx_;
        ::Point pubkey_;
    };
    
    // Alias to match old API
    using Point = PointWrapper;

    // Batch add 4G to 4 points simultaneously
    bool batch_add_4G(PointWrapper& p0, PointWrapper& p1, PointWrapper& p2, PointWrapper& p3, const Context& ctx);

}

#endif // BITCOIN_PUZZLE_SCANNER_ECC_H
