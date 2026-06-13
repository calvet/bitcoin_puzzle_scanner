import hashlib
import binascii
from ecdsa import SECP256k1, SigningKey

def hash160(pubkey_bytes):
    sha256 = hashlib.sha256(pubkey_bytes).digest()
    ripemd160 = hashlib.new('ripemd160')
    ripemd160.update(sha256)
    return ripemd160.hexdigest()

sk = SigningKey.from_secret_exponent(1, curve=SECP256k1)
uncompressed = b'\x04' + sk.verifying_key.to_string()
compressed = (b'\x02' if sk.verifying_key.pubkey.point.y() % 2 == 0 else b'\x03') + sk.verifying_key.to_string()[:32]

print("Priv: 1")
print("Uncompressed hash:", hash160(uncompressed))
print("Compressed hash:", hash160(compressed))
