#ifndef D256_HPP
#define D256_HPP

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>

class D256 {
    static constexpr int WORD_BITS{64};
    static constexpr int WORD_COUNT{4};
    static constexpr int BYTE_COUNT{32};

    uint64_t words[WORD_COUNT]{0,0,0,0};
    bool negative{false};
    bool isNaN{false};
    
    D256 pow(int64_t exp) const;
    static void mul64by10(uint64_t a, uint64_t& low, uint64_t& high);
    static void div256by10(uint64_t* words, uint64_t& remainder);
    static void mul64x64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high);
    
public:
    D256() noexcept = default;
    D256(int64_t value) noexcept;
    explicit D256(const std::string& str);
    D256(const D256& other) noexcept;
    D256(D256&& other) noexcept;
    ~D256() noexcept = default;
    
    D256& operator=(const D256& other) noexcept = default;
    D256& operator=(D256&& other) noexcept = default;
    
    D256 operator+(const D256& other) const;
    D256 operator-(const D256& other) const;
    D256 operator*(const D256& other) const;
    D256 operator/(const D256& other) const;
    D256 operator%(const D256& other) const;
    
    D256& operator+=(const D256& other);
    D256& operator-=(const D256& other);
    D256& operator*=(const D256& other);
    D256& operator/=(const D256& other);
    D256& operator%=(const D256& other);
    
    bool operator==(const D256& other) const noexcept;
    bool operator!=(const D256& other) const noexcept;
    bool operator<(const D256& other) const noexcept;
    bool operator>(const D256& other) const noexcept;
    bool operator<=(const D256& other) const noexcept;
    bool operator>=(const D256& other) const noexcept;

    D256 operator-() const noexcept;
    D256 operator+() const noexcept;
    
    D256& operator++();
    D256 operator++(int);
    D256& operator--();
    D256 operator--(int);
    
    D256 operator&(const D256& other) const noexcept;
    D256 operator|(const D256& other) const noexcept;
    D256 operator^(const D256& other) const noexcept;
    D256 operator~() const noexcept;
    D256 operator<<(int shift) const noexcept;
    D256 operator>>(int shift) const noexcept;
    
    bool isZero() const noexcept;
    bool isOne() const noexcept;
    bool isNegative() const noexcept { return negative; }
    bool isPositive() const noexcept { return !negative && !isZero(); }
    bool isNan() const noexcept { return isNaN; }
    
    std::string toString() const;
    explicit operator bool() const noexcept { return !isZero(); }
    explicit operator int64_t() const noexcept;
    
    void printHex(std::ostream& os) const;
    size_t hash() const noexcept;
    
    static D256 zero() noexcept { return D256(); }
    static D256 one() noexcept { return D256(1); }
    static D256 max() noexcept;
    static D256 min() noexcept;
    
    friend std::ostream& operator<<(std::ostream& os, const D256& obj);
    friend std::istream& operator>>(std::istream& is, D256& obj);
};

std::ostream& operator<<(std::ostream& os, const D256& obj);
std::istream& operator>>(std::istream& is, D256& obj);

namespace std {
    template<>
    struct hash<D256> {
        size_t operator()(const D256& d) const noexcept {
            return d.hash();
        }
    };
}

#endif