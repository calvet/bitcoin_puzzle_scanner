import ecdsa
import hashlib
import base58

def pubkey_to_address(pubkey_bytes):
    sha256 = hashlib.sha256(pubkey_bytes).digest()
    ripemd160 = hashlib.new('ripemd160')
    ripemd160.update(sha256)
    hash160 = ripemd160.digest()
    return hash160.hex()

def get_pubkey(privkey_int):
    sk = ecdsa.SigningKey.from_secret_exponent(privkey_int, curve=ecdsa.SECP256k1)
    vk = sk.get_verifying_key()
    return vk.to_string("compressed").hex()

print("PubKey 1:", get_pubkey(1))
print("PubKey 2:", get_pubkey(2))
print("Hash160 of empty string:", pubkey_to_address(b''))
print("Hash160 of 'hello world':", pubkey_to_address(b'hello world'))
