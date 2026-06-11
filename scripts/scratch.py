import ecdsa
sk = ecdsa.SigningKey.from_secret_exponent(0x4c, curve=ecdsa.SECP256k1)
print("4c pubkey:", sk.get_verifying_key().to_string("compressed").hex())
sk = ecdsa.SigningKey.from_secret_exponent(7, curve=ecdsa.SECP256k1)
print("7 pubkey:", sk.get_verifying_key().to_string("compressed").hex())
