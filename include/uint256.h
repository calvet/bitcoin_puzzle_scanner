#ifndef BITCOIN_PUZZLE_SCANNER_UINT256_H
#define BITCOIN_PUZZLE_SCANNER_UINT256_H

#include <cstdint>
#include <string>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace Types {

    struct UInt256 {
        uint64_t q3; // Most significant
        uint64_t q2;
        uint64_t q1;
        uint64_t q0; // Least significant

        UInt256() : q3(0), q2(0), q1(0), q0(0) {}
        UInt256(uint64_t val) : q3(0), q2(0), q1(0), q0(val) {}
        UInt256(uint64_t h3, uint64_t h2, uint64_t h1, uint64_t h0) : q3(h3), q2(h2), q1(h1), q0(h0) {}

        static UInt256 from_hex(const std::string& hex_str) {
            std::string s = hex_str;
            if (s.substr(0, 2) == "0x" || s.substr(0, 2) == "0X") {
                s = s.substr(2);
            }
            if (s.length() > 64) {
                throw std::invalid_argument("Hex string too long for UInt256");
            }

            UInt256 result;
            int len_s = static_cast<int>(s.length());
            int parts = (len_s + 15) / 16;
            
            for (int i = 0; i < parts; ++i) {
                int len = std::min(16, len_s - i * 16);
                int start = len_s - i * 16 - len;
                std::string part = s.substr(start, len);
                uint64_t val = std::stoull(part, nullptr, 16);
                if (i == 0) result.q0 = val;
                else if (i == 1) result.q1 = val;
                else if (i == 2) result.q2 = val;
                else if (i == 3) result.q3 = val;
            }
            return result;
        }

        std::string to_hex() const {
            std::stringstream ss;
            bool print = false;
            
            if (q3 > 0) {
                ss << std::hex << q3;
                print = true;
            }
            if (print) ss << std::setfill('0') << std::setw(16);
            if (q2 > 0 || print) {
                ss << std::hex << q2;
                print = true;
            }
            if (print) ss << std::setfill('0') << std::setw(16);
            if (q1 > 0 || print) {
                ss << std::hex << q1;
                print = true;
            }
            if (print) ss << std::setfill('0') << std::setw(16);
            ss << std::hex << q0;
            
            return ss.str();
        }

        UInt256& operator+=(uint64_t val) {
            uint64_t old_q0 = q0;
            q0 += val;
            if (q0 < old_q0) {
                uint64_t old_q1 = q1;
                q1 += 1;
                if (q1 < old_q1) {
                    uint64_t old_q2 = q2;
                    q2 += 1;
                    if (q2 < old_q2) {
                        q3 += 1;
                    }
                }
            }
            return *this;
        }

        UInt256 operator+(uint64_t val) const {
            UInt256 res = *this;
            res += val;
            return res;
        }

        UInt256 operator+(const UInt256& other) const {
            UInt256 res = *this;
            uint64_t carry = 0;
            
            uint64_t old_q0 = res.q0;
            res.q0 += other.q0;
            if (res.q0 < old_q0) carry = 1; else carry = 0;

            uint64_t old_q1 = res.q1;
            res.q1 += other.q1 + carry;
            if (res.q1 < old_q1 || (carry && res.q1 == old_q1)) carry = 1; else carry = 0;

            uint64_t old_q2 = res.q2;
            res.q2 += other.q2 + carry;
            if (res.q2 < old_q2 || (carry && res.q2 == old_q2)) carry = 1; else carry = 0;

            res.q3 += other.q3 + carry;

            return res;
        }

        UInt256& operator++() {
            *this += 1;
            return *this;
        }

        bool operator==(const UInt256& other) const {
            return q3 == other.q3 && q2 == other.q2 && q1 == other.q1 && q0 == other.q0;
        }
        
        bool operator!=(const UInt256& other) const {
            return !(*this == other);
        }

        bool operator<(const UInt256& other) const {
            if (q3 != other.q3) return q3 < other.q3;
            if (q2 != other.q2) return q2 < other.q2;
            if (q1 != other.q1) return q1 < other.q1;
            return q0 < other.q0;
        }

        bool operator<=(const UInt256& other) const {
            return *this < other || *this == other;
        }

        bool operator>(const UInt256& other) const {
            return !(*this <= other);
        }

        bool operator>=(const UInt256& other) const {
            return !(*this < other);
        }

        uint64_t operator-(const UInt256& other) const {
            if (*this < other) return 0;
            if (q3 == other.q3 && q2 == other.q2 && q1 == other.q1) {
                return q0 - other.q0;
            } else if (q3 == other.q3 && q2 == other.q2 && q1 == other.q1 + 1) {
                return q0 + (~other.q0 + 1);
            }
            return 0xFFFFFFFFFFFFFFFFULL;
        }
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_UINT256_H
