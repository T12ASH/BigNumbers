// main.cpp
#include "include/D512.hpp"
#include <iostream>
#include <cassert>



void testAdditionSubtraction() {
    std::cout << "\n=== Тест сложения и вычитания ===\n";
    
    D512 a(100);
    D512 b(200);
    D512 c(-50);
    D512 d(50);
    
    std::cout << "a = " << a << ", b = " << b << std::endl;
    std::cout << "a + b = " << (a + b) << " (ожидается 300)" << std::endl;
    std::cout << "a - b = " << (a - b) << " (ожидается -100)" << std::endl;
    
    std::cout << "\nc = " << c << ", d = " << d << std::endl;
    std::cout << "c + d = " << (c + d) << " (ожидается 0)" << std::endl;
    std::cout << "c - d = " << (c - d) << " (ожидается -100)" << std::endl;
    
    // Тест с большими числами
    D512 big1("10000000000000000000");
    D512 big2("1");
    std::cout << "\nbig1 = " << big1 << std::endl;
    std::cout << "-big1 + (-(-1)) = " << (-big1 + (-(-1))) << std::endl;
    std::cout << "big1 - 1 = " << (big1 - big2) << std::endl;
    
    // Тест +=
    D512 e(50);
    std::cout << "\ne = 50" << std::endl;
    e += D512(25);
    std::cout << "e += 25 -> " << e << " (ожидается 75)" << std::endl;
    e -= D512(100);
    std::cout << "e -= 100 -> " << e << " (ожидается -25)" << std::endl;
}

int main() {
    std::cout << "=== Библиотека BigIntegerLIB ===\n";
    
    try {
        testAdditionSubtraction();
        
        std::cout << "\n✅ Все тесты пройдены!\n";
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}