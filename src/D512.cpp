// src/D512.cpp
#include "../include/D512.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Впомогательный метод (Парсинг строк) _______________

void D512::multiply64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high) {
    // Разбиваем на 32-битные половинки
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;
    uint64_t b_low = b & 0xFFFFFFFF;
    uint64_t b_high = b >> 32;
    
    // Умножаем половинки
    uint64_t p1 = a_low * b_low;
    uint64_t p2 = a_low * b_high;
    uint64_t p3 = a_high * b_low;
    uint64_t p4 = a_high * b_high;
    
    // Собираем результат
    uint64_t mid = p2 + (p1 >> 32);
    uint64_t mid_low = mid & 0xFFFFFFFF;
    uint64_t mid_high = mid >> 32;
    
    low = (p1 & 0xFFFFFFFF) | (mid_low << 32);
    high = p4 + mid_high + (p3 >> 32) + ((p3 & 0xFFFFFFFF) + (mid_low >> 32) > 0xFFFFFFFF ? 1 : 0);
}

void D512::multiplyBy10() {
    uint64_t carry = 0;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
        uint64_t low, high;
        multiply64(words[i], 10, low, high);
        
        // Прибавляем перенос от предыдущего слова
        uint64_t sum = low + carry;
        bool overflow = (sum < low);  // было переполнение при сложении?
        
        words[i] = sum;
        carry = high + (overflow ? 1 : 0);
    }
    
    // Если после всех слов остался перенос - число не влезло в 512 бит
    if (carry != 0) {
        isNaN = true;
        throw std::overflow_error("D512: число слишком большое (переполнение при умножении на 10)");
    }
}

void D512::addDigit(uint64_t digit) {
    if (digit > 9) return;  // защита от дурака
    
    uint64_t carry = digit;
    
    for (int i = 0; i < WORD_COUNT && carry != 0; ++i) {
        uint64_t sum = words[i] + carry;
        
        if (sum < words[i]) {  // было переполнение
            carry = 1;
        } else {
            carry = 0;
        }
        
        words[i] = sum;
    }
    
    // Если после всех слов остался перенос - число не влезло
    if (carry != 0) {
        isNaN = true;
        throw std::overflow_error("D512: число слишком большое (переполнение при добавлении цифры)");
    }
}

// Конструкторы _______________________________________

D512::D512(int64_t value) noexcept : negative(value < 0) {
    uint64_t absValue = static_cast<uint64_t>(value < 0 ? -value : value);
    words[0] = absValue;
}

D512::D512(const std::string& str) {
    if (str.empty()) {
        isNaN = true;
        throw std::invalid_argument("D512: пустая строка");
    }
    
    size_t start = 0;
    if (str[0] == '-') {
        negative = true;
        start = 1;
    }
    
    // Проверка на пустую строку после знака
    if (start >= str.length()) {
        isNaN = true;
        throw std::invalid_argument("D512: строка содержит только знак минус");
    }
    
    // Проверка, что все символы - цифры
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            isNaN = true;
            throw std::invalid_argument("D512: недопустимый символ в строке");
        }
    }
    
    try {
        // Обрабатываем каждую цифру
        for (size_t i = start; i < str.length(); ++i) {
            uint64_t digit = str[i] - '0';
            
            // Умножаем текущее число на 10
            multiplyBy10();
            
            // Прибавляем цифру
            addDigit(digit);
        }
    } catch (const std::overflow_error& e) {
        isNaN = true;
        throw;  // пробрасываем исключение дальше
    }
    
    // Если получился ноль, сбрасываем флаг отрицательности
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

// Заглушки

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


D512::operator int64_t() const noexcept {
    return static_cast<int64_t>(negative ? -words[0] : words[0]);
}


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



D512 D512::operator+(const D512& other) const {
    // Заглушка - TODO: реализовать
    D512 result;
    result.isNaN = true;
    return result;
}

D512 D512::operator-(const D512& other) const {
    // Заглушка
    D512 result;
    result.isNaN = true;
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

D512& D512::operator+=(const D512& other) {
    *this = *this + other;
    return *this;
}

bool D512::isZero() const noexcept {
    for (int i = 0; i < WORD_COUNT; ++i) {
        if (words[i] != 0) return false;
    }
    return true;
}

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