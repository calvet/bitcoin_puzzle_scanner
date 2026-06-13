import hashlib
from ecdsa import SECP256k1, SigningKey

def hash160(pubkey_bytes):
    sha256 = hashlib.sha256(pubkey_bytes).digest()
    ripemd160 = hashlib.new('ripemd160')
    ripemd160.update(sha256)
    return ripemd160.hexdigest()

privkey_hex = '0' * 62 + '71' # Wait, puzzle 7 key is unknown? We just need to check what kind of addresses they are.
# Wait, I don't know the exact key of Puzzle 7. 
# But I know that puzzles 1-64 use uncompressed!
