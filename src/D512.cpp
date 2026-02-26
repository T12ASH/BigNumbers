// src/D512.cpp
#include "../include/D512.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>


// Конструкторы _______________________________________

D512::D512(int64_t value) noexcept : negative(value < 0) {
    uint64_t absValue = static_cast<uint64_t>(value < 0 ? -value : value);
    words[0] = absValue;
}

D512::D512(const std::string& str) {
    // 1. Проверка на пустую строку
    if (str.empty()) {
        isNaN = true;
        throw std::invalid_argument("D512: пустая строка");
    }
    
    size_t start = 0;
    negative = false;
    
    // 2. Проверка первого символа на минус
    if (str[0] == '-') {
        negative = true;
        start = 1;
        
        // 3. После минуса должны быть цифры
        if (start >= str.length()) {
            isNaN = true;
            throw std::invalid_argument("D512: после минуса нет цифр");
        }
    }
    
    // 4. Проверка что первый символ после знака - не ноль
    if (str[start] == '0') {
        // Если это единственный символ - ок (число 0)
        if (str.length() - start > 1) {
            isNaN = true;
            throw std::invalid_argument("D512: число не может начинаться с нуля");
        }
    }
    
    // 5. Проверка всех символов - только цифры
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            isNaN = true;
            throw std::invalid_argument("D512: недопустимый символ '" + std::string(1, str[i]) + "' - нужны только цифры");
        }
    }
    
    // 6. Доп проверка - нет ли второго минуса где-то внутри
    for (size_t i = start; i < str.length(); ++i) {
        if (str[i] == '-') {
            isNaN = true;
            throw std::invalid_argument("D512: минус может быть только первым символом");
        }
    }
    
    // Парсинг числа
    std::string digits = str.substr(start);
    
    // Если число "0" - сразу выходим
    if (digits == "0") {
        negative = false;  // -0 не бывает
        return;
    }
    
    std::vector<uint8_t> bits;
    
    while (digits != "0") {
        int remainder = (digits.back() - '0') % 2;
        bits.push_back(static_cast<uint8_t>(remainder));
        
        std::string next;
        int carry = 0;
        for (char c : digits) {
            int current = carry * 10 + (c - '0');
            int digit = current / 2;
            carry = current % 2;
            
            if (!next.empty() || digit != 0) {
                next.push_back('0' + digit);
            }
        }
        
        digits = next.empty() ? "0" : next;
        
        // Защита от бесконечного цикла
        if (bits.size() > WORD_BITS * WORD_COUNT + 1) {
            isNaN = true;
            throw std::overflow_error("D512: число слишком большое");
        }
    }
    
    if (bits.size() > WORD_BITS * WORD_COUNT) {
        isNaN = true;
        throw std::overflow_error("D512: число превышает 512 бит");
    }
    
    // Заполняем слова
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) {
            size_t word_idx = i / WORD_BITS;
            size_t bit_idx = i % WORD_BITS;
            words[word_idx] |= (1ULL << bit_idx);
        }
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
// Заглушки

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

D512& D512::operator+=(const D512& other) {
    *this = *this + other;
    return *this;
}

D512& D512::operator-=(const D512& other) {
    *this = *this - other;
    return *this;
}

bool D512::isZero() const noexcept {
    for (int i = 0; i < WORD_COUNT; ++i) {
        if (words[i] != 0) return false;
    }
    return true;
}

