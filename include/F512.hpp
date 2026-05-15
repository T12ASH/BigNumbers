#ifndef F512_HPP
#define F512_HPP

#include "D512.hpp"
#include <iostream>
#include <string>

class F512 {
private:
    D512 mantissa;       // целое число, все значащие цифры
    int64_t exponent;    // порядок (показатель степени 10)
    bool negative;       // знак числа
    bool isNaN;          // не число
    bool isInf;          // бесконечность

    void normalize();    // нормализация (убирает хвостовые нули)

public:
    F512() noexcept;
    F512(int64_t value) noexcept;
    F512(double value);
    explicit F512(const std::string& str);
    explicit F512(const D512& integer);

    F512(const F512& other) noexcept = default;
    F512(F512&& other) noexcept = default;
    ~F512() noexcept = default;

    F512& operator=(const F512& other) noexcept = default;
    F512& operator=(F512&& other) noexcept = default;

    // Арифметика
    F512 operator+(const F512& other) const;
    F512 operator-(const F512& other) const;
    F512 operator*(const F512& other) const;
    F512 operator/(const F512& other) const;

    // Присваивание с операцией
    F512& operator+=(const F512& other);
    F512& operator-=(const F512& other);
    F512& operator*=(const F512& other);
    F512& operator/=(const F512& other);

    // Сравнение
    bool operator==(const F512& other) const;
    bool operator!=(const F512& other) const;
    bool operator<(const F512& other) const;
    bool operator>(const F512& other) const;
    bool operator<=(const F512& other) const;
    bool operator>=(const F512& other) const;

    // Унарные
    F512 operator-() const;
    F512 operator+() const;

    // Преобразования
    std::string toString() const;
    explicit operator double() const;

    // Проверки
    bool isZero() const;
    bool isNan() const { return isNaN; }
    bool isInfinity() const { return isInf; }

    // Статические методы
    static F512 zero() { return F512(); }
    static F512 one() { return F512("1"); }
    static F512 max() { return F512(D512::max()); }
    static F512 min() { return F512(D512::min()); }

    friend std::ostream& operator<<(std::ostream& os, const F512& obj);
    friend std::istream& operator>>(std::istream& is, F512& obj);
};

std::ostream& operator<<(std::ostream& os, const F512& obj);
std::istream& operator>>(std::istream& is, F512& obj);

#endif