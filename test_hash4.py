import hashlib
from ecdsa import SECP256k1, SigningKey

def hash160(pubkey_bytes):
    sha256 = hashlib.sha256(pubkey_bytes).digest()
    ripemd160 = hashlib.new('ripemd160')
    ripemd160.update(sha256)
    return ripemd160.hexdigest()

for i in range(4, 8):
    sk = SigningKey.from_secret_exponent(i, curve=SECP256k1)
    uncompressed = b'\x04' + sk.verifying_key.to_string()
    compressed = (b'\x02' if sk.verifying_key.pubkey.point.y() % 2 == 0 else b'\x03') + sk.verifying_key.to_string()[:32]
    print(f"Priv: {i}")
    print("Unc:", hash160(uncompressed))
    print("Com:", hash160(compressed))

print("\nPuzzle 3 target: 5dedfbf9ea599dd4e3ca6a80b333c472fd0b3f69")
