import hashlib
from ecdsa import SECP256k1, SigningKey

def hash160(pubkey_bytes):
    sha256 = hashlib.sha256(pubkey_bytes).digest()
    ripemd160 = hashlib.new('ripemd160')
    ripemd160.update(sha256)
    return ripemd160.hexdigest()

for i in range(2, 4):
    sk = SigningKey.from_secret_exponent(i, curve=SECP256k1)
    compressed = (b'\x02' if sk.verifying_key.pubkey.point.y() % 2 == 0 else b'\x03') + sk.verifying_key.to_string()[:32]
    print(f"Priv: {i}, Com: {hash160(compressed)}")
