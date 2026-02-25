// main.cpp
#include "include/D512.hpp"
#include <iostream>
#include <cassert>

void testConstructors() {
    std::cout << "\n=== Тест конструкторов ===\n";
    
    D512 a;
    D512 b(12345);
    D512 c("9876543210");
    D512 d("-42");
    
    std::cout << "a (default): " << a << std::endl;
    std::cout << "b (12345): " << b << std::endl;
    std::cout << "c (9876543210): " << c << std::endl;
    std::cout << "d (-42): " << d << std::endl;
    
    // Тест копирования
    D512 e = b;
    std::cout << "e (копия b): " << e << std::endl;
}

void testComparisons() {
    std::cout << "\n=== Тест сравнений ===\n";
    
    D512 a(100);
    D512 b(200);
    D512 c(100);
    D512 d(-50);
    
    std::cout << "a (100) < b (200): " << (a < b) << std::endl;
    std::cout << "a (100) == c (100): " << (a == c) << std::endl;
    std::cout << "d (-50) < a (100): " << (d < a) << std::endl;
}

void testHexOutput() {
    std::cout << "\n=== Тест HEX вывода ===\n";
    
    D512 a(0x1234567890ABCDEF);
    std::cout << "a = ";
    a.printHex(std::cout);
    std::cout << std::endl;
    
    D512 b = D512::max();
    std::cout << "max = ";
    b.printHex(std::cout);
    std::cout << std::endl;
}

int main() {
    std::cout << "=== Библиотека BigIntegerLIB ===\n";
    std::cout << "Тестирование класса D512 (512-битные целые)\n";
    
    try {
        testConstructors();
        testComparisons();
        testHexOutput();
        
        std::cout << "\n✅ Все тесты пройдены!\n";
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}