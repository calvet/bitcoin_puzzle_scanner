def modinv(a, m=101): # use a small prime for testing
    return pow(a, m - 2, m)

dx = [0]*10
for i in range(1, 10):
    dx[i] = i # dx[i] = i for testing

# simulate zero_idx = 4
dx[4] = 0
has_zero = True
zero_idx = 4
dx[4] = 1 # prevent zero

prod = [0]*10
prod[1] = dx[1]
for i in range(2, 10):
    prod[i] = (prod[i-1] * dx[i]) % 101

inv = modinv(prod[9])
inv_dx = [0]*10
for i in range(9, 1, -1):
    inv_dx[i] = (inv * prod[i-1]) % 101
    inv = (inv * dx[i]) % 101
inv_dx[1] = inv

print("Expected:")
for i in range(1, 10):
    if i != 4:
        print(f"i={i}, 1/dx={modinv(dx[i])}")
    else:
        print(f"i={i}, expected 1/dx undefined")

print("\nActual inv_dx:")
for i in range(1, 10):
    print(f"i={i}, inv_dx={inv_dx[i]}")
