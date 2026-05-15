#ifndef F256_HPP
#define F256_HPP

#include "D256.hpp"
#include <iostream>
#include <string>

class F256 {
private:
    D256 mantissa;       // целое число, все значащие цифры
    int64_t exponent;    // порядок (показатель степени 10)
    bool negative;       // знак числа
    bool isNaN;          // не число
    bool isInf;          // бесконечность

    void normalize();    // нормализация (убирает хвостовые нули)

public:
    F256() noexcept;
    F256(int64_t value) noexcept;
    F256(double value);
    explicit F256(const std::string& str);
    explicit F256(const D256& integer);

    F256(const F256& other) noexcept = default;
    F256(F256&& other) noexcept = default;
    ~F256() noexcept = default;

    F256& operator=(const F256& other) noexcept = default;
    F256& operator=(F256&& other) noexcept = default;

    // Арифметика
    F256 operator+(const F256& other) const;
    F256 operator-(const F256& other) const;
    F256 operator*(const F256& other) const;
    F256 operator/(const F256& other) const;

    // Присваивание с операцией
    F256& operator+=(const F256& other);
    F256& operator-=(const F256& other);
    F256& operator*=(const F256& other);
    F256& operator/=(const F256& other);

    // Сравнение
    bool operator==(const F256& other) const;
    bool operator!=(const F256& other) const;
    bool operator<(const F256& other) const;
    bool operator>(const F256& other) const;
    bool operator<=(const F256& other) const;
    bool operator>=(const F256& other) const;

    // Унарные
    F256 operator-() const;
    F256 operator+() const;

    // Преобразования
    std::string toString() const;
    explicit operator double() const;

    // Проверки
    bool isZero() const;
    bool isNan() const { return isNaN; }
    bool isInfinity() const { return isInf; }

    // Статические методы
    static F256 zero() { return F256(); }
    static F256 one() { return F256("1"); }
    static F256 max() { return F256(D256::max()); }
    static F256 min() { return F256(D256::min()); }

    friend std::ostream& operator<<(std::ostream& os, const F256& obj);
    friend std::istream& operator>>(std::istream& is, F256& obj);
};

std::ostream& operator<<(std::ostream& os, const F256& obj);
std::istream& operator>>(std::istream& is, F256& obj);

#endif