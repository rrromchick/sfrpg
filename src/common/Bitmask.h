#pragma once

#include <stdint.h>

using Bitset = uint32_t;

class Bitmask {
public:
    Bitmask() : bits(0) {}
    Bitmask(const Bitset& l_bits) : bits(l_bits) {}

    Bitset getMask() const { return bits; }
    void setMask(const Bitset& l_bits) { bits = l_bits; }

    bool matches(const Bitmask& l_bits, const Bitset& l_relevant = 0) const {
        return (l_relevant ? ((l_bits.getMask() & l_relevant) == (bits & l_relevant)) :
            (l_bits.getMask() == bits));
    }

    bool getBit(const unsigned int& l_pos) const {
        return ((bits & (1 << l_pos)) != 0);
    }

    void turnOnBit(const unsigned int& l_pos) {
        bits |= 1 << l_pos;
    }

    void turnOnBits(const Bitset& l_bits) {
        bits |= l_bits;
    }

    void clearBit(const unsigned int& l_pos) {
        bits &= ~(1 << l_pos);
    }

    void toggleBit(const unsigned int& l_pos) {
        bits ^= 1 << l_pos;
    }

    void clear() { bits = 0; }
private:
    Bitset bits;
};