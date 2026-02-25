// main.cpp
#include "include/D512.hpp"
#include <iostream>
#include <cassert>



int main() {
    std::cout << "=== Библиотека BigIntegerLIB ===\n";
    
    // Создаем 512-битное число
    D512 big_num("13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095");
    
    // Выводим всеми способами:
    std::cout << "\n1. Через operator<< (cout):\n";
    std::cout << big_num << std::endl;
    
    std::cout << "\n2. Через toString():\n";
    std::cout << big_num.toString() << std::endl;
    
    std::cout << "\n3. В 16-ричном виде:\n";
    big_num.printHex(std::cout);
    std::cout << std::endl;
    
    return 0;
}