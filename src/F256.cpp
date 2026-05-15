#include "../include/F256.hpp"
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <iostream>

static const size_t MAX_DECIMAL_DIGITS = 77; // 256 бит ≈ 77 десятичных цифр

// ----------------------------------------------------------------------
// Конструкторы
// ----------------------------------------------------------------------
F256::F256() noexcept
    : mantissa(), exponent(0), negative(false), isNaN(false), isInf(false) {}

F256::F256(int64_t value) noexcept
    : mantissa(), exponent(0), negative(value < 0), isNaN(false), isInf(false)
{
    if (value >= 0) {
        mantissa = D256(value);
    } else {
        if (value == INT64_MIN) {
            mantissa = D256("9223372036854775808");
        } else {
            mantissa = D256(-value);
        }
    }

    if (mantissa.isZero()) {
        negative = false;
    }
}

F256::F256(double value) : exponent(0), negative(false), isNaN(false), isInf(false) {
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
    *this = F256(ss.str());
}

F256::F256(const D256& integer)
    : mantissa(integer), exponent(0), negative(integer.isNegative()), isNaN(false), isInf(false) {}

F256::F256(const std::string& str) : exponent(0), negative(false), isNaN(false), isInf(false) {
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
        mantissa = D256(0);
        exponent = 0;
        negative = false;
        return;
    }

    mantissa = D256(full);
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
void F256::normalize() {
    if (mantissa.isZero()) {
        exponent = 0;
        negative = false;
        return;
    }
    D256 ten(10);
    D256 zero(0);
    while ((mantissa % ten) == zero && mantissa != zero) {
        mantissa = mantissa / ten;
        exponent++;
    }
}

// ----------------------------------------------------------------------
// Вывод в строку
// ----------------------------------------------------------------------
std::string F256::toString() const {
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
bool F256::isZero() const { return mantissa.isZero() && !isNaN && !isInf; }

F256 F256::operator-() const {
    F256 res = *this;
    if (!res.isZero()) res.negative = !negative;
    return res;
}

F256 F256::operator+() const { return *this; }

// ----------------------------------------------------------------------
// Операторы сравнения
// ----------------------------------------------------------------------
bool F256::operator<(const F256& other) const {
    if (isNaN || other.isNaN) return false;
    if (isInf || other.isInf) {
        if (isInf && other.isInf) {
            if (negative == other.negative) return false;
            return negative;
        }
        if (isInf) return negative;
        if (other.isInf) return !other.negative;
    }
    if (isZero() && other.isZero()) return false;
    if (negative != other.negative) return negative;

    F256 a = *this;
    F256 b = other;

    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;
    D256 ten(10);

    if (!negative) {
        if (a.exponent > b.exponent) {
            int64_t diff = a.exponent - b.exponent;
            if (diff > MAX_ALIGN_DIFF) return false;
            for (int64_t i = 0; i < diff; ++i) {
                a.mantissa = a.mantissa * ten;
                if (a.mantissa.isNan()) return false;
            }
            a.exponent = b.exponent;
        } else if (b.exponent > a.exponent) {
            int64_t diff = b.exponent - a.exponent;
            if (diff > MAX_ALIGN_DIFF) return true;
            for (int64_t i = 0; i < diff; ++i) {
                b.mantissa = b.mantissa * ten;
                if (b.mantissa.isNan()) return true;
            }
            b.exponent = a.exponent;
        }
        return a.mantissa < b.mantissa;
    } else {
        if (a.exponent > b.exponent) {
            int64_t diff = a.exponent - b.exponent;
            if (diff > MAX_ALIGN_DIFF) return true;
            for (int64_t i = 0; i < diff; ++i) {
                a.mantissa = a.mantissa * ten;
                if (a.mantissa.isNan()) return true;
            }
            a.exponent = b.exponent;
        } else if (b.exponent > a.exponent) {
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

bool F256::operator>(const F256& other) const { return other < *this; }
bool F256::operator<=(const F256& other) const { return !(other < *this); }
bool F256::operator>=(const F256& other) const { return !(*this < other); }

bool F256::operator==(const F256& other) const {
    if (isNaN || other.isNaN) return false;
    if (isInf || other.isInf) return isInf == other.isInf && negative == other.negative;
    if (isZero() && other.isZero()) return true;
    if (negative != other.negative) return false;

    F256 a = *this;
    F256 b = other;

    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;
    D256 ten(10);

    if (a.exponent > b.exponent) {
        int64_t diff = a.exponent - b.exponent;
        if (diff > MAX_ALIGN_DIFF) return false;
        for (int64_t i = 0; i < diff; ++i) {
            a.mantissa = a.mantissa * ten;
            if (a.mantissa.isNan()) return false;
        }
        a.exponent = b.exponent;
    } else if (b.exponent > a.exponent) {
        int64_t diff = b.exponent - a.exponent;
        if (diff > MAX_ALIGN_DIFF) return false;
        for (int64_t i = 0; i < diff; ++i) {
            b.mantissa = b.mantissa * ten;
            if (b.mantissa.isNan()) return false;
        }
        b.exponent = a.exponent;
    }

    return a.mantissa == b.mantissa;
}

bool F256::operator!=(const F256& other) const { return !(*this == other); }

// ----------------------------------------------------------------------
// Арифметика
// ----------------------------------------------------------------------
F256 F256::operator+(const F256& other) const {
    if (isNaN || other.isNaN) return F256("NaN");
    if (isInf || other.isInf) {
        if (isInf && other.isInf && negative != other.negative) return F256("NaN");
        return isInf ? *this : other;
    }
    if (isZero()) return other;
    if (other.isZero()) return *this;

    F256 a = *this;
    F256 b = other;

    if (a.mantissa.isNegative()) a.mantissa = -a.mantissa;
    if (b.mantissa.isNegative()) b.mantissa = -b.mantissa;

    const int64_t MAX_ALIGN_DIFF = MAX_DECIMAL_DIGITS;
    D256 ten(10);

    if (a.exponent > b.exponent) {
        int64_t diff = a.exponent - b.exponent;
        if (diff > MAX_ALIGN_DIFF) return a;
        for (int64_t i = 0; i < diff; ++i) {
            a.mantissa = a.mantissa * ten;
            if (a.mantissa.isNan()) return F256("NaN");
        }
        a.exponent = b.exponent;
    } else if (b.exponent > a.exponent) {
        int64_t diff = b.exponent - a.exponent;
        if (diff > MAX_ALIGN_DIFF) return b;
        for (int64_t i = 0; i < diff; ++i) {
            b.mantissa = b.mantissa * ten;
            if (b.mantissa.isNan()) return F256("NaN");
        }
        b.exponent = a.exponent;
    }

    F256 res;
    res.exponent = a.exponent;
    res.isNaN = false;
    res.isInf = false;

    if (negative == other.negative) {
        res.mantissa = a.mantissa + b.mantissa;
        res.negative = negative;
        if (res.mantissa.isNan()) return F256("NaN");
        res.normalize();
        if (res.mantissa.isZero()) res.negative = false;
        return res;
    }

    if (a.mantissa == b.mantissa) return F256("0");
    if (a.mantissa > b.mantissa) {
        res.mantissa = a.mantissa - b.mantissa;
        res.negative = negative;
    } else {
        res.mantissa = b.mantissa - a.mantissa;
        res.negative = other.negative;
    }
    if (res.mantissa.isNan()) return F256("NaN");
    res.normalize();
    if (res.mantissa.isZero()) res.negative = false;
    return res;
}

F256 F256::operator-(const F256& other) const {
    if (isNaN || other.isNaN) return F256("NaN");
    if (isInf || other.isInf) {
        if (isInf && other.isInf) return F256("NaN");
        if (isInf) return *this;
        F256 res = other;
        res.negative = !res.negative;
        return res;
    }
    F256 b = other;
    if (!b.isZero()) b.negative = !b.negative;
    return *this + b;
}

F256 F256::operator*(const F256& other) const {
    if (isNaN || other.isNaN) return F256("NaN");
    if (isInf || other.isInf) {
        if (isZero() || other.isZero()) return F256("NaN");
        F256 res;
        res.isInf = true;
        res.negative = (negative != other.negative);
        return res;
    }
    F256 res;
    res.mantissa = mantissa * other.mantissa;
    if (res.mantissa.isNan()) return F256("NaN");
    res.exponent = exponent + other.exponent;
    res.negative = (negative != other.negative);
    res.normalize();
    if (res.mantissa.isZero()) res.negative = false;
    return res;
}

F256 F256::operator/(const F256& other) const {
    if (isNaN || other.isNaN || other.isZero()) return F256("NaN");
    if (isInf || other.isInf) {
        if (isInf && other.isInf) return F256("NaN");
        if (isInf) {
            F256 res;
            res.isInf = true;
            res.negative = (negative != other.negative);
            return res;
        }
        return F256("0");
    }
    if (isZero()) return F256("0");

    F256 res;
    const int64_t PREC = 40;
    D256 dividend = mantissa;
    D256 ten(10);
    for (int64_t i = 0; i < PREC; ++i) {
        dividend = dividend * ten;
        if (dividend.isNan()) return F256("NaN");
    }
    res.mantissa = dividend / other.mantissa;
    if (res.mantissa.isNan()) return F256("NaN");
    res.exponent = exponent - other.exponent - PREC;
    res.negative = (negative != other.negative);
    res.normalize();
    if (res.mantissa.isZero()) res.negative = false;
    return res;
}

// ----------------------------------------------------------------------
// Операторы присваивания
// ----------------------------------------------------------------------
F256& F256::operator+=(const F256& other) { *this = *this + other; return *this; }
F256& F256::operator-=(const F256& other) { *this = *this - other; return *this; }
F256& F256::operator*=(const F256& other) { *this = *this * other; return *this; }
F256& F256::operator/=(const F256& other) { *this = *this / other; return *this; }

// ----------------------------------------------------------------------
// Глобальные операторы ввода/вывода
// ----------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const F256& obj) {
    os << obj.toString();
    return os;
}

std::istream& operator>>(std::istream& is, F256& obj) {
    std::string str;
    is >> str;
    obj = F256(str);
    return is;
}

F256::operator double() const {
    if (isNaN || isInf) return 0.0;
    std::stringstream ss;
    ss << toString();
    double d;
    ss >> d;
    return d;
}