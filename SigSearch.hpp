#ifndef SegSearch_HPP
#define SegSearch_HPP

#include <array>
#include <functional>
#include <iterator>
#include <ostream>
#include <string_view>
#include <type_traits>

#include <cassert>
#include <climits>
#include <iostream>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#define CPP_VERSION _MSVC_LANG
#elif defined(__cplusplus)
#define CPP_VERSION __cplusplus
#else
#error "A C++ compiler is required" 
#endif // CPP_VERSION

#if CPP_VERSION < 202002L  // c++20
#error "A C++20 or newer is required"
#endif

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ <= 8)
#define ALWAYS_INLINE_MID __attribute__((always_inline))
#elif defined(__GNUC__) && ( __GNUC__ > 9 || (__GNUC__ == 9 && __GNUC_MINOR__ >= 3))
#define ALWAYS_INLINE_PRE [[gnu::always_inline]]
#elif defined(_MSC_VER) && _MSC_VER >= 1927 && CPP_VERSION >= 202002L
#define ALWAYS_INLINE_POST [[msvc::forceinline]]
#endif

#ifndef ALWAYS_INLINE_PRE
#define ALWAYS_INLINE_PRE
#endif
#ifndef ALWAYS_INLINE_MID
#define ALWAYS_INLINE_MID
#endif
#ifndef ALWAYS_INLINE_POST
#define ALWAYS_INLINE_POST
#endif

namespace SigSearch
{
    template <typename Char, size_t N>
    struct StaticString
    {
        Char data[N] = { 0 };
        constexpr StaticString(const Char(&str)[N]) {
            std::copy(static_cast<const Char*>(str), str + N, data);
        }

        constexpr size_t size() const { return N - 1; }

        constexpr const Char operator[](size_t i) const { return data[i]; }
        constexpr Char operator[](size_t i) { return data[i]; }
    };

    //// just inline for loop with const size N
    template <size_t N>
    ALWAYS_INLINE_PRE ALWAYS_INLINE_MID ALWAYS_INLINE_POST constexpr inline static void Repeat(auto f) {
        [f] ALWAYS_INLINE_PRE <auto... Index>(std::index_sequence<Index...>)ALWAYS_INLINE_MID ALWAYS_INLINE_POST {
            (f. template operator() < Index > (), ...);
        }(std::make_index_sequence<N>());
    }

    consteval unsigned hex_char_to_val(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw "Invalid hex digit";
    }

    struct CmpOp {
        size_t offset;
        size_t size;
        uint64_t value;
    };

    template<size_t PlanSize>
    struct ComparisonSequence {
        std::array<CmpOp, PlanSize> ops{};
    };

    template <StaticString Src>
    struct CompileTimeSignature
    {

    private:
        constexpr static size_t SignatureLenght = []() consteval {
            size_t count = 0;
            for (size_t i = 0; i < Src.size();) {
                if (Src[i] == ' ') { ++i; continue; }
                ++count;
                if (Src[i] == '?') {
                    if (i + 1 < Src.size() && Src[i + 1] == '?') i += 2;
                    else ++i;
                }
                else {
                    i += 2;
                }
            }
            return count;
            }();

        constexpr static std::array<std::pair<char, bool>, SignatureLenght> MaskedSignatureBytes = []() consteval {
            std::array<std::pair<char, bool>, SignatureLenght> temp_sig;
            size_t sig_len = 0;
            for (size_t i = 0; i < Src.size();) {
                if (Src[i] == ' ') { ++i; continue; }

                if (Src[i] == '?') {
                    temp_sig[sig_len++] = { 0, true };
                    if (i + 1 < Src.size() && Src[i + 1] == '?') i += 2;
                    else ++i;
                }
                else {
                    unsigned high = hex_char_to_val(Src[i]);
                    unsigned low = hex_char_to_val(Src[i + 1]);
                    temp_sig[sig_len++] = { static_cast<char>((high << 4) | low), false };
                    i += 2;
                }
            }
            return temp_sig;
            }();

        constexpr static size_t opCount = []() consteval {
            size_t opId = 0;
            size_t current_offset = 0;

            while (current_offset < SignatureLenght) {
                if (MaskedSignatureBytes[current_offset].second) {
                    current_offset++;
                    continue;
                }

                auto can_read = [&](size_t size) {
                    if (current_offset + size > SignatureLenght) return false;
                    for (size_t i = 0; i < size; ++i) {
                        if (MaskedSignatureBytes[current_offset + i].second) return false;
                    }
                    return true;
                    };

                auto read_int = [&](size_t size) {
                    uint64_t val = 0;
                    for (size_t i = 0; i < size; ++i) {
                        val |= static_cast<uint64_t>(static_cast<unsigned char>(MaskedSignatureBytes[current_offset + i].first)) << (i * 8);
                    }
                    return val;
                    };

                if (can_read(8)) {
                    opId++;
                    current_offset += 8;
                }
                else if (can_read(4)) {
                    opId++;
                    current_offset += 4;
                }
                else if (can_read(2)) {
                    opId++;
                    current_offset += 2;
                }
                else if (can_read(1)) {
                    opId++;
                    current_offset += 1;
                }
                else {
                    throw std::exception();
                }
            }
            return opId;
            }();

        constexpr static ComparisonSequence<opCount> cmpSequence = []() consteval {
            std::array<CmpOp, opCount> sequenceOps{};
            size_t opId = 0;
            size_t current_offset = 0;

            auto can_read = [&](size_t size) {
                if (current_offset + size > SignatureLenght) return false;
                for (size_t i = 0; i < size; ++i) {
                    if (MaskedSignatureBytes[current_offset + i].second) return false;
                }
                return true;
                };

            auto read_int = [&](size_t size) {
                uint64_t val = 0;
                for (size_t i = 0; i < size; ++i) {
                    val |= static_cast<uint64_t>(static_cast<unsigned char>(MaskedSignatureBytes[current_offset + i].first)) << (i * 8);
                }
                return val;
                };

            while (current_offset < SignatureLenght) {
                if (MaskedSignatureBytes[current_offset].second) {
                    current_offset++;
                    continue;
                }

                if (can_read(8)) {
                    sequenceOps[opId++] = { current_offset, 8, read_int(8) };
                    current_offset += 8;
                }
                else if (can_read(4)) {
                    sequenceOps[opId++] = { current_offset, 4, read_int(4) };
                    current_offset += 4;
                }
                else if (can_read(2)) {
                    sequenceOps[opId++] = { current_offset, 2, read_int(2) };
                    current_offset += 2;
                }
                else if (can_read(1)) {
                    sequenceOps[opId++] = { current_offset, 1, read_int(1) };
                    current_offset += 1;
                }
                else {
                    throw std::exception();
                }
            }

            return ComparisonSequence<opCount>{ sequenceOps };
            }();

        template <const CmpOp& op>
        static inline bool do_compare(uintptr_t start) {
            const auto ptr = start + op.offset;

            if constexpr (op.size == 8) {
                return *reinterpret_cast<const uint64_t*>(ptr) == op.value;
            }
            else if constexpr (op.size == 4) {
                return *reinterpret_cast<const uint32_t*>(ptr) == static_cast<uint32_t>(op.value);
            }
            else if constexpr (op.size == 2) {
                return *reinterpret_cast<const uint16_t*>(ptr) == static_cast<uint16_t>(op.value);
            }
            else { // op.size == 1
                return *reinterpret_cast<const uint8_t*>(ptr) == static_cast<uint8_t>(op.value);
            }
        }

        template <size_t... Is>
        static inline bool MatchAt(uintptr_t start, const std::index_sequence<Is...>)
        {
            return (do_compare<cmpSequence.ops[Is]>(start) && ...);
        }

    public:

        static constexpr size_t size() noexcept { return SignatureLenght; }

        static inline bool MatchAt(uintptr_t start) {
            return MatchAt(start, std::make_index_sequence<opCount>());
        }
    };

    template <StaticString Src>
    struct Signature {
        inline bool MatchAt(uintptr_t start) const {
            return CompileTimeSignature<Src>::MatchAt(start);
        }
        const size_t size() const noexcept {
            return CompileTimeSignature<Src>::size();
        };
    };

    template <typename>
    struct is_signature : std::false_type {};

    template <StaticString Src>
    struct is_signature<Signature<Src>> : std::true_type {};

    // Finds single signature in [start,end) range with aligned address and returns found address
    // Found address can be zero if range does not have specified signature
    template <typename T>
    static uintptr_t FindSignatureInRangeAligned(uintptr_t start, uintptr_t end, size_t alignment, const T& sig) {
        static_assert(
            is_signature<T>::value,
            "sig must be of type Signature<...>"
            );
        if (start == 0 || end == 0 || end <= start || alignment == 0) {
            return 0;
        }
        for (uintptr_t i = start; i < end - sig.size(); i += alignment)
        {
            if (sig.MatchAt(i))
            {
                return i;
            }
        }

        return 0;
    }

    // Finds single signature in [start,end) range and returns found address
    // Found address can be zero if range does not have specified signature
    template <typename T>
    static uintptr_t FindSignatureInRange(uintptr_t start, uintptr_t end, const T& sig) {
        return FindSignatureInRangeAligned<T>(start, end, 1, sig);
    }

    // Finds all specified signatures in [start,end) range with aligned address and returns first found address
    // Found address can be zero if range does not have any specified signatures
    template <typename... Sigs>
    static uintptr_t FindAnyInRangeAligned(uintptr_t start, uintptr_t end, size_t alignment, const Sigs&... sigs) {
        static_assert(
            (is_signature<Sigs>::value && ...),
            "sigs must be of type Signature<...>"
            );
        if (start == 0 || end == 0 || end <= start || alignment == 0) {
            return 0;
        }

        for (uintptr_t addr = start; addr < end; addr += alignment) {
            if (((addr + sigs.size() <= end && sigs.MatchAt(addr)) || ...)) {
                return addr;
            }
        }
        return 0;
    }

    // Finds all specified signatures in [start,end) range and returns first found address
    // Found address can be zero if range does not have any specified signatures
    template <typename... Sigs>
    static uintptr_t FindAnyInRange(uintptr_t start, uintptr_t end, const Sigs&... sigs) {
        return FindAnyInRangeAligned<Sigs...>(start, end, 1, sigs...);
    }

    // Finds all specified signatures in [start,end) range with aligned address and returns an std::array with found addresses
    // One or more found address can be zero if range does not have this signature
    template <typename... Sigs>
    static auto FindAllInRangeAligned(uintptr_t start, uintptr_t end, size_t alignment, const Sigs&... sigs) -> std::array<uintptr_t, sizeof...(sigs)> {
        static_assert(
            (is_signature<Sigs>::value && ...),
            "sigs must be of type Signature<...>"
            );
        constexpr std::size_t size = sizeof...(sigs);

        std::array<uintptr_t, size> results{};
        if (start == 0 || end == 0 || end <= start || alignment == 0) {
            return results;
        }

        auto signatures = std::tie(sigs...);

        for (uintptr_t addr = start; addr < end; addr += alignment) {
            bool found_all = true;
            Repeat<size>(ALWAYS_INLINE_PRE[&]<size_t Index>() ALWAYS_INLINE_MID ALWAYS_INLINE_POST {
                if (results[Index] != 0) return;

                const auto& sig = std::get<Index>(signatures);

                if (addr + sig.size() > end) return;

                if (sig.MatchAt(addr))
                {
                    results[Index] = addr;
                }
                else
                {
                    found_all = false;
                }
            });

            if (found_all)
            {
                break;
            }
        }
        return results;
    }

    // Finds all specified signatures in [start,end) range and returns an std::array with found addresses
    // One or more found address can be zero if range does not have this signature
    template <typename... Sigs>
    static auto FindAllInRange(uintptr_t start, uintptr_t end, const Sigs&... sigs) -> std::array<uintptr_t, sizeof...(sigs)> {
        return FindAllInRangeAligned<Sigs...>(start, end, 1, sigs...);
    }

    inline namespace literals {
        template <StaticString Src>
        consteval auto operator""_sig() {
            return Signature<Src>();
        }
    }
}
#endif // SegSearch_HPP
