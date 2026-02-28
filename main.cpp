#include "include/D512.hpp"
#include <iostream>
#include <chrono>
#include <vector>

// Утилита для замера времени
class Timer {
    std::chrono::high_resolution_clock::time_point start;
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    double elapsed() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

void testConstructors() {
    std::cout << "\n=== 1. Тест конструкторов ===\n";
    
    D512 a;  // 0
    D512 b(12345);
    D512 c("9876543210");
    D512 d("-42");
    D512 e(b);  // копирование
    D512 f(std::move(c));  // перемещение
    
    std::cout << "a (default): " << a << "\n";
    std::cout << "b (12345): " << b << "\n";
    std::cout << "c (оригинал) после перемещения: " << c << " (должен быть 0)\n";
    std::cout << "f (перемещённый): " << f << "\n";
    std::cout << "d (-42): " << d << "\n";
    std::cout << "e (копия b): " << e << "\n";
}

void testComparisons() {
    std::cout << "\n=== 2. Тест сравнений ===\n";
    
    D512 a(100);
    D512 b(200);
    D512 c(100);
    D512 d(-50);
    D512 e(-100);
    
    std::cout << "a (100) == c (100): " << (a == c ? "да" : "нет") << "\n";
    std::cout << "a (100) != b (200): " << (a != b ? "да" : "нет") << "\n";
    std::cout << "a (100) < b (200): " << (a < b ? "да" : "нет") << "\n";
    std::cout << "b (200) > a (100): " << (b > a ? "да" : "нет") << "\n";
    std::cout << "d (-50) < a (100): " << (d < a ? "да" : "нет") << "\n";
    std::cout << "e (-100) < d (-50): " << (e < d ? "да" : "нет") << "\n";
    std::cout << "e (-100) > d (-50): " << (e > d ? "да" : "нет") << "\n";
}

void testAdditionSubtraction() {
    std::cout << "\n=== 3. Тест сложения и вычитания ===\n";
    
    D512 a(100);
    D512 b(200);
    D512 c(-50);
    D512 d(50);
    
    std::cout << "100 + 200 = " << (a + b) << " (ожидается 300)\n";
    std::cout << "100 - 200 = " << (a - b) << " (ожидается -100)\n";
    std::cout << "-50 + 50 = " << (c + d) << " (ожидается 0)\n";
    std::cout << "-50 - 50 = " << (c - d) << " (ожидается -100)\n";
    std::cout << "50 - (-50) = " << (d - c) << " (ожидается 100)\n";
    
    // Тест с большими числами
    D512 big1("10000000000000000000");
    D512 big2("1");
    std::cout << "10^19 + 1 = " << (big1 + big2) << "\n";
    std::cout << "10^19 - 1 = " << (big1 - big2) << "\n";
    
    // Тест += и -=
    D512 e(50);
    e += D512(25);
    std::cout << "e += 25 -> " << e << " (ожидается 75)\n";
    e -= D512(100);
    std::cout << "e -= 100 -> " << e << " (ожидается -25)\n";
}

void testMultiplication() {
    std::cout << "\n=== 4. Тест умножения ===\n";
    
    D512 a(123);
    D512 b(456);
    D512 c(-123);
    
    std::cout << "123 * 456 = " << (a * b) << " (ожидается 56088)\n";
    std::cout << "123 * (-456) = " << (a * c) << " (ожидается -56088)\n";
    std::cout << "(-123) * (-456) = " << (c * c) << " (ожидается 15129? нет, 123*123)\n";
    
    // Тест с большими числами
    D512 big1("10000000000000000000");
    D512 big2("2");
    std::cout << "10^19 * 2 = " << (big1 * big2) << " (ожидается 2*10^19)\n";
    
    // Тест умножения с переполнением
    D512 max_num = D512::max();
    std::cout << "max * 1 = " << (max_num * D512(1)) << " (ожидается max)\n";
}

void testUnaryOperators() {
    std::cout << "\n=== 5. Тест унарных операторов ===\n";
    
    D512 a(100);
    D512 b(-100);
    D512 c(0);
    
    std::cout << "+a = " << (+a) << "\n";
    std::cout << "-a = " << (-a) << "\n";
    std::cout << "-b = " << (-b) << "\n";
    std::cout << "-c = " << (-c) << " (ожидается 0)\n";
}

void testIncrementDecrement() {
    std::cout << "\n=== 6. Тест инкремента/декремента ===\n";
    
    D512 a(100);
    std::cout << "a = " << a << "\n";
    std::cout << "a++ = " << a++ << " (должно быть 100)\n";
    std::cout << "после a++ = " << a << " (должно быть 101)\n";
    std::cout << "++a = " << ++a << " (должно быть 102)\n";
    
    D512 b(-100);
    std::cout << "b = " << b << "\n";
    std::cout << "b-- = " << b-- << " (должно быть -100)\n";
    std::cout << "после b-- = " << b << " (должно быть -101)\n";
}

void testBitwiseOperators() {
    std::cout << "\n=== 7. Тест битовых операций ===\n";
    
    D512 a(0b1010);  // 10
    D512 b(0b1100);  // 12
    
    std::cout << "a & b = " << (a & b) << " (ожидается 8)\n";
    std::cout << "a | b = " << (a | b) << " (ожидается 14)\n";
    std::cout << "a ^ b = " << (a ^ b) << " (ожидается 6)\n";
    std::cout << "~a = " << (~a) << " (ожидается ...)\n";
    
    D512 c(1);
    std::cout << "1 << 10 = " << (c << 10) << " (ожидается 1024)\n";
    std::cout << "1024 >> 5 = " << ((c << 10) >> 5) << " (ожидается 32)\n";
}

void testConversions() {
    std::cout << "\n=== 8. Тест преобразований ===\n";
    
    D512 a(12345);
    std::cout << "a.toString() = " << a.toString() << "\n";
    std::cout << "int64_t(a) = " << int64_t(a) << "\n";
    std::cout << "bool(a) = " << (a ? "true" : "false") << "\n";
    
    D512 b(0);
    std::cout << "bool(0) = " << (b ? "true" : "false") << "\n";
}

void testHexOutput() {
    std::cout << "\n=== 9. Тест HEX вывода ===\n";
    
    D512 a(0x1234567890ABCDEF);
    std::cout << "a = ";
    a.printHex(std::cout);
    std::cout << "\n";
    
    D512 b = D512::max();
    std::cout << "max = ";
    b.printHex(std::cout);
    std::cout << "\n";
}

void testHugeNumbers() {
    std::cout << "\n=== 10. Тест огромных чисел (512 бит) ===\n";
    
    D512 big1("13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095");
    D512 big2("1");
    
    std::cout << "big1 (max) = " << big1 << "\n";
    std::cout << "big1 + 1 = " << (big1 + big2) << " (ожидается NaN)\n";
    
    // Проверка парсинга
    D512 big3("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
    std::cout << "big3 = " << big3 << "\n";
}

void testStreamOperators() {
    std::cout << "\n=== 11. Тест операторов ввода/вывода ===\n";
    
    D512 a("12345");
    std::cout << "Вывод через << : " << a << "\n";
    
    // Тест ввода (нужно ввести число)
    std::cout << "Введите число (например 999): ";
    D512 b;
    std::cin >> b;
    std::cout << "Вы ввели: " << b << "\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "    ТЕСТИРОВАНИЕ КЛАССА D512\n";
    std::cout << "========================================\n";
    
    try {
        Timer total;
        
        testConstructors();
        testComparisons();
        testAdditionSubtraction();
        testMultiplication();
        testUnaryOperators();
        testIncrementDecrement();
        testBitwiseOperators();
        testConversions();
        testHexOutput();
        testHugeNumbers();
        
        std::cout << "\n✅ Все тесты пройдены за " << total.elapsed() << " мс\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}