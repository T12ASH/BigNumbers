#include "../include/D512.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

// Вспомогательные методы _____________________________

void D512::mul64by10(uint64_t a, uint64_t& low, uint64_t& high) {
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;

    uint64_t p_low = a_low * 10;
    uint64_t p_high = a_high * 10;

    uint64_t carry = p_low >> 32;
    uint64_t mid = (p_high & 0xFFFFFFFF) + carry;

    high = (p_high >> 32) + (mid >> 32);
    low = ((mid & 0xFFFFFFFF) << 32) | (p_low & 0xFFFFFFFF);
}

void D512::div128by10(uint64_t high, uint64_t low,
    uint64_t& quotient_high, uint64_t& quotient_low,
    uint64_t& remainder) {

    uint64_t q_high = high / 10;
    uint64_t r_high = high % 10;

    uint64_t mid1 = (r_high << 32) | (low >> 32);
    uint64_t q_mid1 = mid1 / 10;
    uint64_t r_mid1 = mid1 % 10;

    uint64_t mid2 = (r_mid1 << 32) | (low & 0xFFFFFFFF);
    uint64_t q_mid2 = mid2 / 10;
    remainder = mid2 % 10;

    quotient_high = q_high;
    quotient_low = (q_mid1 << 32) | q_mid2;
}


// Конструкторы _______________________________________

D512::D512(int64_t value) noexcept : negative(value < 0) {
    uint64_t absValue = static_cast<uint64_t>(value < 0 ? -value : value);
    words[0] = absValue;
}

D512::D512(const std::string& str) {
    // 1. Сброс состояния
    negative = false;
    isNaN = false;
    std::memset(words, 0, sizeof(words));
    
    // 2. Проверка на пустую строку
    if (str.empty()) {
        isNaN = true;
        throw std::invalid_argument("D512: пустая строка");
    }
    
    size_t start = 0;
    
    // 3. Обработка знака
    if (str[0] == '-') {
        negative = true;
        start = 1;
        
        if (start >= str.length()) {
            isNaN = true;
            throw std::invalid_argument("D512: после минуса нет цифр");
        }
    }
    
    // 4. Проверка на ведущий ноль
    if (str[start] == '0' && str.length() - start > 1) {
        isNaN = true;
        throw std::invalid_argument("D512: число не может начинаться с нуля");
    }
    
    // 5. Валидация всех символов
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            isNaN = true;
            throw std::invalid_argument("D512: недопустимый символ '" + std::string(1, str[i]) + "'");
        }
        if (str[i] == '-') {
            isNaN = true;
            throw std::invalid_argument("D512: минус может быть только первым символом");
        }
    }
    
    if (str.substr(start) == "0") {
        negative = false;
        return;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        uint64_t digit = str[i] - '0';
        uint64_t carry = digit;

        for (int word = 0; word < WORD_COUNT; ++word) {
            uint64_t low, high;
            mul64by10(words[word], low, high);
            
            uint64_t sum = low + carry;
            bool overflow = (sum < low);
            
            words[word] = sum;
            carry = high + (overflow ? 1 : 0);
        }
        
        if (carry != 0) {
            isNaN = true;
            throw std::overflow_error("D512: число превышает 512 бит");
        }
    }
    
    if (isZero()) {
        negative = false;
    }
}

D512::D512(const D512& other) noexcept {
    std::memcpy(words, other.words, sizeof(words));
    negative = other.negative;
    isNaN = other.isNaN;
}

D512::D512(D512&& other) noexcept {
    std::memcpy(words, other.words, sizeof(words));
    negative = other.negative;
    isNaN = other.isNaN;
    
    // Очищаем исходный объект
    std::memset(other.words, 0, sizeof(other.words));
    other.negative = false;
    other.isNaN = false;
}

D512::~D512() noexcept = default;

std::string D512::toString() const {
    if (isNaN) return "NaN";
    if (isZero()) return "0";
    
    D512 temp = *this;
    temp.negative = false;
    std::string result;
    
    while (!temp.isZero()) {
        uint64_t remainder = 0;
        
        // Делим число на 10
        for (int i = WORD_COUNT - 1; i >= 0; --i) {
            uint64_t part = temp.words[i];
            
            // Делим 64-битное слово на 10 с учетом остатка
            // Используем тот факт, что 2^64 / 10 примерно 1844674407370955161
            uint64_t quotient = 0;
            uint64_t rem = remainder;
            
            // Эмуляция 128-битного деления через 64-битные операции
            for (int bit = 63; bit >= 0; --bit) {
                rem = (rem << 1) | ((part >> bit) & 1);
                if (rem >= 10) {
                    rem -= 10;
                    quotient |= (1ULL << bit);
                }
            }
            
            temp.words[i] = quotient;
            remainder = rem;
        }
        
        result.push_back('0' + static_cast<char>(remainder));
    }
    
    if (negative) result.push_back('-');
    std::reverse(result.begin(), result.end());
    return result;
}


// Операторы сравнения ________________________________

bool D512::operator==(const D512& other) const noexcept {
    if (isNaN || other.isNaN) return false;
    if (negative != other.negative) return false;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
        if (words[i] != other.words[i]) return false;
    }
    return true;
}

bool D512::operator!=(const D512& other) const noexcept {
    return !(*this == other);
}

bool D512::operator<(const D512& other) const noexcept {
    if (isNaN || other.isNaN) return false;
    
    // Разные знаки
    if (negative != other.negative) {
        return negative;  // отрицательное < положительного
    }
    
    // Одинаковые знаки - сравниваем абсолютные значения
    for (int i = WORD_COUNT - 1; i >= 0; --i) {
        if (words[i] != other.words[i]) {
            return negative ? words[i] > other.words[i] : words[i] < other.words[i];
        }
    }
    return false;  // равны
}

bool D512::operator>(const D512& other) const noexcept {
    return other < *this;
}

bool D512::operator<=(const D512& other) const noexcept {
    return !(other < *this);
}

bool D512::operator>=(const D512& other) const noexcept {
    return !(*this < other);
}


// Унарные операторы __________________________________

D512 D512::operator-() const noexcept {
    D512 result = *this;
    if (!result.isZero()) {
        result.negative = !result.negative;
    }
    return result;
}

D512 D512::operator+() const noexcept {
    return *this;
}


// Инкремент/декремент ________________________________

D512& D512::operator++() {
    // TODO: Реализовать инкремент с учетом переноса
    if (!negative) {
        // Увеличиваем абсолютное значение
        for (int i = 0; i < WORD_COUNT; ++i) {
            if (++words[i] != 0) break;  // Если нет переполнения
        }
    } else {
        // Для отрицательных чисел инкремент уменьшает абсолютное значение
        // TODO: Сложнее, нужно учитывать заем
    }
    return *this;
}

D512 D512::operator++(int) {
    D512 temp = *this;
    ++(*this);
    return temp;
}

D512& D512::operator--() {
    // TODO: Реализовать декремент
    return *this;
}

D512 D512::operator--(int) {
    D512 temp = *this;
    --(*this);
    return temp;
}


// Битовые операторы __________________________________

D512 D512::operator&(const D512& other) const noexcept {
    D512 result;
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = words[i] & other.words[i];
    }
    result.negative = negative & other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D512 D512::operator|(const D512& other) const noexcept {
    D512 result;
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = words[i] | other.words[i];
    }
    result.negative = negative | other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D512 D512::operator^(const D512& other) const noexcept {
    D512 result;
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = words[i] ^ other.words[i];
    }
    result.negative = negative ^ other.negative;
    result.isNaN = isNaN | other.isNaN;
    return result;
}

D512 D512::operator~() const noexcept {
    D512 result;
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = ~words[i];
    }
    result.negative = !negative;
    result.isNaN = isNaN;
    return result;
}

D512 D512::operator<<(int shift) const noexcept {
    if (shift <= 0) return *this;
    
    D512 result;
    int word_shift = shift / WORD_BITS;
    int bit_shift = shift % WORD_BITS;
    
    if (word_shift >= WORD_COUNT) return result;
    
    for (int i = 0; i < WORD_COUNT - word_shift; ++i) {
        result.words[i + word_shift] = words[i] << bit_shift;
    }
    
    if (bit_shift != 0) {
        for (int i = 0; i < WORD_COUNT - word_shift - 1; ++i) {
            result.words[i + word_shift + 1] |= words[i] >> (WORD_BITS - bit_shift);
        }
    }
    
    result.negative = negative;
    result.isNaN = isNaN;
    return result;
}

D512 D512::operator>>(int shift) const noexcept {
    if (shift <= 0) return *this;
    
    D512 result;
    int word_shift = shift / WORD_BITS;
    int bit_shift = shift % WORD_BITS;
    
    if (word_shift >= WORD_COUNT) return result;
    
    for (int i = word_shift; i < WORD_COUNT; ++i) {
        result.words[i - word_shift] = words[i] >> bit_shift;
    }
    
    if (bit_shift != 0) {
        for (int i = word_shift + 1; i < WORD_COUNT; ++i) {
            result.words[i - word_shift - 1] |= words[i] << (WORD_BITS - bit_shift);
        }
    }
    
    result.negative = negative;
    result.isNaN = isNaN;
    return result;
}


// Преобразования _____________________________________

D512::operator int64_t() const noexcept {
    return static_cast<int64_t>(negative ? -words[0] : words[0]);
}


// Вспомогательные методы ____________________________

void D512::printHex(std::ostream& os) const {
    std::ios_base::fmtflags flags(os.flags());
    os << std::hex << std::setfill('0');
    
    if (negative) os << "-";
    os << "0x";
    
    for (int i = WORD_COUNT - 1; i >= 0; --i) {
        os << std::setw(16) << words[i];
    }
    
    os.flags(flags);
}

size_t D512::hash() const noexcept {
    // Простая хеш-функция XOR всех слов
    size_t h = 0;
    for (int i = 0; i < WORD_COUNT; ++i) {
        h ^= std::hash<uint64_t>{}(words[i]);
    }
    h ^= std::hash<bool>{}(negative);
    return h;
}


// Статические методы _______________________________

D512 D512::max() noexcept {
    D512 result;
    result.negative = false;
    std::memset(result.words, 0xFF, sizeof(result.words));  // Все биты = 1
    return result;
}

D512 D512::min() noexcept {
    D512 result = max();
    result.negative = true;
    return result;
}


// Арифметические операторы _________________________

D512 D512::operator+(const D512& other) const {
    if (isNaN || other.isNaN) {
        D512 result;
        result.isNaN = true;
        return result;
    }
    
    // Разные знаки - вызываем вычитание
    if (negative != other.negative) {
        D512 temp = other;
        temp.negative = !temp.negative;
        return *this - temp;
    }
    
    D512 result;
    uint64_t carry = 0;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
        uint64_t sum = words[i] + other.words[i] + carry;
        
        // Проверка переполнения
        if (sum < words[i] || (carry && sum == words[i])) {
            carry = 1;
        } else {
            carry = 0;
        }
        
        result.words[i] = sum;
    }
    
    result.negative = negative;
    
    if (carry) {
        result.isNaN = true;  // Переполнение 512 бит
    }
    
    return result;
}

D512 D512::operator-(const D512& other) const {
    if (isNaN || other.isNaN) {
        D512 result;
        result.isNaN = true;
        return result;
    }
    
    // Разные знаки
    if (negative != other.negative) {
        D512 temp = other;
        temp.negative = !temp.negative;
        return *this + temp;
    }
    
    // Сравниваем абсолютные значения
    bool absLess = false;
    for (int i = WORD_COUNT - 1; i >= 0; --i) {
        if (words[i] != other.words[i]) {
            absLess = words[i] < other.words[i];
            break;
        }
    }
    
    // Определяем знак результата
    bool resultNegative = negative ? !absLess : absLess;
    
    // Выбираем большее и меньшее
    const D512* larger;
    const D512* smaller;
    
    if (absLess) {
        larger = &other;
        smaller = this;
    } else {
        larger = this;
        smaller = &other;
    }
    
    D512 result;
    uint64_t borrow = 0;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
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

D512 D512::operator*(const D512& other) const {
    // Заглушка
    D512 result;
    result.isNaN = true;
    return result;
}

D512 D512::operator/(const D512& other) const {
    // Заглушка
    D512 result;
    result.isNaN = true;
    return result;
}


// Операторы с присваиванием ________________________

D512& D512::operator+=(const D512& other) {
    *this = *this + other;
    return *this;
}

D512& D512::operator-=(const D512& other) {
    *this = *this - other;
    return *this;
}


// Проверки состояния _______________________________

bool D512::isZero() const noexcept {
    for (int i = 0; i < WORD_COUNT; ++i) {
        if (words[i] != 0) return false;
    }
    return true;
}


// Глобальные операторы _____________________________

std::ostream& operator<<(std::ostream& os, const D512& obj) {
    os << obj.toString();
    return os;
}

std::istream& operator>>(std::istream& is, D512& obj) {
    std::string str;
    is >> str;
    obj = D512(str);
    return is;
}