#include "../include/D512.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

// Вспомогательные методы _____________________________
static int compareArrays(const uint64_t* a, const uint64_t* b) {
    for (int i = 8 - 1; i >= 0; --i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

D512 D512::pow(int64_t exp) const {
    if (exp == 0) return D512(1);
    if (exp < 0) return D512(0);
    D512 result = *this;
    for (int64_t i = 1; i < exp; ++i) {
        result = result * *this;
    }
    return result;
}
// Сложение без проверок (для Карацубы)
D512 D512::karatsuba_add(const D512& other) const {
    D512 result;
    uint64_t carry = 0;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
        uint64_t sum = words[i] + other.words[i] + carry;
        carry = (sum < words[i] || (carry && sum == words[i])) ? 1 : 0;
        result.words[i] = sum;
    }
    
    result.negative = negative;
    return result;
}

// Вычитание без проверок (для Карацубы)
D512 D512::karatsuba_sub(const D512& other) const {
    D512 result;
    uint64_t borrow = 0;
    
    for (int i = 0; i < WORD_COUNT; ++i) {
        uint64_t diff = words[i] - other.words[i] - borrow;
        borrow = (diff > words[i] || (borrow && diff == words[i])) ? 1 : 0;
        result.words[i] = diff;
    }
    
    result.negative = negative;
    return result;
}

// Рекурсивная Карацуба
D512 D512::karatsuba(const D512& other, int depth) const {
    // Базовый случай: если слова кончились или маленькое число — обычное умножение
    const int HALF = WORD_COUNT / 2;
    
    if (depth >= 2 || WORD_COUNT <= 2) {
        // Обычное умножение (упрощённое для Карацубы)
        D512 result;
        result.negative = negative != other.negative;
        
        uint64_t temp[WORD_COUNT * 2] = {0};
        
        for (int i = 0; i < WORD_COUNT; ++i) {
            if (words[i] == 0) continue;
            
            uint64_t carry = 0;
            for (int j = 0; j < WORD_COUNT; ++j) {
                uint64_t low, high;
                mul64x64(words[i], other.words[j], low, high);
                
                uint64_t sum = temp[i + j] + low + carry;
                
                bool overflow1 = (sum < low);
                bool overflow2 = (sum < carry);
                
                temp[i + j] = sum;
                carry = high + (overflow1 ? 1 : 0) + (overflow2 ? 1 : 0);
            }
            if (carry) {
                temp[i + WORD_COUNT] += carry;
            }
        }
        
        for (int i = 0; i < WORD_COUNT; ++i) {
            result.words[i] = temp[i];
        }
        
        for (int i = WORD_COUNT; i < WORD_COUNT * 2; ++i) {
            if (temp[i] != 0) {
                result.isNaN = true;
                break;
            }
        }
        
        if (result.isZero()) result.negative = false;
        return result;
    }
    
    // Разбиваем на половинки
    D512 a, b, c, d;
    
    for (int i = 0; i < HALF; ++i) {
        a.words[i] = words[i + HALF];  // старшая половина
        b.words[i] = words[i];          // младшая половина
        c.words[i] = other.words[i + HALF];
        d.words[i] = other.words[i];
    }
    
    // Рекурсивные вызовы
    D512 ac = a.karatsuba(c, depth + 1);
    D512 bd = b.karatsuba(d, depth + 1);
    
    // (a+b) и (c+d)
    D512 sum_a = a.karatsuba_add(b);
    D512 sum_c = c.karatsuba_add(d);
    D512 abcd = sum_a.karatsuba(sum_c, depth + 1);
    
    // middle = (a+b)(c+d) - ac - bd
    D512 middle = abcd.karatsuba_sub(ac).karatsuba_sub(bd);
    
    // Сборка результата:
    // result = ac << 512 + middle << 256 + bd
    D512 result;
    
    // ac << 512 (сдвиг на 8 слов)
    for (int i = 0; i < HALF; ++i) {
        result.words[i + 2 * HALF] = ac.words[i];
    }
    
    // middle << 256 (сдвиг на 4 слова)
    uint64_t carry = 0;
    for (int i = 0; i < HALF; ++i) {
        uint64_t sum = result.words[i + HALF] + middle.words[i] + carry;
        carry = (sum < middle.words[i] || (carry && sum == middle.words[i])) ? 1 : 0;
        result.words[i + HALF] = sum;
    }
    if (carry) result.isNaN = true;
    
    // bd (младшие слова)
    carry = 0;
    for (int i = 0; i < HALF; ++i) {
        uint64_t sum = result.words[i] + bd.words[i] + carry;
        carry = (sum < bd.words[i] || (carry && sum == bd.words[i])) ? 1 : 0;
        result.words[i] = sum;
    }
    if (carry) result.isNaN = true;
    
    result.negative = negative != other.negative;
    
    if (result.isZero()) result.negative = false;
    
    return result;
}


void D512::mul64x64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high) {
    // Разбиваем на 32-битные половинки
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;
    uint64_t b_low = b & 0xFFFFFFFF;
    uint64_t b_high = b >> 32;
    
    // Умножаем половинки
    uint64_t p1 = a_low * b_low;      // 32x32 → 64 (младшие)
    uint64_t p2 = a_low * b_high;
    uint64_t p3 = a_high * b_low;
    uint64_t p4 = a_high * b_high;    // 32x32 → 64 (старшие)
    
    // Собираем 128-битный результат
    // (a_high<<32 + a_low) * (b_high<<32 + b_low) =
    // = p4<<64 + (p3 + p2)<<32 + p1
    
    uint64_t mid = p2 + p3;
    uint64_t carry = (mid < p2) ? 1 : 0;  // переполнение при сложении
    
    low = p1 + (mid << 32);
    high = p4 + (mid >> 32) + carry;
    
    // Если low переполнился при сложении с p1
    if (low < p1) {
        high++;
    }
}

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

void D512::div512by10(uint64_t* words, uint64_t& remainder) {
    uint64_t carry = 0;
    
    for (int word = WORD_COUNT - 1; word >= 0; --word) {
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
    
    // Обработка знака
    size_t start = 0;
    if (str[0] == '-') {
        negative = true;
        start = 1;
        
        if (start >= str.length()) {
            isNaN = true;
            throw std::invalid_argument("D512: после минуса нет цифр");
        }
    }
    
    // Проверка на ведущий ноль
    if (str[start] == '0' && str.length() - start > 1) {
        isNaN = true;
        throw std::invalid_argument("D512: число не может начинаться с нуля");
    }
    
    // Валидация всех символов
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
    
    // Защита от отрицательного нуля (-0)
    if (str.substr(start) == "0") {
        negative = false;
        return;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        uint64_t digit{static_cast<uint64_t>(str[i] - '0')};
        uint64_t carry{digit};

        for (int word = 0; word < WORD_COUNT; ++word) {
            uint64_t low{}, high{};
            mul64by10(words[word], low, high);
            
            uint64_t sum = low + carry;
            bool overflow = (sum < low);
            
            words[word] = sum;
            carry = high + overflow;
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
        
        div512by10(temp.words, remainder);
        
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
    if (!negative) {
        uint64_t carry = 1;
        for (int i = 0; i < WORD_COUNT && carry; ++i) {
            uint64_t old = words[i];
            words[i] += carry;
            carry = (old > words[i]) ? 1 : 0;
        }
        if(carry) isNaN = true;
    } else {
        uint64_t borrow = 1;
        for (int i = 0; i < WORD_COUNT && borrow; ++i) {
            uint64_t old = words[i];
            words[i] -= borrow;
            borrow = (old < borrow) ? 1 : 0;
        }
        if (isZero()) negative = false;
    }

    return *this;
}

D512 D512::operator++(int) {
    D512 temp = *this;
    ++(*this);
    return temp;
}

D512& D512::operator--() {
    if (isNaN) {
        return *this;
    }

    if (negative) {
        uint64_t carry = 1;
        for (int i = 0; i < WORD_COUNT && carry; ++i) {
            uint64_t old = words[i];
            words[i] += carry;

            carry = (words[i] < old) ? 1 : 0;
        }
        if (carry) {
            isNaN = true;
        }
    } else {
        if (isZero()) {
            words[0] = 1;
            negative = true;
            return *this;
        }
        uint64_t borrow = 1;

        for (int i = 0; i < WORD_COUNT && borrow; ++i) {
            uint64_t old = words[i];
            words[i] -= borrow;
            borrow = (old < borrow) ? 1 : 0;
        }
        if (isZero()) {
            negative = false;
        }
    }

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
    if (isNaN || other.isNaN) {
        D512 result;
        result.isNaN = true;
        return result;
    }

    D512 result;
    result.negative = negative != other.negative;
        
    uint64_t temp[WORD_COUNT * 2] = {0};
        
    for (int i = 0; i < WORD_COUNT; ++i) {
        if (words[i] == 0) continue;
            
        uint64_t carry = 0;
        for (int j = 0; j < WORD_COUNT; ++j) {
            uint64_t low, high;
            mul64x64(words[i], other.words[j], low, high);
                    
            uint64_t sum = temp[i + j] + low + carry;
                    
            bool overflow1 = (sum < low);
            bool overflow2 = (sum < carry);
                
            temp[i + j] = sum;
            carry = high + (overflow1 ? 1 : 0) + (overflow2 ? 1 : 0);
        }
        if (carry) {
            temp[i + WORD_COUNT] += carry;
        }
    }
        
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = temp[i];
    }
        
    for (int i = WORD_COUNT; i < WORD_COUNT * 2; ++i) {
        if (temp[i] != 0) {
            result.isNaN = true;
            break;
        }
    }
        
    if (result.isZero()) {
        result.negative = false;
    }
        
    return result;
}

D512 D512::operator/(const D512& other) const {
    if (isNaN || other.isNaN || other.isZero()) {
        D512 result;
        result.isNaN = true;
        return result;
    }
    
    
    // Знак результата
    bool resultNegative = negative != other.negative;
    
    // Работаем с абсолютными значениями
    D512 dividend = *this;
    D512 divisor = other;
    dividend.negative = false;
    divisor.negative = false;
    
    // Если делимое меньше делителя — частное 0, остаток = dividend
    if (dividend < divisor) {
        D512 result;
        result.negative = resultNegative;
        return result;
    }
    
    // Подготавливаем массивы для работы
    uint64_t quotient[WORD_COUNT] = {0};
    uint64_t remainder[WORD_COUNT] = {0};
    
    // Проходим по битам от старшего к младшему
    for (int bit = WORD_BITS * WORD_COUNT - 1; bit >= 0; --bit) {
        // Сдвигаем остаток влево на 1 бит
        uint64_t carry = 0;
        for (int i = 0; i < WORD_COUNT; ++i) {
            uint64_t newCarry = remainder[i] >> 63;
            remainder[i] = (remainder[i] << 1) | carry;
            carry = newCarry;
        }
        
        // Добавляем текущий бит делимого в остаток
        int wordIdx = bit / WORD_BITS;
        int bitIdx = bit % WORD_BITS;
        if (dividend.words[wordIdx] & (1ULL << bitIdx)) {
            remainder[0] |= 1;
        }
        
        // Пробуем вычесть делитель
        if (compareArrays(remainder, divisor.words) >= 0) {
            // Вычитаем делитель из остатка
            uint64_t borrow = 0;
            for (int i = 0; i < WORD_COUNT; ++i) {
                uint64_t diff = remainder[i] - divisor.words[i] - borrow;
                borrow = (diff > remainder[i] || (borrow && diff == remainder[i])) ? 1 : 0;
                remainder[i] = diff;
            }
            
            // Устанавливаем бит в частном
            int qWordIdx = bit / WORD_BITS;
            int qBitIdx = bit % WORD_BITS;
            quotient[qWordIdx] |= (1ULL << qBitIdx);
        }
    }
    
    // Собираем результат
    D512 result;
    result.negative = resultNegative;
    for (int i = 0; i < WORD_COUNT; ++i) {
        result.words[i] = quotient[i];
    }
    
    if (result.isZero()) result.negative = false;
    
    return result;
}

D512 D512::operator%(const D512& other) const {
    if (isNaN || other.isNaN || other.isZero()) {
        D512 result;
        result.isNaN = true;
        return result;
    }
    
    // Берём частное и остаток через деление
    D512 quotient = *this / other;
    D512 product = quotient * other;
    D512 remainder = *this - product;
    
    // Корректируем знак остатка (должен быть как у делимого)
    if (!remainder.isZero() && negative != remainder.negative) {
        remainder = remainder + other;
    }
    
    return remainder;
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

bool D512::isOne() const noexcept {
    if (negative) return false;
    for (int i = 1; i < WORD_COUNT; ++i) {
        if (words[i] != 0) return false;
    }
    return words[0] == 1;
}