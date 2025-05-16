/*MIT License

Copyright (c) 2025 8891689

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. 
https://github.com/8891689 
*/
/* bitrange.cpp */
#include "bitrange.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Zero out all limbs
static void clear(BigUInt *x) {
    for (int i = 0; i < LIMBS; ++i) x->limbs[i] = 0;
}

// x = 2^exp; throws out_of_range if exp ≥ 256
static void pow2(BigUInt *x, unsigned exp) {
    clear(x);
    if (exp >= LIMBS * 32) {
        throw std::out_of_range("exponent out of range");
    }
    unsigned limb = exp / 32;
    unsigned bit  = exp % 32;
    x->limbs[limb] = (1u << bit);
}

// x = x - 1, assume x != 0
static void dec1(BigUInt *x) {
    for (int i = 0; i < LIMBS; ++i) {
        if (x->limbs[i] > 0) {
            --x->limbs[i];
            break;
        } else {
            x->limbs[i] = 0xFFFFFFFFu;
        }
    }
}

// Parse hex string into x; returns false on any error
static bool from_hex(BigUInt *x, const std::string &hex) {
    clear(x);
    if (hex.empty() || hex.size() > LIMBS * 8) {
        std::cerr << "[E] Invalid hex length\n";
        return false;
    }
    unsigned bitpos = 0;
    for (int i = (int)hex.size() - 1; i >= 0; --i) {
        char c = hex[i];
        uint32_t v;
        if      (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else {
            std::cerr << "[E] Non-hex character '" << c << "'\n";
            return false;
        }
        unsigned limb = bitpos / 32;
        unsigned shift = bitpos % 32;
        x->limbs[limb] |= (v << shift);
        if (shift > 28 && limb + 1 < LIMBS) {
            x->limbs[limb+1] |= (v >> (32 - shift));
        }
        bitpos += 4;
    }
    return true;
}

// Convert x to hex string (no prefix), most significant nibble first
static std::string to_hex(const BigUInt *x) {
    std::ostringstream oss;
    bool nonzero_seen = false;
    for (int i = LIMBS - 1; i >= 0; --i) {
        // print all limbs but suppress leading zeros
        if (nonzero_seen) {
            oss << std::setw(8) << std::setfill('0') << std::hex << x->limbs[i];
        } else if (x->limbs[i] != 0) {
            oss << std::hex << x->limbs[i];
            nonzero_seen = true;
        }
    }
    return nonzero_seen ? oss.str() : "0";
}

int set_bitrange(int bits, BigUInt *min_out, BigUInt *max_out) {
    if (bits < 1 || bits > LIMBS * 32) {
        std::cerr << "[E] bits out of range [1,256]: " << bits << "\n";
        return -1;
    }
    try {
        pow2(min_out, bits - 1);  // compute 2^(bits-1)
        if (bits < LIMBS * 32) {
            pow2(max_out, bits);
            dec1(max_out);
        } else {
            // bits == 256: full ones
            for (int i = 0; i < LIMBS; ++i) max_out->limbs[i] = 0xFFFFFFFFu;
        }
    } catch (const std::out_of_range &e) {
        std::cerr << "[E] " << e.what() << "\n";
        return -1;
    }
    std::cout << "[+] bits=" << bits
              << " → min=0x" << to_hex(min_out)
              << ", max=0x" << to_hex(max_out) << "\n";
    return 0;
}

int set_range(const char *param, BigUInt *min_out, BigUInt *max_out) {
    const char *sep = std::strchr(param, ':');
    if (!sep || sep == param || *(sep + 1) == '\0' || std::strchr(sep + 1, ':')) {
        std::cerr << "[E] format must be A:B\n";
        return -1;
    }
    std::string A(param, sep - param);
    std::string B(sep + 1);
    if (!from_hex(min_out, A) || !from_hex(max_out, B)) {
        return -1;
    }
    std::cout << "[+] range=" << A << ":" << B
              << " → min=0x" << to_hex(min_out)
              << ", max=0x" << to_hex(max_out) << "\n";
    return 0;
}

