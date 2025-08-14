# SigSearch — README

## English

### What this is
A small, header-only C++ utility for compile-time-friendly byte-signature matching. It parses signatures written as hex strings with optional wildcards (like `"48 8B ?? 89"`) at compile time and produces a lightweight `Signature` object that can be matched against memory ranges at runtime.

This is not a framework — just a single header that implements:
- a tiny `fixed_string`-like compile-time string (taken/ported from existing ideas),
- a compile-time signature parser that turns a textual pattern into a byte array and a mask,
- helper functions to scan address ranges: `FindSignatureInRange`, `FindAnyInRange`, and `FindAllInRange`,
- a user literal `"..."_sig` so you can write signatures inline.

For example
```cpp
using namespace SigSearch::literals;
static char sig[] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10 };
int main()
{
    printf("%llx", SigSearch::FindSignatureInRange((uintptr_t)sig, (uintptr_t)sig+sizeof(sig), "03 ?? 05"_sig));
}
```
this can be compiled into
```cpp
  char *v3; // rdx

  v3 = sig;
  while ( *v3 != 3 || v3[2] != 5 )
  {
    if ( ++v3 >= &sig[7] )
    {
      v3 = 0;
      break;
    }
  }
  printf("%llx", v3);
```
when decompiling in IDA

### Requirements
- C++20 or newer. If you compile with C++23 and the standard `std::bitset` is available, the header uses it; otherwise it contains a lightweight bitset implementation.

### Quick usage examples
```cpp
#include "SigSearch.hpp"
using namespace SigSearch::literals;

int main() {
    // Single signature search
    auto sig = "48 8B ?? 89"_sig;
    uintptr_t found = SigSearch::FindSignatureInRange(startAddr, endAddr, sig);

    // Multiple signature search
    auto sig1 = "48 8B ?? 89"_sig;
    auto sig2 = "90 90 90"_sig;
    uintptr_t foundAny = SigSearch::FindAnyInRange(startAddr, endAddr, sig1, sig2);

    // Find all matches for one or more signatures
    auto results = SigSearch::FindAllInRange(startAddr, endAddr, sig1);
    for (auto addr : results) {
        if (addr) {
            // Do something with match
        }
    }
}
```

### API (short)
- `Signature<"..">` / `".."_sig` — compile-time signature object.
- `Signature::MatchAt(uintptr_t addr)` — check if signature matches at address.
- `FindSignatureInRange(start,end,sig)` — returns first address or 0.
- `FindAnyInRange(start,end,sig1,sig2,...)` — returns first address where any of signatures match.
- `FindAllInRange(start,end,sig1,sig2,...)` — returns `std::array<uintptr_t, N>` with found addresses (0 if not found).

### Notes & limitations
- The header works on raw memory reads; ensure you have permission to read the memory range.
- Byte order must match memory layout.
- Requires a modern compiler.

### Attribution
Uses concepts from:
- https://github.com/neargye-wg21/bitset-constexpr-proposal
- https://github.com/unterumarmung/fixed_string

---

## Русский

### Что это такое
Небольшая заголовочная библиотека на C++, которая позволяет описывать сигнатуры в виде hex-строк с `?`-джокером и получать объект сигнатуры на этапе компиляции, а затем искать совпадения в памяти во время выполнения.

К примеру
```cpp
using namespace SigSearch::literals;
static char sig[] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10 };
int main()
{
    printf("%llx", SigSearch::FindSignatureInRange((uintptr_t)sig, (uintptr_t)sig+sizeof(sig), "03 ?? 05"_sig));
}
```
это скомпилируется в такой код
```cpp
  char *v3; // rdx

  v3 = sig;
  while ( *v3 != 3 || v3[2] != 5 )
  {
    if ( ++v3 >= &sig[7] )
    {
      v3 = 0;
      break;
    }
  }
  printf("%llx", v3);
```
при декомпиляции в IDA

### Требования
- Компилятор с поддержкой C++20 или новее.

### Примеры использования
```cpp
#include "SigSearch.hpp"
using namespace SigSearch::literals;

int main() {
    // Поиск одной сигнатуры
    auto sig = "48 8B ?? 89"_sig;
    uintptr_t found = SigSearch::FindSignatureInRange(startAddr, endAddr, sig);

    // Поиск любой из нескольких сигнатур
    auto sig1 = "48 8B ?? 89"_sig;
    auto sig2 = "90 90 90"_sig;
    uintptr_t foundAny = SigSearch::FindAnyInRange(startAddr, endAddr, sig1, sig2);

    // Поиск всех совпадений для одной сигнатуры
    auto results = SigSearch::FindAllInRange(startAddr, endAddr, sig1);
    for (auto addr : results) {
        if (addr) {
            // Обработка найденного адреса
        }
    }
}
```

### API
- `Signature<"..">` / `".."_sig` — сигнатура, создаваемая на этапе компиляции.
- `Signature::MatchAt(uintptr_t addr)` — проверить совпадение по адресу.
- `FindSignatureInRange(uintptr_t start,uintptr_t end, sig)` — первый адрес совпадения или 0.
- `FindAnyInRange(uintptr_t start,uintptr_t end, sig1,sig2,...)` — первый адрес совпадения для любой сигнатуры.
- `FindAllInRange(uintptr_t start,uintptr_t end, sig1,sig2,...)` — массив адресов совпадений.

### Ограничения
- Чтение памяти напрямую, требуется доступ к диапазону.
- Байты в сигнатуре должны соответствовать порядку в памяти.
- Требуется современный компилятор.

### Авторство
Использованы идеи из:
- https://github.com/neargye-wg21/bitset-constexpr-proposal
- https://github.com/unterumarmung/fixed_string
