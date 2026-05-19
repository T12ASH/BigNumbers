# ПРОСТО КУРСОВАЯ РАБОТА

# BigIntegerLIB

> C++ библиотека для работы с большими числами  
> Поддержка 256/512-битных целых и вещественных чисел без внешних зависимостей.

---

# 📌 Возможности

## 🔢 Целые числа (`D256`, `D512`)
- 256 и 512 бит
- Знаковые числа
- Полный набор арифметических операций:
  - `+`
  - `-`
  - `*`
  - `/`
  - `%`
- Побитовые операции:
  - `&`
  - `|`
  - `^`
  - `~`
  - `<<`
  - `>>`
- Операторы сравнения:
  - `==`
  - `!=`
  - `<`
  - `>`
  - `<=`
  - `>=`
- Инкремент и декремент:
  - `++`
  - `--`

---

## 🌐 Системы счисления
- Парсинг строк в системах счисления от **2 до 36**
- Конвертация в любую систему счисления от **2 до 36**
- Поддержка:
  - Binary
  - Octal
  - Decimal
  - Hexadecimal
  - Base36

---

## 📈 Вещественные числа (`F256`, `F512`)
- Высокая точность
- Мантисса + порядок
- Фиксированная десятичная точка
- Поддержка:
  - сложения
  - вычитания
  - умножения
  - деления
  - преобразования в строку

---

## 🧩 Дополнительно
- Пользовательские литералы (`_N`)
- Полностью ручная реализация
- Без использования:
  - GMP
  - OpenSSL
  - Boost.Multiprecision
- Portable C++17

---

# 📂 Структура проекта

```text
BigIntegerLIB/
├── include/
│   ├── D256.hpp
│   ├── D512.hpp
│   ├── F256.hpp
│   └── F512.hpp
├── src/
│   ├── D256.cpp
│   ├── D512.cpp
│   ├── F256.cpp
│   └── F512.cpp
├── CMakeLists.txt
└── README.md
```

---

# 🔧 Сборка

## Требования
- C++17 и выше совместимый компилятор
  - MSVC 2022
  - GCC 9+
  - Clang
- CMake 3.10+

---

## Windows (Visual Studio)

```bash
mkdir build
cd build

cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

---

## Linux / macOS

```bash
mkdir build
cd build

cmake .. -G "Unix Makefiles"
cmake --build .
```

---

# 📖 Примеры использования

## Создание чисел

```cpp
#include "include/D512.hpp"

D512 a;                         // 0
D512 b(12345);                  // из int64
D512 c("12345678901234567890"); // из строки
D512 d("FF", 16);               // HEX
D512 e("ZOV", 36);              // Base36

D512 f = "12345"_N;             // пользовательский литерал
```

---

## Арифметика

```cpp
D512 x(100);
D512 y(200);

std::cout << x + y << "\n";
std::cout << x - y << "\n";
std::cout << x * y << "\n";
std::cout << x / y << "\n";
std::cout << x % y << "\n";

x += y;
y *= x;
```

---

## Сравнение

```cpp
if (x < y)
{
    // ...
}

if (x == D512(100))
{
    // ...
}
```

---

## Побитовые операции

```cpp
D512 mask(0xFF);

D512 shifted = mask << 8;
D512 anded = mask & D512(0x0F);
```

---

## Системы счисления

```cpp
D512 num("ZOVZOV", 36);

std::cout << num.toBase(36) << "\n";
std::cout << num.toBase(10) << "\n";
std::cout << num.toBase(16) << "\n";

std::cout << num.toString() << "\n";
```

---

## Вещественные числа

```cpp
#include "include/F512.hpp"

F512 pi("3.14159265358979323846");
F512 e("2.718281828459045");

std::cout << pi + e << "\n";
std::cout << pi.toString() << "\n";
```

---

# 🧠 Внутреннее устройство

## Хранение числа (`D512`)

```cpp
uint64_t words[8];
bool negative;
bool isNaN;
```

- `words` — массив из 8 × 64 бит
- `negative` — знак числа
- `isNaN` — ошибка/переполнение

---

## Парсинг строки

```text
result = 0

for each character:
    digit = value(character)
    result = result * base + digit
```

---

## Конвертация в строку

```text
while number != 0:
    remainder = number % base
    number = number / base
    append digit(remainder)

reverse string
```

---

## Сложение с переносом

```text
carry = 0

for i = 0..7:
    sum = a[i] + b[i] + carry
    carry = overflow(sum)
```

---

## Умножение

```text
64 × 64 → 128 бит

mul64x64(a, b, low, high)

дальше выполняется сложение
с учётом переносов
```

---

# 🧪 Тестирование

Библиотека протестирована на:

- Граничных значениях
  - `0`
  - `1`
  - `MAX`
  - `MIN`
- Отрицательных числах
- Переносах при сложении
- Переполнении при умножении
- Делении на ноль
- Конвертации систем счисления
- Парсинге Base2–Base36
- Вещественных числах
- Нормализации
- `toString()`

---

# 🚀 Пример

```cpp
#include <iostream>
#include "include/D512.hpp"

int main()
{
    D512 a("FFFFFFFFFFFFFFFF", 16);
    D512 b(2);

    D512 result = a * b;

    std::cout << result.toBase(16) << std::endl;

    return 0;
}
```

---