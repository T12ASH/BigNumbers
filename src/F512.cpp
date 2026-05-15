#include "../include/F512.hpp"
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <iostream>

static const size_t MAX_DECIMAL_DIGITS = 155; // Максимум цифр для 512 бит

// ----------------------------------------------------------------------
// Конструкторы
// ----------------------------------------------------------------------
F512::F512() noexcept
    : mantissa(), exponent(0), negative(false), isNaN(false), isInf(false) {}

F512::F512(int64_t value) noexcept
    : mantissa(), exponent(0), negative(value < 0), isNaN(false), isInf(false)
{
    if (value >= 0) {
        mantissa = D512(value);
    } else {
        if (value == INT64_MIN) {
            mantissa = D512("9223372036854775808");
        } else {
            mantissa = D512(-value);
        }
    }

    if (mantissa.isZero()) {
        negative = false;
    }
}

F512::F512(double value) : exponent(0), negative(false), isNaN(false), isInf(false) {
    if (std::isnan(value)) {
        isNaN = true;
        return;
    }

    if (std::isinf(value)) {
        isInf = true;
        negative = value < 0;
        return;
    }

    std::stringstream ss;
    ss << std::setprecision(17) << value;
    *this = F512(ss.str());
}


F512::F512(const D512& integer)
    : mantissa(integer), exponent(0), negative(integer.isNegative()), isNaN(false), isInf(false) {}

F512::F512(const std::string& str) : exponent(0), negative(false), isNaN(false), isInf(false) {
    if (str.empty() || str == "NaN") { isNaN = true; return; }
    if (str == "inf" || str == "+inf") { isInf = true; return; }
    if (str == "-inf") { isInf = true; negative = true; return; }

    size_t pos = 0;
    if (str[0] == '-') { negative = true; pos++; }
    else if (str[0] == '+') pos++;

    size_t dot = str.find('.', pos);
    size_t e = str.find('e', pos); 
    if (e == std::string::npos) e = str.find('E', pos);

    std::string int_part, frac_part, exp_part;
    int exp_sign = 1;

    if (dot != std::string::npos) {
        int_part = str.substr(pos, dot - pos);
        if (e != std::string::npos) {
            frac_part = str.substr(dot + 1, e - dot - 1);
            size_t estart = e + 1;
            if (estart < str.length() && str[estart] == '-') { exp_sign = -1; estart++; }
            else if (estart < str.length() && str[estart] == '+') estart++;
            exp_part = str.substr(estart);
        } else {
            frac_part = str.substr(dot + 1);
        }
    } else {
        if (e != std::string::npos) {
            int_part = str.substr(pos, e - pos);
            size_t estart = e + 1;
            if (estart < str.length() && str[estart] == '-') { exp_sign = -1; estart++; }
            else if (estart < str.length() && str[estart] == '+') estart++;
            exp_part = str.substr(estart);
        } else {
            int_part = str.substr(pos);
        }
        frac_part = "";
    }

    if (int_part.empty()) int_part = "0";

    std::string full = int_part + frac_part;
    
    // Удаляем ведущие нули
    size_t first = full.find_first_not_of('0');
    if (first != std::string::npos) {
        full = full.substr(first);
        if (int_part != "0") {
            exponent += static_cast<int64_t>(first);
        }
    } else {
        full = "0";
    }

    if (full.length() > MAX_DECIMAL_DIGITS) {
        int64_t cut_count = full.length() - MAX_DECIMAL_DIGITS;
        exponent += cut_count;
        full = full.substr(0, MAX_DECIMAL_DIGITS);
    }

    if (full == "0") {
        mantissa = D512(0);
        exponent = 0;
        negative = false;
        return;
    }

    mantissa = D512(full);
    exponent -= static_cast<int64_t>(frac_part.length());

    if (!exp_part.empty()) {
        int64_t exp_val = std::stoll(exp_part);
        exponent += (exp_sign == 1) ? exp_val : -exp_val;
    }
    normalize();
}

// ----------------------------------------------------------------------
// Нормализация 
// ----------------------------------------------------------------------
void F512::normalize() {
    if (mantissa.isZero()) {
        exponent = 0;
        negative = false;
        return;
    }
    D512 ten(10);
    D512 zero(0);
    while ((mantissa % ten) == zero && mantissa != zero) {
        mantissa = mantissa / ten;
        exponent++;
    }
}

// ----------------------------------------------------------------------
// Вывод в строку
// ----------------------------------------------------------------------
std::string F512::toString() const {
    if (isNaN) return "NaN";
    if (isInf) return negative ? "-inf" : "inf";
    if (mantissa.isZero()) return "0";

    std::string s = mantissa.toString();
    int64_t exp = exponent;

    while (s.length() > 1 && s.back() == '0') {
        s.pop_back();
        exp++;
    }

    if (exp >= 0) {
        s.append(exp, '0');
        return negative ? "-" + s : s;
    }

    int64_t point_pos = static_cast<int64_t>(s.length()) + exp;
    if (point_pos <= 0) {
        std::string res = "0." + std::string(-point_pos, '0') + s;
        return negative ? "-" + res : res;
    } else {
        std::string res = s.substr(0, point_pos) + "." + s.substr(point_pos);
        return negative ? "-" + res : res;
    }
}

// ----------------------------------------------------------------------
// Унарные операторы и прочее
// ----------------------------------------------------------------------
bool F512::isZero() const { return mantissa.isZero() && !isNaN && !isInf; }

F512 F512::operator-() const {
    F512 res = *this;
    if (!res.isZero()) res.negative = !negative;
    return res;
}

F512 F512::operator+() const { return *this; }

// ----------------------------------------------------------------------
// Операторы сравнения
// ----------------------------------------------------------------------
bool F512::operator<(const F512& other) const {
    if (isNaN || other.isNaN) {
        return false;
    }

    // infinities
    if (isInf || other.isInf) {
        if (isInf && other.isInf) {
            if (negative == other.negative) return false;
            return negative;
        }
        if (isInf) return negative;          // -inf < everything, +inf < nothing
        if (other.isInf) return !other.negative; // everything < +inf, nothing < -inf
    }

    // zeros
    if (isZero() && other.isZero()) {
        return false;
    }

    // different signs
    if (negative != other.negative) {
        return negative;
    }

    F512 a = *this;
    F512 b = other;

    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;
    D512 ten(10);

    if (!negative) {
        // both positive
        if (a.exponent > b.exponent) {
            int64_t diff = a.exponent - b.exponent;
            if (diff > MAX_ALIGN_DIFF) return false;
            for (int64_t i = 0; i < diff; ++i) {
                a.mantissa = a.mantissa * ten;
                if (a.mantissa.isNan()) return false;
            }
            a.exponent = b.exponent;
        }
        else if (b.exponent > a.exponent) {
            int64_t diff = b.exponent - a.exponent;
            if (diff > MAX_ALIGN_DIFF) return true;
            for (int64_t i = 0; i < diff; ++i) {
                b.mantissa = b.mantissa * ten;
                if (b.mantissa.isNan()) return true;
            }
            b.exponent = a.exponent;
        }

        return a.mantissa < b.mantissa;
    }
    else {
        // both negative: larger magnitude means smaller number
        if (a.exponent > b.exponent) {
            int64_t diff = a.exponent - b.exponent;
            if (diff > MAX_ALIGN_DIFF) return true;
            for (int64_t i = 0; i < diff; ++i) {
                a.mantissa = a.mantissa * ten;
                if (a.mantissa.isNan()) return true;
            }
            a.exponent = b.exponent;
        }
        else if (b.exponent > a.exponent) {
            int64_t diff = b.exponent - a.exponent;
            if (diff > MAX_ALIGN_DIFF) return false;
            for (int64_t i = 0; i < diff; ++i) {
                b.mantissa = b.mantissa * ten;
                if (b.mantissa.isNan()) return false;
            }
            b.exponent = a.exponent;
        }

        return a.mantissa > b.mantissa;
    }
}


bool F512::operator>(const F512& other) const { return other < *this; }
bool F512::operator<=(const F512& other) const { return !(other < *this); }
bool F512::operator>=(const F512& other) const { return !(*this < other); }

bool F512::operator==(const F512& other) const {
    if (isNaN || other.isNaN) {
        return false;
    }

    if (isInf || other.isInf) {
        return isInf == other.isInf && negative == other.negative;
    }

    if (isZero() && other.isZero()) {
        return true;
    }

    if (negative != other.negative) {
        return false;
    }

    F512 a = *this;
    F512 b = other;

    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;
    D512 ten(10);

    if (a.exponent > b.exponent) {
        int64_t diff = a.exponent - b.exponent;

        if (diff > MAX_ALIGN_DIFF) {
            return false;
        }

        for (int64_t i = 0; i < diff; ++i) {
            a.mantissa = a.mantissa * ten;
            if (a.mantissa.isNan()) {
                return false;
            }
        }
        a.exponent = b.exponent;
    }
    else if (b.exponent > a.exponent) {
        int64_t diff = b.exponent - a.exponent;

        if (diff > MAX_ALIGN_DIFF) {
            return false;
        }

        for (int64_t i = 0; i < diff; ++i) {
            b.mantissa = b.mantissa * ten;
            if (b.mantissa.isNan()) {
                return false;
            }
        }
        b.exponent = a.exponent;
    }

    return a.mantissa == b.mantissa;
}


bool F512::operator!=(const F512& other) const { return !(*this == other); }

// ----------------------------------------------------------------------
// Арифметика
// ----------------------------------------------------------------------
F512 F512::operator+(const F512& other) const {
    if (isNaN || other.isNaN) {
        return F512("NaN");
    }

    if (isInf || other.isInf) {
        if (isInf && other.isInf) {
            if (negative != other.negative) {
                return F512("NaN");
            }
            return *this;
        }
        return isInf ? *this : other;
    }

    // Если одно из чисел ноль
    if (isZero()) return other;
    if (other.isZero()) return *this;

    F512 a = *this;
    F512 b = other;

    // Мантиссы должны быть положительными, знак отдельно
    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    // Максимальная разумная разница порядков:
    // если больше — меньшее число уже не влияет на результат
    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;

    // ------------------------------------------------------
    // Выравнивание порядков
    // ------------------------------------------------------
    if (a.exponent > b.exponent) {
        int64_t diff = a.exponent - b.exponent;

        // b намного меньше a, теряется в точности
        if (diff > MAX_ALIGN_DIFF) {
            return a;
        }

        D512 ten(10);
        for (int64_t i = 0; i < diff; ++i) {
            a.mantissa = a.mantissa * ten;
            if (a.mantissa.isNan()) {
                return F512("NaN");
            }
        }
        a.exponent = b.exponent;
    }
    else if (b.exponent > a.exponent) {
        int64_t diff = b.exponent - a.exponent;

        // a намного меньше b, теряется в точности
        if (diff > MAX_ALIGN_DIFF) {
            return b;
        }

        D512 ten(10);
        for (int64_t i = 0; i < diff; ++i) {
            b.mantissa = b.mantissa * ten;
            if (b.mantissa.isNan()) {
                return F512("NaN");
            }
        }
        b.exponent = a.exponent;
    }

    F512 res;
    res.exponent = a.exponent;
    res.isNaN = false;
    res.isInf = false;

    // Одинаковые знаки → обычное сложение
    if (negative == other.negative) {
        res.mantissa = a.mantissa + b.mantissa;
        res.negative = negative;

        if (res.mantissa.isNan()) {
            return F512("NaN");
        }

        res.normalize();

        if (res.mantissa.isZero()) {
            res.negative = false;
        }

        return res;
    }

    // Разные знаки → вычитание модулей
    if (a.mantissa == b.mantissa) {
        return F512("0");
    }

    if (a.mantissa > b.mantissa) {
        res.mantissa = a.mantissa - b.mantissa;
        res.negative = negative;
    } else {
        res.mantissa = b.mantissa - a.mantissa;
        res.negative = other.negative;
    }

    if (res.mantissa.isNan()) {
        return F512("NaN");
    }

    res.normalize();

    if (res.mantissa.isZero()) {
        res.negative = false;
    }

    return res;
}

F512 F512::operator-(const F512& other) const {
    if (isNaN || other.isNaN) {
        return F512("NaN");
    }

    if (isInf || other.isInf) {
        if (isInf && other.isInf) {
            return F512("NaN");
        }

        if (isInf) {
            return *this;
        }

        F512 res = other;
        res.negative = !res.negative;
        return res;
    }

    F512 b = other;

    if (!b.isZero()) {
        b.negative = !b.negative;
    }

    return *this + b;
}


F512 F512::operator*(const F512& other) const {
    if (isNaN || other.isNaN) {
        return F512("NaN");
    }

    if (isInf || other.isInf) {
        if (isZero() || other.isZero()) {
            return F512("NaN");
        }

        F512 res;
        res.isInf = true;
        res.negative = (negative != other.negative);
        return res;
    }

    F512 res;
    res.mantissa = mantissa * other.mantissa;

    if (res.mantissa.isNan()) {
        return F512("NaN");
    }

    res.exponent = exponent + other.exponent;
    res.negative = (negative != other.negative);

    res.normalize();

    if (res.mantissa.isZero()) {
        res.negative = false;
    }

    return res;
}


F512 F512::operator/(const F512& other) const {
    if (isNaN || other.isNaN || other.isZero()) {
        return F512("NaN");
    }

    if (isInf || other.isInf) {
        if (isInf && other.isInf) {
            return F512("NaN");
        }

        if (isInf) {
            F512 res;
            res.isInf = true;
            res.negative = (negative != other.negative);
            return res;
        }

        return F512("0");
    }

    if (isZero()) {
        return F512("0");
    }

    F512 res;
    const int64_t PREC = 40;

    D512 dividend = mantissa;
    D512 ten(10);

    for (int64_t i = 0; i < PREC; ++i) {
        dividend = dividend * ten;
        if (dividend.isNan()) {
            return F512("NaN");
        }
    }

    res.mantissa = dividend / other.mantissa;

    if (res.mantissa.isNan()) {
        return F512("NaN");
    }

    res.exponent = exponent - other.exponent - PREC;
    res.negative = (negative != other.negative);

    res.normalize();

    if (res.mantissa.isZero()) {
        res.negative = false;
    }

    return res;
}


// ----------------------------------------------------------------------
// Операторы присваивания
// ----------------------------------------------------------------------
F512& F512::operator+=(const F512& other) { *this = *this + other; return *this; }
F512& F512::operator-=(const F512& other) { *this = *this - other; return *this; }
F512& F512::operator*=(const F512& other) { *this = *this * other; return *this; }
F512& F512::operator/=(const F512& other) { *this = *this / other; return *this; }

// ----------------------------------------------------------------------
// Глобальные операторы ввода/вывода
// ----------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const F512& obj) {
    os << obj.toString();
    return os;
}

std::istream& operator>>(std::istream& is, F512& obj) {
    std::string str;
    is >> str;
    obj = F512(str);
    return is;
}

F512::operator double() const {
    if (isNaN || isInf) return 0.0;
    std::stringstream ss; 
    ss << toString(); 
    double d; 
    ss >> d;
    return d;
}
