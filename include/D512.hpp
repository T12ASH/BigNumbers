//include/D512.hpp
#ifndef D512_HPP
#define D512_HPP

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

/**
 * @class D512
 * @brief Класс для работы с 512-битными целыми числами
 * 
 * Хранение: массив из 8 64-битных слов
 * words[0] - младшие 64 бита
 * words[7] - старшие 64 бита
 */
class D512 {
    static constexpr int WORD_BITS{64};              // Бит в одном слове
    static constexpr int WORD_COUNT{8};              // 8 * 64 = 512 бит
    static constexpr int BYTE_COUNT{64};             // 512 / 8 = 64 байта

    uint64_t words[WORD_COUNT]{0,0,0,0,0,0,0,0};     // 8 слов по 64 бита
    bool negative{false};                            // Флаг знака (true - отрицательное)
    bool isNaN{false};                               // Флаг ошибки (true - не число)
public:

    // Конструкторы ________________________________________
    /**
     * @brief Конструктор по умолчанию (число 0)
     */
    D512() noexcept = default;
    
    /**
     * @brief Конструктор от 64-битного целого
     * @param value целое число
     */
    D512(int64_t value) noexcept;
    
    /**
     * @brief Конструктор от строки
     * @param str строка вида "12345", "-6789" и т.д.
     * @throw std::invalid_argument если строка некорректна
     */
    explicit D512(const std::string& str);
    
    /**
     * @brief Конструктор копирования
     * Сложность O(n^2) - ДОРАБОТАТЬ
     * Память    O(n)
     */
    D512(const D512& other) noexcept;
    
    /**
     * @brief Конструктор перемещения
     */
    D512(D512&& other) noexcept;

    // Деструктор _________________________________________
    ~D512() noexcept;

    // Операторы присваивания _____________________________
    D512& operator=(const D512& other) noexcept = default;
    D512& operator=(D512&& other) noexcept = default;
    
    // Арифметические операторы ___________________________
    D512 operator+(const D512& other) const;
    D512 operator-(const D512& other) const;
    D512 operator*(const D512& other) const;
    D512 operator/(const D512& other) const;
    D512 operator%(const D512& other) const;
    
    // Операторы с присваиванием __________________________
    D512& operator+=(const D512& other);
    D512& operator-=(const D512& other);
    D512& operator*=(const D512& other);
    D512& operator/=(const D512& other);
    D512& operator%=(const D512& other);
    
    // Операторы сравнения ________________________________
    bool operator==(const D512& other) const noexcept;
    bool operator!=(const D512& other) const noexcept;
    bool operator<(const D512& other) const noexcept;
    bool operator>(const D512& other) const noexcept;
    bool operator<=(const D512& other) const noexcept;
    bool operator>=(const D512& other) const noexcept;

    // Унарные операторы __________________________________
    D512 operator-() const noexcept;
    D512 operator+() const noexcept;
    
    // Инкремент/декремент ________________________________
    D512& operator++();          // Префиксный
    D512 operator++(int);        // Постфиксный
    D512& operator--();
    D512 operator--(int);
    
    // Битовые операторы __________________________________
    D512 operator&(const D512& other) const noexcept;
    D512 operator|(const D512& other) const noexcept;
    D512 operator^(const D512& other) const noexcept;
    D512 operator~() const noexcept;
    D512 operator<<(int shift) const noexcept;
    D512 operator>>(int shift) const noexcept;
    
    // Проверки состояния _________________________________
    bool isZero() const noexcept;
    bool isNegative() const noexcept { return negative; }
    bool isPositive() const noexcept { return !negative && !isZero(); }
    bool isNan() const noexcept { return isNaN; }
    
    // Преобразования _____________________________________
    std::string toString() const;
    explicit operator bool() const noexcept { return !isZero(); }
    explicit operator int64_t() const noexcept;  // Обрезка до 64 бит
    
    // Вспомогательные методы _____________________________
    void printHex(std::ostream& os) const;       // Для отладки
    size_t hash() const noexcept;                // Для использования в хеш-таблицах
    
    // Статические методы _________________________________
    static D512 zero() noexcept { return D512(); }
    static D512 one() noexcept { return D512(1); }
    static D512 max() noexcept;                   // Максимальное значение
    static D512 min() noexcept;                   // Минимальное значение
    
    // Дружественные функции ______________________________
    friend std::ostream& operator<<(std::ostream& os, const D512& obj);
    friend std::istream& operator>>(std::istream& is, D512& obj);
    
    // Для C++/CLI ________________________________________
    const uint64_t* getWords() const noexcept { return words; }
    int getWordCount() const noexcept { return WORD_COUNT; }

    friend std::ostream& operator<<(std::ostream& os, const D512& obj);
    friend std::istream& operator>>(std::istream& is, D512& obj);
};
// Глобальные операторы ___________________________________
std::ostream& operator<<(std::ostream& os, const D512& obj);
std::istream& operator>>(std::istream& is, D512& obj);

namespace std {
    template<>
    struct hash<D512> {
        size_t operator()(const D512& d) const noexcept {
            return d.hash();
        }
    };
}
#endif