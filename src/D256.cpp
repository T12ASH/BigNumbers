#include "../include/D256.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

static int compareArrays(const uint64_t* a, const uint64_t* b) {
    for (int i = 3; i >= 0; --i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

D256 D256::pow(int64_t exp) const {
    if (exp == 0) return D256(1);
    if (exp < 0) return D256(0);
    D256 result = *this;
    for (int64_t i = 1; i < exp; ++i) {
        result = result * *this;
    }
    return result;
}

void D256::mul64x64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high) {
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;
    uint64_t b_low = b & 0xFFFFFFFF;
    uint64_t b_high = b >> 32;
    
    uint64_t p1 = a_low * b_low;
    uint64_t p2 = a_low * b_high;
    uint64_t p3 = a_high * b_low;
    uint64_t p4 = a_high * b_high;
    
    uint64_t mid = p2 + p3;
    uint64_t carry = (mid < p2) ? 1 : 0;
    
    low = p1 + (mid << 32);
    high = p4 + (mid >> 32) + carry;
    
    if (low < p1) {
        high++;
    }
}

void D256::mul64by10(uint64_t a, uint64_t& low, uint64_t& high) {
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;

    uint64_t p_low = a_low * 10;
    uint64_t p_high = a_high * 10;

    uint64_t carry = p_low >> 32;
    uint64_t mid = (p_high & 0xFFFFFFFF) + carry;

    high = (p_high >> 32) + (mid >> 32);
    low = ((mid & 0xFFFFFFFF) << 32) | (p_low & 0xFFFFFFFF);
}

void D256::div256by10(uint64_t* words, uint64_t& remainder) {
    uint64_t carry = 0;
    
    for (int word = 3; word >= 0; --word) {
        uint32_t high = words[word] >> 32;
        uint32_t low  = words[word] & 0xFFFFFFFF;
        
        uint64_t temp1 = (carry << 32) | high;
        uint32_t q_high = temp1 / 10;
        carry = temp1 % 10;
        
        uint64_t temp2 = (carry << 32) | low;
        uint32_t q_low = temp2 / 10;
        carry = temp2 % 10;

        words[word] = ((uint64_t)q_high << 32) | q_low;
    }
    
    remainder = carry;
}

D256::D256(int64_t value) noexcept : negative(value < 0) {
    uint64_t absValue = static_cast<uint64_t>(value < 0 ? -value : value);
    words[0] = absValue;
}

D256::D256(const std::string& str) {
    if (str.empty()) {
        isNaN = true;
        throw std::invalid_argument("D256: пустая строка");
    }
    
    size_t start = 0;
    if (str[0] == '-') {
        negative = true;
        start = 1;
        if (start >= str.length()) {
            isNaN = true;
            throw std::invalid_argument("D256: после минуса нет цифр");
        }
    }
    
    if (str[start] == '0' && str.length() - start > 1) {
        isNaN = true;
        throw std::invalid_argument("D256: число не может начинаться с нуля");
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            isNaN = true;
            throw std::invalid_argument("D256: недопустимый символ '" + std::string(1, str[i]) + "'");
        }
        if (str[i] == '-') {
            isNaN = true;
            throw std::invalid_argument("D256: минус может быть только первым символом");
        }
    }
    
    if (str.substr(start) == "0") {
        negative = false;
        return;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        uint64_t digit = static_cast<uint64_t>(str[i] - '0');
        uint64_t carry = digit;

        for (int word = 0; word < 4; ++word) {
            uint64_t low{}, high{};
            mul64by10(words[word], low, high);
            
            uint64_t sum = low + carry;
            bool overflow = (sum < low);
            
            words[word] = sum;
            carry = high + overflow;
        }
        
        if (carry != 0) {
            isNaN = true;
            throw std::overflow_error("D256: число превышает 256 бит");
        }
    }
    
    if (isZero()) {
        negative = false;
    }
}

D256::D256(const std::string& str, int base) : negative(false), isNaN(false) {
    if (base < 2 || base > 36) {
        isNaN = true;
        throw std::invalid_argument("D512: base must be between 2 and 36");
    }
    if (str.empty()) {
        isNaN = true;
        throw std::invalid_argument("D512: empty string");
    }

    size_t start = 0;
    if (str[0] == '-') {
        negative = true;
        start = 1;
        if (start >= str.length()) {
            isNaN = true;
            throw std::invalid_argument("D256: no digits after sign");
        }
    }

    std::memset(words, 0, sizeof(words));

    for (size_t i = start; i < str.length(); ++i) {
        char c = str[i];
        uint64_t digit;

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'A' && c <= 'Z')
            digit = 10 + (c - 'A');
        else if (c >= 'a' && c <= 'z')
            digit = 10 + (c - 'a');
        else {
            isNaN = true;
            throw std::invalid_argument("D256: invalid character");
        }

        if (digit >= static_cast<uint64_t>(base)) {
            isNaN = true;
            throw std::invalid_argument("D256: digit out of range for given base");
        }

        uint64_t carry = digit;
        for (int word = 0; word < WORD_COUNT; ++word) {
            uint64_t low, high;
            mul64x64(words[word], static_cast<uint64_t>(base), low, high);

            uint64_t sum = low + carry;
            bool overflow = (sum < low);

            words[word] = sum;
            carry = high + (overflow ? 1 : 0);
        }

        if (carry != 0) {
            isNaN = true;
            throw std::overflow_error("D256: number too large for 512 bits");
        }
    }

    if (isZero()) negative = false;
}

D256::D256(const D256& other) noexcept {
    std::memcpy(words, other.words, sizeof(words));
    negative = other.negative;
    isNaN = other.isNaN;
}

D256::D256(D256&& other) noexcept {
    std::memcpy(words, other.words, sizeof(words));
    negative = other.negative;
    isNaN = other.isNaN;
    std::memset(other.words, 0, sizeof(other.words));
    other.negative = false;
    other.isNaN = false;
}

std::string D256::toString() const {
    if (isNaN) return "NaN";
    if (isZero()) return "0";
    
    D256 temp = *this;
    temp.negative = false;
    std::string result;
    
    while (!temp.isZero()) {
        uint64_t remainder = 0;
        div256by10(temp.words, remainder);
        result.push_back('0' + static_cast<char>(remainder));
    }
    
    if (negative) result.push_back('-');
    std::reverse(result.begin(), result.end());
    return result;
}

bool D256::operator==(const D256& other) const noexcept {
    if (isNaN || other.isNaN) return false;
    if (negative != other.negative) return false;
    
    for (int i = 0; i < 4; ++i) {
        if (words[i] != other.words[i]) return false;
    }
    return true;
}

bool D256::operator!=(const D256& other) const noexcept {
    return !(*this == other);
}

bool D256::operator<(const D256& other) const noexcept {
    if (isNaN || other.isNaN) return false;
    if (negative != other.negative) {
        return negative;
    }
    
    for (int i = 3; i >= 0; --i) {
        if (words[i] != other.words[i]) {
            return negative ? words[i] > other.words[i] : words[i] < other.words[i];
        }
    }
    return false;
}

bool D256::operator>(const D256& other) const noexcept {
    return other < *this;
}

bool D256::operator<=(const D256& other) const noexcept {
    return !(other < *this);
}

bool D256::operator>=(const D256& other) const noexcept {
    return !(*this < other);
}

D256 D256::operator-() const noexcept {
    D256 result = *this;
    if (!result.isZero()) {
        result.negative = !result.negative;
    }
    return result;
}

D256 D256::operator+() const noexcept {
    return *this;
}

D256& D256::operator++() {
    if (!negative) {
        uint64_t carry = 1;
        for (int i = 0; i < 4 && carry; ++i) {
            uint64_t old = words[i];
            words[i] += carry;
            carry = (old > words[i]) ? 1 : 0;
        }
        if(carry) isNaN = true;
    } else {
        uint64_t borrow = 1;
        for (int i = 0; i < 4 && borrow; ++i) {
            uint64_t old = words[i];
            words[i] -= borrow;
            borrow = (old < borrow) ? 1 : 0;
        }
        if (isZero()) negative = false;
    }
    return *this;
}

D256 D256::operator++(int) {
    D256 temp = *this;
    ++(*this);
    return temp;
}

D256& D256::operator--() {
    if (isNaN) return *this;
    
    if (negative) {
        uint64_t carry = 1;
        for (int i = 0; i < 4 && carry; ++i) {
            uint64_t old = words[i];
            words[i] += carry;
            carry = (words[i] < old) ? 1 : 0;
        }
        if (carry) isNaN = true;
    } else {
        if (isZero()) {
            words[0] = 1;
            negative = true;
            return *this;
        }
        uint64_t borrow = 1;
        for (int i = 0; i < 4 && borrow; ++i) {
            uint64_t old = words[i];
            words[i] -= borrow;
            borrow = (old < borrow) ? 1 : 0;
        }
        if (isZero()) negative = false;
    }
    return *this;
}

D256 D256::operator--(int) {
    D256 temp = *this;
    --(*this);
    return temp;
}

D256 D256::operator&(const D256& other) const noexcept {
    D256 result;
    for (int i = 0; i < 4; ++i) {
        result.words[i] = words[i] & other.words[i];
    }
    result.negative = negative & other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D256 D256::operator|(const D256& other) const noexcept {
    D256 result;
    for (int i = 0; i < 4; ++i) {
        result.words[i] = words[i] | other.words[i];
    }
    result.negative = negative | other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D256 D256::operator^(const D256& other) const noexcept {
    D256 result;
    for (int i = 0; i < 4; ++i) {
        result.words[i] = words[i] ^ other.words[i];
    }
    result.negative = negative ^ other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D256 D256::operator~() const noexcept {
    D256 result;
    for (int i = 0; i < 4; ++i) {
        result.words[i] = ~words[i];
    }
    result.negative = !negative;
    result.isNaN = isNaN;
    return result;
}

D256 D256::operator<<(int shift) const noexcept {
    if (shift <= 0) return *this;
    D256 result;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    if (word_shift >= 4) return result;
    for (int i = 0; i < 4 - word_shift; ++i) {
        result.words[i + word_shift] = words[i] << bit_shift;
    }
    if (bit_shift != 0) {
        for (int i = 0; i < 4 - word_shift - 1; ++i) {
            result.words[i + word_shift + 1] |= words[i] >> (64 - bit_shift);
        }
    }
    result.negative = negative;
    result.isNaN = isNaN;
    return result;
}

D256 D256::operator>>(int shift) const noexcept {
    if (shift <= 0) return *this;
    D256 result;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    if (word_shift >= 4) return result;
    for (int i = word_shift; i < 4; ++i) {
        result.words[i - word_shift] = words[i] >> bit_shift;
    }
    if (bit_shift != 0) {
        for (int i = word_shift + 1; i < 4; ++i) {
            result.words[i - word_shift - 1] |= words[i] << (64 - bit_shift);
        }
    }
    result.negative = negative;
    result.isNaN = isNaN;
    return result;
}

D256::operator int64_t() const noexcept {
    return static_cast<int64_t>(negative ? -words[0] : words[0]);
}

void D256::printHex(std::ostream& os) const {
    std::ios_base::fmtflags flags(os.flags());
    os << std::hex << std::setfill('0');
    if (negative) os << "-";
    os << "0x";
    for (int i = 3; i >= 0; --i) {
        os << std::setw(16) << words[i];
    }
    os.flags(flags);
}

size_t D256::hash() const noexcept {
    size_t h = 0;
    for (int i = 0; i < 4; ++i) {
        h ^= std::hash<uint64_t>{}(words[i]);
    }
    h ^= std::hash<bool>{}(negative);
    return h;
}

D256 D256::max() noexcept {
    D256 result;
    result.negative = false;
    std::memset(result.words, 0xFF, sizeof(result.words));
    return result;
}

D256 D256::min() noexcept {
    D256 result = max();
    result.negative = true;
    return result;
}

D256 D256::operator+(const D256& other) const {
    if (isNaN || other.isNaN) {
        D256 result;
        result.isNaN = true;
        return result;
    }
    if (negative != other.negative) {
        D256 temp = other;
        temp.negative = !temp.negative;
        return *this - temp;
    }
    D256 result;
    uint64_t carry = 0;
    for (int i = 0; i < 4; ++i) {
        uint64_t sum = words[i] + other.words[i] + carry;
        if (sum < words[i] || (carry && sum == words[i])) {
            carry = 1;
        } else {
            carry = 0;
        }
        result.words[i] = sum;
    }
    result.negative = negative;
    if (carry) {
        result.isNaN = true;
    }
    return result;
}

D256 D256::operator-(const D256& other) const {
    if (isNaN || other.isNaN) {
        D256 result;
        result.isNaN = true;
        return result;
    }
    if (negative != other.negative) {
        D256 temp = other;
        temp.negative = !temp.negative;
        return *this + temp;
    }
    bool absLess = false;
    for (int i = 3; i >= 0; --i) {
        if (words[i] != other.words[i]) {
            absLess = words[i] < other.words[i];
            break;
        }
    }
    bool resultNegative = negative ? !absLess : absLess;
    const D256* larger;
    const D256* smaller;
    if (absLess) {
        larger = &other;
        smaller = this;
    } else {
        larger = this;
        smaller = &other;
    }
    D256 result;
    uint64_t borrow = 0;
    for (int i = 0; i < 4; ++i) {
        uint64_t diff = larger->words[i] - smaller->words[i] - borrow;
        if (diff > larger->words[i] || (borrow && diff == larger->words[i])) {
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.words[i] = diff;
    }
    result.negative = resultNegative;
    return result;
}

D256 D256::operator*(const D256& other) const {
    if (isNaN || other.isNaN) {
        D256 result;
        result.isNaN = true;
        return result;
    }
    D256 result;
    result.negative = negative != other.negative;
    uint64_t temp[8] = {0};
    for (int i = 0; i < 4; ++i) {
        if (words[i] == 0) continue;
        uint64_t carry = 0;
        for (int j = 0; j < 4; ++j) {
            uint64_t low, high;
            mul64x64(words[i], other.words[j], low, high);
            uint64_t sum = temp[i + j] + low + carry;
            bool overflow1 = (sum < low);
            bool overflow2 = (sum < carry);
            temp[i + j] = sum;
            carry = high + (overflow1 ? 1 : 0) + (overflow2 ? 1 : 0);
        }
        if (carry) {
            temp[i + 4] += carry;
        }
    }
    for (int i = 0; i < 4; ++i) {
        result.words[i] = temp[i];
    }
    for (int i = 4; i < 8; ++i) {
        if (temp[i] != 0) {
            result.isNaN = true;
            break;
        }
    }
    if (result.isZero()) result.negative = false;
    return result;
}

D256 D256::operator/(const D256& other) const {
    if (isNaN || other.isNaN || other.isZero()) {
        D256 result;
        result.isNaN = true;
        return result;
    }
    bool resultNegative = negative != other.negative;
    D256 dividend = *this;
    D256 divisor = other;
    dividend.negative = false;
    divisor.negative = false;
    if (dividend < divisor) {
        D256 result;
        result.negative = resultNegative;
        return result;
    }
    uint64_t quotient[4] = {0};
    uint64_t remainder[4] = {0};
    for (int bit = 255; bit >= 0; --bit) {
        uint64_t carry = 0;
        for (int i = 0; i < 4; ++i) {
            uint64_t newCarry = remainder[i] >> 63;
            remainder[i] = (remainder[i] << 1) | carry;
            carry = newCarry;
        }
        int wordIdx = bit / 64;
        int bitIdx = bit % 64;
        if (dividend.words[wordIdx] & (1ULL << bitIdx)) {
            remainder[0] |= 1;
        }
        if (compareArrays(remainder, divisor.words) >= 0) {
            uint64_t borrow = 0;
            for (int i = 0; i < 4; ++i) {
                uint64_t diff = remainder[i] - divisor.words[i] - borrow;
                borrow = (diff > remainder[i] || (borrow && diff == remainder[i])) ? 1 : 0;
                remainder[i] = diff;
            }
            int qWordIdx = bit / 64;
            int qBitIdx = bit % 64;
            quotient[qWordIdx] |= (1ULL << qBitIdx);
        }
    }
    D256 result;
    result.negative = resultNegative;
    for (int i = 0; i < 4; ++i) {
        result.words[i] = quotient[i];
    }
    if (result.isZero()) result.negative = false;
    return result;
}

D256 D256::operator%(const D256& other) const {
    if (isNaN || other.isNaN || other.isZero()) {
        D256 result;
        result.isNaN = true;
        return result;
    }
    D256 quotient = *this / other;
    D256 product = quotient * other;
    D256 remainder = *this - product;
    if (!remainder.isZero() && negative != remainder.negative) {
        remainder = remainder + other;
    }
    return remainder;
}

D256& D256::operator+=(const D256& other) {
    *this = *this + other;
    return *this;
}

D256& D256::operator-=(const D256& other) {
    *this = *this - other;
    return *this;
}

bool D256::isZero() const noexcept {
    for (int i = 0; i < 4; ++i) {
        if (words[i] != 0) return false;
    }
    return true;
}

bool D256::isOne() const noexcept {
    if (negative) return false;
    for (int i = 1; i < 4; ++i) {
        if (words[i] != 0) return false;
    }
    return words[0] == 1;
}

std::ostream& operator<<(std::ostream& os, const D256& obj) {
    os << obj.toString();
    return os;
}

std::istream& operator>>(std::istream& is, D256& obj) {
    std::string str;
    is >> str;
    obj = D256(str);
    return is;
}

std::string D256::toBase(int base) const {
    if (base < 2 || base > 36) {
        throw std::invalid_argument("D512: base must be between 2 and 36");
    }
    if (isNaN) return "NaN";
    if (isZero()) return "0";
    
    D256 temp = *this;
    temp.negative = false;
    D256 divisor(base);
    std::string result;
    
    while (!temp.isZero()) {
        D256 remainder = temp % divisor;
        temp = temp / divisor;
        
        uint64_t digit = remainder.words[0];
        char ch;
        if (digit < 10) {
            ch = '0' + static_cast<char>(digit);
        } else {
            ch = 'A' + static_cast<char>(digit - 10);
        }
        result.push_back(ch);
    }
    
    std::reverse(result.begin(), result.end());
    
    if (negative) {
        result = "-" + result;
    }
    
    return result;
}