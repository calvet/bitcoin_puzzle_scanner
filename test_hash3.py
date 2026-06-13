import base58
addr = "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH"
decoded = base58.b58decode_check(addr)
print("Base58 decoded:", decoded.hex())
