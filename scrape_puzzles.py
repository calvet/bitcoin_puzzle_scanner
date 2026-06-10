import requests
import re
import base58

def get_hash160(address):
    try:
        decoded = base58.b58decode(address)
        return decoded[1:21].hex()
    except Exception as e:
        return ""

url = "https://privatekeys.pw/puzzles/bitcoin-puzzle-tx?table=0&status=all"
response = requests.get(url, headers={'User-Agent': 'Mozilla/5.0'})
html = response.text

# Each puzzle block seems to have <h5 id="p1">Bitcoin Puzzle #1</h5>
puzzles = {}

# Split by Bitcoin Puzzle #
blocks = html.split('Bitcoin Puzzle #')[1:]
for block in blocks:
    num_match = re.search(r'^(\d+)', block)
    if not num_match: continue
    num = int(num_match.group(1))
    
    # find address in block
    addr_match = re.search(r'address/bitcoin/(1[A-Za-z0-9]+)', block)
    if addr_match:
        address = addr_match.group(1)
        hash160 = get_hash160(address)
        
        status = "SOLVED"
        if 'UNSOLVED' in block[:1500]:
            status = "UNSOLVED"
            
        puzzles[num] = {"address": address, "hash160": hash160, "status": status}

print(f"Found {len(puzzles)} puzzles")
for k in sorted(puzzles.keys())[:5]:
    print(k, puzzles[k])

# Generate C++ header
with open('include/puzzles.h', 'w') as f:
    f.write("#ifndef BITCOIN_PUZZLES_H\n#define BITCOIN_PUZZLES_H\n\n")
    f.write("#include <string>\n#include <map>\n\n")
    f.write("namespace Config {\n")
    f.write("    struct PuzzleInfo {\n")
    f.write("        int puzzle_number;\n")
    f.write("        std::string address;\n")
    f.write("        std::string hash160;\n")
    f.write("        bool solved;\n")
    f.write("    };\n\n")
    f.write("    inline const std::map<int, PuzzleInfo>& GetPuzzles() {\n")
    f.write("        static const std::map<int, PuzzleInfo> puzzles = {\n")
    for num in sorted(puzzles.keys()):
        p = puzzles[num]
        solved_str = "true" if p["status"] == "SOLVED" else "false"
        f.write(f'            {{{num}, {{{num}, "{p["address"]}", "{p["hash160"]}", {solved_str}}}}},\n')
    f.write("        };\n")
    f.write("        return puzzles;\n")
    f.write("    }\n")
    f.write("}\n\n#endif // BITCOIN_PUZZLES_H\n")

print("Generated include/puzzles.h")
