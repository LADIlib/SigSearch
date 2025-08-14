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

#if CPP_VERSION >= 202302L  // c++23
#include <bitset>
template<size_t _Bits>
using _BitSetClass = std::bitset<_Bits>;
#elif CPP_VERSION >= 202002L // c++20
#include <bit>
namespace nstd {

    template <size_t _Bits>
    class bitset {  // store fixed-length sequence of Boolean elements
    public:
#pragma warning(push)
#pragma warning(disable : 4296)  // expression is always true (/Wall)
        using _Ty = std::conditional_t<_Bits <= sizeof(unsigned long) * CHAR_BIT, unsigned long, unsigned long long>;
#pragma warning(pop)

        class reference {  // proxy for an element
            friend bitset<_Bits>;

        public:
            constexpr ~reference() noexcept {}  // TRANSITION, ABI

            constexpr reference& operator=(bool _Val) noexcept {
                _Pbitset->_Set_unchecked(_Mypos, _Val);
                return *this;
            }

            constexpr reference& operator=(const reference& _Bitref) noexcept {
                _Pbitset->_Set_unchecked(_Mypos, static_cast<bool>(_Bitref));
                return *this;
            }

            constexpr reference& flip() noexcept {
                _Pbitset->_Flip_unchecked(_Mypos);
                return *this;
            }

            constexpr bool operator~() const noexcept {
                return !_Pbitset->_Subscript(_Mypos);
            }

            constexpr operator bool() const noexcept {
                return _Pbitset->_Subscript(_Mypos);
            }

        private:
            constexpr reference() noexcept : _Pbitset(nullptr), _Mypos(0) {}

            constexpr reference(bitset<_Bits>& _Bitset, size_t _Pos) : _Pbitset(&_Bitset), _Mypos(_Pos) {}

            bitset<_Bits>* _Pbitset;
            size_t _Mypos;  // position of element in bitset
        };

        static constexpr void _Validate(size_t _Pos) {  // verify that _Pos is within bounds
            assert(_Pos < _Bits && "bitset index outside range");
        }

        constexpr bool _Subscript(size_t _Pos) const {
            return (_Array[_Pos / _Bitsperword] & (_Ty{ 1 } << _Pos % _Bitsperword)) != 0;
        }

        constexpr bool operator[](size_t _Pos) const {
            _Validate(_Pos);
            return _Subscript(_Pos);
        }

        constexpr reference operator[](size_t _Pos) {
            _Validate(_Pos);
            return reference(*this, _Pos);
        }

        constexpr bitset() noexcept : _Array() {}  // construct with all false values

        static constexpr bool _Need_mask = _Bits < CHAR_BIT * sizeof(unsigned long long);

        static constexpr unsigned long long _Mask = (1ULL << (_Need_mask ? _Bits : 0)) - 1ULL;

        constexpr bitset(unsigned long long _Val) noexcept : _Array{ static_cast<_Ty>(_Need_mask ? _Val & _Mask : _Val) } {}

        template <class _Traits, class _Elem>
        constexpr void _Construct(const _Elem* const _Ptr, size_t _Count, const _Elem _Elem0, const _Elem _Elem1) {
            if (_Count > _Bits) {
                for (size_t _Idx = _Bits; _Idx < _Count; ++_Idx) {
                    const auto _Ch = _Ptr[_Idx];
                    if (!_Traits::eq(_Elem1, _Ch) && !_Traits::eq(_Elem0, _Ch)) {
                        _Xinv();
                    }
                }

                _Count = _Bits;
            }

            size_t _Wpos = 0;
            if (_Count != 0) {
                size_t _Bits_used_in_word = 0;
                auto _Last = _Ptr + _Count;
                _Ty _This_word = 0;
                do {
                    --_Last;
                    const auto _Ch = *_Last;
                    _This_word |= static_cast<_Ty>(_Traits::eq(_Elem1, _Ch)) << _Bits_used_in_word;
                    if (!_Traits::eq(_Elem1, _Ch) && !_Traits::eq(_Elem0, _Ch)) {
                        _Xinv();
                    }

                    if (++_Bits_used_in_word == _Bitsperword) {
                        _Array[_Wpos] = _This_word;
                        ++_Wpos;
                        _This_word = 0;
                        _Bits_used_in_word = 0;
                    }
                } while (_Ptr != _Last);

                if (_Bits_used_in_word != 0) {
                    _Array[_Wpos] = _This_word;
                    ++_Wpos;
                }
            }

            for (; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] = 0;
            }
        }

        template <class _Elem, class _Traits, class _Alloc>
        constexpr explicit bitset(
            const std::basic_string<_Elem, _Traits, _Alloc>& _Str,
            typename std::basic_string<_Elem, _Traits, _Alloc>::size_type _Pos = 0,
            typename std::basic_string<_Elem, _Traits, _Alloc>::size_type _Count = std::basic_string<_Elem, _Traits, _Alloc>::npos,
            _Elem _Elem0 = static_cast<_Elem>('0'),
            _Elem _Elem1 = static_cast<_Elem>('1')) {
            // construct from [_Pos, _Pos + _Count) elements in string
            if (_Str.size() < _Pos) {
                _Xran();  // _Pos off end
            }

            if (_Str.size() - _Pos < _Count) {
                _Count = _Str.size() - _Pos;  // trim _Count to size
            }

            _Construct<_Traits>(_Str.data() + _Pos, _Count, _Elem0, _Elem1);
        }

        template <class _Elem>
        constexpr explicit bitset(const _Elem* _Ntcts,
            typename std::basic_string<_Elem>::size_type _Count = std::basic_string<_Elem>::npos,
            _Elem _Elem0 = static_cast<_Elem>('0'),
            _Elem _Elem1 = static_cast<_Elem>('1')) {
            if (_Count == std::basic_string<_Elem>::npos) {
                _Count = std::char_traits<_Elem>::length(_Ntcts);
            }

            _Construct<std::char_traits<_Elem>>(_Ntcts, _Count, _Elem0, _Elem1);
        }

        constexpr bitset& operator&=(const bitset& _Right) noexcept {
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] &= _Right._Array[_Wpos];
            }

            return *this;
        }

        constexpr bitset& operator|=(const bitset& _Right) noexcept {
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] |= _Right._Array[_Wpos];
            }

            return *this;
        }

        constexpr bitset& operator^=(const bitset& _Right) noexcept {
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] ^= _Right._Array[_Wpos];
            }

            return *this;
        }

        constexpr bitset& operator<<=(size_t _Pos) noexcept {  // shift left by _Pos, first by words then by bits
            const auto _Wordshift = static_cast<ptrdiff_t>(_Pos / _Bitsperword);
            if (_Wordshift != 0) {
                for (ptrdiff_t _Wpos = _Words; 0 <= _Wpos; --_Wpos) {
                    _Array[_Wpos] = _Wordshift <= _Wpos ? _Array[_Wpos - _Wordshift] : 0;
                }
            }

            if ((_Pos %= _Bitsperword) != 0) {  // 0 < _Pos < _Bitsperword, shift by bits
                for (ptrdiff_t _Wpos = _Words; 0 < _Wpos; --_Wpos) {
                    _Array[_Wpos] = (_Array[_Wpos] << _Pos) | (_Array[_Wpos - 1] >> (_Bitsperword - _Pos));
                }

                _Array[0] <<= _Pos;
            }
            _Trim();
            return *this;
        }

        constexpr bitset& operator>>=(size_t _Pos) noexcept {  // shift right by _Pos, first by words then by bits
            const auto _Wordshift = static_cast<ptrdiff_t>(_Pos / _Bitsperword);
            if (_Wordshift != 0) {
                for (ptrdiff_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                    _Array[_Wpos] = _Wordshift <= _Words - _Wpos ? _Array[_Wpos + _Wordshift] : 0;
                }
            }

            if ((_Pos %= _Bitsperword) != 0) {  // 0 < _Pos < _Bitsperword, shift by bits
                for (ptrdiff_t _Wpos = 0; _Wpos < _Words; ++_Wpos) {
                    _Array[_Wpos] = (_Array[_Wpos] >> _Pos) | (_Array[_Wpos + 1] << (_Bitsperword - _Pos));
                }

                _Array[_Words] >>= _Pos;
            }
            return *this;
        }

        constexpr bitset& set() noexcept {  // set all bits true
            //for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            //    _Array[_Wpos] = std::numeric_limits<_Ty>::;
            //}
            std::memset(&_Array, 0xFF, sizeof(_Array));

            _Trim();
            return *this;
        }

        constexpr bitset& set(size_t _Pos, bool _Val = true) {  // set bit at _Pos to _Val
            if (_Bits <= _Pos) {
                _Xran();  // _Pos off end
            }

            return _Set_unchecked(_Pos, _Val);
        }

        constexpr bitset& reset() noexcept {  // set all bits false
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] = 0;
            }
            // std::memset(&_Array, 0, sizeof(_Array));

            return *this;
        }

        constexpr bitset& reset(size_t _Pos) {  // set bit at _Pos to false
            return set(_Pos, false);
        }

        constexpr bitset operator~() const noexcept {  // flip all bits
            bitset _Tmp = *this;
            _Tmp.flip();
            return _Tmp;
        }

        constexpr bitset& flip() noexcept {  // flip all bits
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] = ~_Array[_Wpos];
            }

            _Trim();
            return *this;
        }

        constexpr bitset& flip(size_t _Pos) {  // flip bit at _Pos
            if (_Bits <= _Pos) {
                _Xran();  // _Pos off end
            }

            return _Flip_unchecked(_Pos);
        }

        constexpr unsigned long to_ulong() const {
            constexpr bool _Bits_zero = _Bits == 0;
            constexpr bool _Bits_small = _Bits <= 32;
            constexpr bool _Bits_large = _Bits > 64;
            if constexpr (_Bits_zero) {
                return 0;
            }
            else if constexpr (_Bits_small) {
                return static_cast<unsigned long>(_Array[0]);
            }
            else {
                if constexpr (_Bits_large) {
                    for (size_t _Idx = 1; _Idx <= _Words; ++_Idx) {
                        if (_Array[_Idx] != 0) {
                            _Xoflo();  // fail if any high-order words are nonzero
                        }
                    }
                }

                if (_Array[0] > ULONG_MAX) {
                    _Xoflo();
                }

                return static_cast<unsigned long>(_Array[0]);
            }
        }

        constexpr unsigned long long to_ullong() const {
            constexpr bool _Bits_zero = _Bits == 0;
            constexpr bool _Bits_large = _Bits > 64;
            if constexpr (_Bits_zero) {
                return 0;
            }
            else {
                if constexpr (_Bits_large) {
                    for (size_t _Idx = 1; _Idx <= _Words; ++_Idx) {
                        if (_Array[_Idx] != 0) {
                            _Xoflo();  // fail if any high-order words are nonzero
                        }
                    }
                }

                return _Array[0];
            }
        }

        template <class _Elem = char, class _Tr = std::char_traits<_Elem>, class _Alloc = std::allocator<_Elem>>
        constexpr std::basic_string<_Elem, _Tr, _Alloc> to_string(_Elem _Elem0 = static_cast<_Elem>('0'),
            _Elem _Elem1 = static_cast<_Elem>('1')) const {
            // convert bitset to string
            std::basic_string<_Elem, _Tr, _Alloc> _Str;
            _Str.reserve(_Bits);

            for (auto _Pos = _Bits; 0 < _Pos;) {
                _Str.push_back(_Subscript(--_Pos) ? _Elem1 : _Elem0);
            }

            return _Str;
        }

        constexpr size_t count() const noexcept {  // count number of set bits
            size_t result = 0;
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                result += std::popcount(_Array[_Wpos]);
            }

            return result;
            // const char* const _Bitsperbyte = "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4"
            //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            //                                 "\4\5\5\6\5\6\6\7\5\6\6\7\6\7\7\x8";
            // const unsigned char* _Ptr       = &reinterpret_cast<const unsigned char&>(_Array);
            // const unsigned char* const _End = _Ptr + sizeof(_Array);
            // size_t _Val                     = 0;
            // for (; _Ptr != _End; ++_Ptr) {
            //    _Val += _Bitsperbyte[*_Ptr];
            //}

            // return _Val;
        }

        constexpr size_t size() const noexcept {
            return _Bits;
        }

        constexpr bool operator==(const bitset& _Right) const noexcept {
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                if (_Array[_Wpos] != _Right._Array[_Wpos]) {
                    return false;
                }
            }
            return true;
            // return std::memcmp(&_Array[0], &_Right._Array[0], sizeof(_Array)) == 0;
        }

#if !_HAS_CXX20
        constexpr bool operator!=(const bitset& _Right) const noexcept {
            return !(*this == _Right);
        }
#endif  // !_HAS_CXX20

        constexpr bool test(size_t _Pos) const {
            if (_Bits <= _Pos) {
                _Xran();  // _Pos off end
            }

            return _Subscript(_Pos);
        }

        constexpr bool any() const noexcept {
            for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                if (_Array[_Wpos] != 0) {
                    return true;
                }
            }

            return false;
        }

        constexpr bool none() const noexcept {
            return !any();
        }

        constexpr bool all() const noexcept {
            constexpr bool _Zero_length = _Bits == 0;
            if constexpr (_Zero_length) {  // must test for this, otherwise would count one full word
                return true;
            }

            constexpr bool _No_padding = _Bits % _Bitsperword == 0;
            for (size_t _Wpos = 0; _Wpos < _Words + _No_padding; ++_Wpos) {
                if (_Array[_Wpos] != ~static_cast<_Ty>(0)) {
                    return false;
                }
            }

            return _No_padding || _Array[_Words] == (static_cast<_Ty>(1) << (_Bits % _Bitsperword)) - 1;
        }

        constexpr bitset operator<<(size_t _Pos) const noexcept {
            bitset _Tmp = *this;
            _Tmp <<= _Pos;
            return _Tmp;
        }

        constexpr bitset operator>>(size_t _Pos) const noexcept {
            bitset _Tmp = *this;
            _Tmp >>= _Pos;
            return _Tmp;
        }

        constexpr _Ty _Getword(size_t _Wpos) const noexcept {  // nonstandard extension; get underlying word
            return _Array[_Wpos];
        }

    private:
        friend std::hash<bitset<_Bits>>;

        static constexpr ptrdiff_t _Bitsperword = CHAR_BIT * sizeof(_Ty);
        static constexpr ptrdiff_t _Words = _Bits == 0 ? 0 : (_Bits - 1) / _Bitsperword;  // NB: number of words - 1

        constexpr void _Trim() noexcept {  // clear any trailing bits in last word
            constexpr bool _Work_to_do = _Bits == 0 || _Bits % _Bitsperword != 0;
            if constexpr (_Work_to_do) {
                _Array[_Words] &= (_Ty{ 1 } << _Bits % _Bitsperword) - 1;
            }
        }

        constexpr bitset& _Set_unchecked(size_t _Pos, bool _Val) noexcept {  // set bit at _Pos to _Val, no checking
            auto& _Selected_word = _Array[_Pos / _Bitsperword];
            const auto _Bit = _Ty{ 1 } << _Pos % _Bitsperword;
            if (_Val) {
                _Selected_word |= _Bit;
            }
            else {
                _Selected_word &= ~_Bit;
            }

            return *this;
        }

        constexpr bitset& _Flip_unchecked(size_t _Pos) noexcept {  // flip bit at _Pos, no checking
            _Array[_Pos / _Bitsperword] ^= _Ty{ 1 } << _Pos % _Bitsperword;
            return *this;
        }

        [[noreturn]] constexpr void _Xinv() const {
            throw std::invalid_argument("invalid bitset char");
        }

        [[noreturn]] constexpr void _Xoflo() const {
            throw std::overflow_error("bitset overflow");
        }

        [[noreturn]] constexpr void _Xran() const {
            throw std::out_of_range("invalid bitset position");
        }

        _Ty _Array[_Words + 1];
    };

    template <size_t _Bits>
    constexpr bitset<_Bits> operator&(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
        bitset<_Bits> _Ans = _Left;
        _Ans &= _Right;
        return _Ans;
    }

    template <size_t _Bits>
    constexpr bitset<_Bits> operator|(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
        bitset<_Bits> _Ans = _Left;
        _Ans |= _Right;
        return _Ans;
    }

    template <size_t _Bits>
    constexpr bitset<_Bits> operator^(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
        bitset<_Bits> _Ans = _Left;
        _Ans ^= _Right;
        return _Ans;
    }

}  // namespace nstd

template <size_t _Bits>
struct std::hash<nstd::bitset<_Bits>> {
    constexpr size_t operator()(const nstd::bitset<_Bits>& _BitSet) const noexcept {
        // EXAMPLE ONLY
        std::size_t result = 0;
        for (size_t i = 0; i <= _BitSet._Words; ++i) {
            result ^= _BitSet._Array[i];
        }
        return result;
    }
};

template<size_t _Bits>
using _BitSetClass = nstd::bitset<_Bits>;

#else
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

namespace fixstr
{

    namespace details
    {
        template <typename InputIterator, typename OutputIterator>
        constexpr OutputIterator copy(InputIterator first, InputIterator last, OutputIterator d_first)
        {
            return std::copy(first, last, d_first);
        }

        template <typename ForwardIterator, typename T>
        constexpr void fill(ForwardIterator first, ForwardIterator last, const T& value)
        {
            std::fill(first, last, value);
        }

    } // namespace details

    template <typename TChar, std::size_t N, typename TTraits = std::char_traits<TChar>>
    struct basic_fixed_string
    {
        // exposition only
        using storage_type = std::array<TChar, N + 1>;
#ifndef __INTELLISENSE__
        storage_type _data;
#else
        storage_type _data{};
#endif

        using traits_type = TTraits;
        using value_type = TChar;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = typename storage_type::iterator;
        using const_iterator = typename storage_type::const_iterator;
        using reverse_iterator = typename storage_type::reverse_iterator;
        using const_reverse_iterator = typename storage_type::const_reverse_iterator;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using string_view_type = std::basic_string_view<value_type, traits_type>;
        static constexpr auto npos = string_view_type::npos;

        constexpr basic_fixed_string() noexcept = default;

        constexpr basic_fixed_string(const value_type(&array)[N + 1]) noexcept // NOLINT(google-explicit-constructor)
        {
            details::copy(std::begin(array), std::end(array), _data.begin());
        }

        constexpr basic_fixed_string(basic_fixed_string const& other) noexcept { details::copy(other._data.begin(), other._data.end(), _data.begin()); }

        constexpr basic_fixed_string& operator=(const basic_fixed_string& other) noexcept
        {
            details::copy(other._data.begin(), other._data.end(), _data.begin());
            return *this;
        }

        constexpr basic_fixed_string& operator=(const value_type(&array)[N + 1]) noexcept
        {
            details::copy(std::begin(array), std::end(array), _data.begin());
            return *this;
        }

        // iterators
        [[nodiscard]] constexpr iterator               begin() noexcept { return _data.begin(); }
        [[nodiscard]] constexpr const_iterator         begin() const noexcept { return _data.begin(); }
        [[nodiscard]] constexpr iterator               end() noexcept { return _data.end() - 1; }
        [[nodiscard]] constexpr const_iterator         end() const noexcept { return _data.end() - 1; }
        [[nodiscard]] constexpr const_iterator         cbegin() const noexcept { return _data.cbegin(); }
        [[nodiscard]] constexpr const_iterator         cend() const noexcept { return _data.cend() - 1; }
        [[nodiscard]] constexpr reverse_iterator       rbegin() noexcept { return _data.rbegin() + 1; }
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return _data.rbegin() + 1; }
        [[nodiscard]] constexpr reverse_iterator       rend() noexcept { return _data.rend(); }
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return _data.rend(); }
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return _data.crbegin() + 1; }
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return _data.crend(); }

    private:
        [[nodiscard]] constexpr static bool static_empty() noexcept { return N == 0; }

    public:
        // capacity
        [[nodiscard]] constexpr size_type size() const noexcept { return N; }
        [[nodiscard]] constexpr size_type length() const noexcept { return N; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return N; }
        [[nodiscard]] constexpr bool      empty() const noexcept { return static_empty(); }

        // element access
        [[nodiscard]] constexpr reference       operator[](size_type n) { return _data[n]; }
        [[nodiscard]] constexpr const_reference operator[](size_type n) const { return _data[n]; }
        [[nodiscard]] constexpr reference       at(size_type n) { return _data.at(n); }
        [[nodiscard]] constexpr const_reference at(size_type n) const { return _data.at(n); }

        // The lack of C++20 concepts is disappointing
        // Basically what every `template<...>` line means is `requires (!empty())`
        template <typename..., bool NonEmpty = !static_empty(), typename = std::enable_if_t<NonEmpty>>
        [[nodiscard]] constexpr reference front() noexcept
        {
            return _data.front();
        }
        template <typename..., bool NonEmpty = !static_empty(), typename = std::enable_if_t<NonEmpty>>
        [[nodiscard]] constexpr const_reference front() const noexcept
        {
            return _data.front();
        }
        template <typename..., bool NonEmpty = !static_empty(), typename = std::enable_if_t<NonEmpty>>
        [[nodiscard]] constexpr reference back() noexcept
        {
            return _data[size() - 1];
        }
        template <typename..., bool NonEmpty = !static_empty(), typename = std::enable_if_t<NonEmpty>>
        [[nodiscard]] constexpr const_reference back() const noexcept
        {
            return _data[size() - 1];
        }

        [[nodiscard]] constexpr pointer       data() noexcept { return _data.data(); }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return _data.data(); }

    private:
        template <size_t M>
        using same_with_other_size = basic_fixed_string<value_type, M, traits_type>;

        template <size_type pos, size_type count, size_type size>
        constexpr static size_type calculate_substr_size()
        {
            if constexpr (pos >= size)
                return 0;

            constexpr size_type rcount = count < (size - pos) ? count : (size - pos);

            return rcount;
        }

        template <size_type pos, size_type count>
        using substr_result_type = same_with_other_size<calculate_substr_size<pos, count, N>()>;

    public:
        // string operations
        [[nodiscard]] constexpr operator string_view_type() const noexcept // NOLINT(google-explicit-constructor)
        {
            return { data(), N };
        }

        // clang-format off
        template <size_type pos = 0, size_type count = npos,
            typename..., bool IsPosInBounds = pos <= N, typename = std::enable_if_t<IsPosInBounds>>
            [[nodiscard]] constexpr auto substr() const noexcept
            -> substr_result_type<pos, count>
            // clang-format on
        {
            substr_result_type<pos, count> result{};
            details::copy(begin() + pos, begin() + pos + result.size(), result.begin());
            return result;
        }

        template <size_t M>
        [[nodiscard]] constexpr size_type find(const same_with_other_size<M>& str, size_type pos = 0) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().find(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type find(string_view_type sv, size_type pos = 0) const noexcept { return sv().find(sv, pos); }
        [[nodiscard]] constexpr size_type find(const value_type* s, size_type pos, size_type n) const { return sv().find(s, pos, n); }
        [[nodiscard]] constexpr size_type find(const value_type* s, size_type pos = 0) const { return sv().find(s, pos); }
        [[nodiscard]] constexpr size_type find(value_type c, size_type pos = 0) const noexcept { return sv().find(c, pos); }

        template <size_t M>
        [[nodiscard]] constexpr size_type rfind(const same_with_other_size<M>& str, size_type pos = npos) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().rfind(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type rfind(string_view_type sv, size_type pos = npos) const noexcept { return sv().rfind(sv, pos); }
        [[nodiscard]] constexpr size_type rfind(const value_type* s, size_type pos, size_type n) const { return sv().rfind(s, pos, n); }
        [[nodiscard]] constexpr size_type rfind(const value_type* s, size_type pos = npos) const { return sv().rfind(s, pos); }
        [[nodiscard]] constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept { return sv().rfind(c, pos); }

        template <size_t M>
        [[nodiscard]] constexpr size_type find_first_of(const same_with_other_size<M>& str, size_type pos = 0) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().find_first_of(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type find_first_of(string_view_type sv, size_type pos = 0) const noexcept { return sv().find_first_of(sv, pos); }
        [[nodiscard]] constexpr size_type find_first_of(const value_type* s, size_type pos, size_type n) const { return sv().find_first_of(s, pos, n); }
        [[nodiscard]] constexpr size_type find_first_of(const value_type* s, size_type pos = 0) const { return sv().find_first_of(s, pos); }
        [[nodiscard]] constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept { return sv().find_first_of(c, pos); }

        template <size_t M>
        [[nodiscard]] constexpr size_type find_last_of(const same_with_other_size<M>& str, size_type pos = npos) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().find_last_of(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type find_last_of(string_view_type sv, size_type pos = npos) const noexcept { return sv().find_last_of(sv, pos); }
        [[nodiscard]] constexpr size_type find_last_of(const value_type* s, size_type pos, size_type n) const { return sv().find_last_of(s, pos, n); }
        [[nodiscard]] constexpr size_type find_last_of(const value_type* s, size_type pos = npos) const { return sv().find_last_of(s, pos); }
        [[nodiscard]] constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept { return sv().find_last_of(c, pos); }

        template <size_t M>
        [[nodiscard]] constexpr size_type find_first_not_of(const same_with_other_size<M>& str, size_type pos = 0) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().find_first_of(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type find_first_not_of(string_view_type sv, size_type pos = 0) const noexcept { return sv().find_first_not_of(sv, pos); }
        [[nodiscard]] constexpr size_type find_first_not_of(const value_type* s, size_type pos, size_type n) const { return sv().find_first_not_of(s, pos, n); }
        [[nodiscard]] constexpr size_type find_first_not_of(const value_type* s, size_type pos = 0) const { return sv().find_first_not_of(s, pos); }
        [[nodiscard]] constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept { return sv().find_first_not_of(c, pos); }

        template <size_t M>
        [[nodiscard]] constexpr size_type find_last_not_of(const same_with_other_size<M>& str, size_type pos = npos) const noexcept
        {
            if constexpr (M > N)
                return npos;
            return sv().find_last_of(str.sv(), pos);
        }
        [[nodiscard]] constexpr size_type find_last_not_of(string_view_type sv, size_type pos = npos) const noexcept { return sv().find_last_not_of(sv, pos); }
        [[nodiscard]] constexpr size_type find_last_not_of(const value_type* s, size_type pos, size_type n) const { return sv().find_last_not_of(s, pos, n); }
        [[nodiscard]] constexpr size_type find_last_not_of(const value_type* s, size_type pos = npos) const { return sv().find_last_not_of(s, pos); }
        [[nodiscard]] constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept { return sv().find_last_not_of(c, pos); }

        [[nodiscard]] constexpr int compare(string_view_type v) const noexcept { return sv().compare(v); }
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, string_view_type v) const { return sv().compare(pos1, count1, v); }
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, string_view_type v, size_type pos2, size_type count2) const
        {
            return sv().compare(pos1, count1, v, pos2, count2);
        }
        [[nodiscard]] constexpr int compare(const value_type* s) const { return sv().compare(s); }
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const value_type* s) const { return sv().compare(pos1, count1, s); }
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const value_type* s, size_type count2) const
        {
            return sv().compare(pos1, count1, s, count2);
        }

        [[nodiscard]] constexpr bool starts_with(string_view_type v) const noexcept { return sv().substr(0, v.size()) == v; }
        [[nodiscard]] constexpr bool starts_with(char c) const noexcept { return !empty() && traits_type::eq(front(), c); }
        [[nodiscard]] constexpr bool starts_with(const value_type* s) const noexcept { return starts_with(string_view_type(s)); }

        [[nodiscard]] constexpr bool ends_with(string_view_type sv) const noexcept { return size() >= sv.size() && compare(size() - sv.size(), npos, sv) == 0; }
        [[nodiscard]] constexpr bool ends_with(value_type c) const noexcept { return !empty() && traits_type::eq(back(), c); }
        [[nodiscard]] constexpr bool ends_with(const value_type* s) const { return ends_with(string_view_type(s)); }

        [[nodiscard]] constexpr bool contains(string_view_type sv) const noexcept { return find(sv) != npos; }
        [[nodiscard]] constexpr bool contains(value_type c) const noexcept { return find(c) != npos; }
        [[nodiscard]] constexpr bool contains(const value_type* s) const { return find(s) != npos; }

        void swap(basic_fixed_string& other) noexcept(std::is_nothrow_swappable_v<storage_type>) { _data.swap(other._data); }

    private:
        constexpr string_view_type sv() const { return *this; }
    };

    template <typename TChar, typename TTraits, size_t N>
    void swap(basic_fixed_string<TChar, N, TTraits>& lhs, basic_fixed_string<TChar, N, TTraits>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator==(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        if constexpr (M1 != M2)
            return false;
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) == rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator==(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) == rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator==(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs == static_cast<sv_type>(rhs);
    }

#
    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr auto operator<=>(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) <=> rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr auto operator<=>(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) <=> rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr auto operator<=>(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs <=> static_cast<sv_type>(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator!=(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        if constexpr (M1 != M2)
            return true;
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) != rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator!=(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) != rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator!=(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs != static_cast<sv_type>(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator<(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) < rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator<(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) < rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator<(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs < static_cast<sv_type>(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator<=(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) <= rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator<=(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) <= rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator<=(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs <= static_cast<sv_type>(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator>(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) > rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator>(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) > rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator>(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs > static_cast<sv_type>(rhs);
    }

    template <typename TChar, typename TTraits, size_t M1, size_t M2>
    [[nodiscard]] constexpr bool operator>=(const basic_fixed_string<TChar, M1, TTraits>& lhs, const basic_fixed_string<TChar, M2, TTraits>& rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) >= rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator>=(const basic_fixed_string<TChar, N, TTraits>& lhs, std::basic_string_view<TChar, TTraits> rhs)
    {
        using lhs_type = std::decay_t<decltype(lhs)>;
        using sv_type = typename lhs_type::string_view_type;
        return static_cast<sv_type>(lhs) >= rhs;
    }

    template <typename TChar, typename TTraits, size_t N>
    [[nodiscard]] constexpr bool operator>=(std::basic_string_view<TChar, TTraits> lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        using rhs_type = std::decay_t<decltype(rhs)>;
        using sv_type = typename rhs_type::string_view_type;
        return lhs >= static_cast<sv_type>(rhs);
    }

    template <typename TChar, size_t N>
    basic_fixed_string(const TChar(&)[N]) -> basic_fixed_string<TChar, N - 1>;

    // Early GCC versions that support cNTTP were not able to deduce size_t parameter
    // of basic_fixed_string when fixed_string and other typedef were just type aliases.
    // That's why the following code is written in this way.
    template <size_t N>
    struct fixed_string : basic_fixed_string<char, N>
    {
        using basic_fixed_string<char, N>::basic_fixed_string;
    };
    template <std::size_t N>
    fixed_string(const char(&)[N]) -> fixed_string<N - 1>;

    template <size_t N>
    struct fixed_u8string : basic_fixed_string<char8_t, N>
    {
        using basic_fixed_string<char8_t, N>::basic_fixed_string;
    };
    template <std::size_t N>
    fixed_u8string(const char8_t(&)[N]) -> fixed_u8string<N - 1>;

    template <size_t N>
    struct fixed_u16string : basic_fixed_string<char16_t, N>
    {
        using basic_fixed_string<char16_t, N>::basic_fixed_string;
    };
    template <std::size_t N>
    fixed_u16string(const char16_t(&)[N]) -> fixed_u16string<N - 1>;

    template <size_t N>
    struct fixed_u32string : basic_fixed_string<char32_t, N>
    {
        using basic_fixed_string<char32_t, N>::basic_fixed_string;
    };
    template <std::size_t N>
    fixed_u32string(const char32_t(&)[N]) -> fixed_u32string<N - 1>;

    template <size_t N>
    struct fixed_wstring : basic_fixed_string<wchar_t, N>
    {
        using basic_fixed_string<wchar_t, N>::basic_fixed_string;
    };
    template <std::size_t N>
    fixed_wstring(const wchar_t(&)[N]) -> fixed_wstring<N - 1>;

    template <typename TChar, size_t N, size_t M, typename TTraits>
    constexpr basic_fixed_string<TChar, N + M, TTraits> operator+(const basic_fixed_string<TChar, N, TTraits>& lhs, const basic_fixed_string<TChar, M, TTraits>& rhs)
    {
        basic_fixed_string<TChar, N + M, TTraits> result;
        details::copy(lhs.begin(), lhs.end(), result.begin());
        details::copy(rhs.begin(), rhs.end(), result.begin() + N);
        return result;
    }

    template <typename TChar, size_t N, size_t M, typename TTraits>
    constexpr basic_fixed_string<TChar, N - 1 + M, TTraits> operator+(const TChar(&lhs)[N], const basic_fixed_string<TChar, M, TTraits>& rhs)
    {
        basic_fixed_string lhs2 = lhs;
        return lhs2 + rhs;
    }

    template <typename TChar, size_t N, size_t M, typename TTraits>
    constexpr basic_fixed_string<TChar, N + M - 1, TTraits> operator+(const basic_fixed_string<TChar, N, TTraits>& lhs, const TChar(&rhs)[M])
    {
        basic_fixed_string rhs2 = rhs;
        return lhs + rhs2;
    }

    namespace details
    {
        template <typename TChar>
        constexpr basic_fixed_string<TChar, 1> from_char(TChar ch)
        {
            basic_fixed_string<TChar, 1> fs;
            fs[0] = ch;
            return fs;
        }
    } // namespace details

    template <typename TChar, size_t N, typename TTraits>
    constexpr basic_fixed_string<TChar, N + 1, TTraits> operator+(TChar lhs, const basic_fixed_string<TChar, N, TTraits>& rhs)
    {
        return details::from_char(lhs) + rhs;
    }

    template <typename TChar, size_t N, typename TTraits>
    constexpr basic_fixed_string<TChar, N + 1, TTraits> operator+(const basic_fixed_string<TChar, N, TTraits>& lhs, TChar rhs)
    {
        return lhs + details::from_char(rhs);
    }

    template <typename TChar, size_t N, typename TTraits>
    std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const basic_fixed_string<TChar, N, TTraits>& str)
    {
        out << str.data();
        return out;
    }

} // namespace fixstr

// hash support
namespace std
{
    template <size_t N>
    struct hash<fixstr::fixed_string<N>>
    {
        using argument_type = fixstr::fixed_string<N>;
        size_t operator()(const argument_type& str) const
        {
            using sv_t = typename argument_type::string_view_type;
            return std::hash<sv_t>()(static_cast<sv_t>(str));
        }
    };

    template <size_t N>
    struct hash<fixstr::fixed_u8string<N>>
    {
        using argument_type = fixstr::fixed_u8string<N>;
        size_t operator()(const argument_type& str) const
        {
            using sv_t = typename argument_type::string_view_type;
            return std::hash<sv_t>()(static_cast<sv_t>(str));
        }
    };

    template <size_t N>
    struct hash<fixstr::fixed_u16string<N>>
    {
        using argument_type = fixstr::fixed_u16string<N>;
        size_t operator()(const argument_type& str) const
        {
            using sv_t = typename argument_type::string_view_type;
            return std::hash<sv_t>()(static_cast<sv_t>(str));
        }
    };

    template <size_t N>
    struct hash<fixstr::fixed_u32string<N>>
    {
        using argument_type = fixstr::fixed_u32string<N>;
        size_t operator()(const argument_type& str) const
        {
            using sv_t = typename argument_type::string_view_type;
            return std::hash<sv_t>()(static_cast<sv_t>(str));
        }
    };

    template <size_t N>
    struct hash<fixstr::fixed_wstring<N>>
    {
        using argument_type = fixstr::fixed_wstring<N>;
        size_t operator()(const argument_type& str) const
        {
            using sv_t = typename argument_type::string_view_type;
            return std::hash<sv_t>()(static_cast<sv_t>(str));
        }
    };

} // namespace std

namespace SigSearch
{
    // just inline for loop with const size N
    template <size_t N>
    ALWAYS_INLINE_PRE ALWAYS_INLINE_MID ALWAYS_INLINE_POST constexpr inline static void Repeat(auto f) {
        [f] <auto... Index>(std::index_sequence<Index...>) {
            (f. template operator() < Index > (), ...);
        }(std::make_index_sequence<N>());
    }

    // inlining for cycle with const size N until first "return false" met
    template <size_t N>
    ALWAYS_INLINE_PRE ALWAYS_INLINE_MID ALWAYS_INLINE_POST constexpr inline static void RepeatUntilFalse(auto f) {
        [f] <auto... Index>(std::index_sequence<Index...>) {
            bool res = true;
            ((res && (res = f.template operator() < Index > ())), ...);
        }(std::make_index_sequence<N>());
    }

    consteval unsigned hex_char_to_val(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw "Invalid hex digit";
    }

    template<fixstr::basic_fixed_string Src>
    consteval size_t get_sig_lenght() {
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
    }

    template <fixstr::basic_fixed_string Src>
    struct CompileTimeSignature {
        static constexpr size_t N = get_sig_lenght<Src>();
        constexpr static std::array<char, N> bytes = []() consteval {
                size_t idx = 0;
                std::array<char, N> temp;
                for (size_t i = 0; i < Src.size();) {
                    if (Src[i] == ' ') { ++i; continue; }

                    if (Src[i] == '?') {
                        temp[idx] = 0;
                        if (i + 1 < Src.size() && Src[i + 1] == '?') i += 2;
                        else ++i;
                    }
                    else {
                        unsigned high = hex_char_to_val(Src[i]);
                        unsigned low = hex_char_to_val(Src[i + 1]);
                        temp[idx] = static_cast<char>((high << 4) | low);
                        i += 2;
                    }
                    ++idx;
                }
                return temp;
            }();
        constexpr static _BitSetClass<N> mask = []() consteval {
                size_t idx = 0;
                _BitSetClass<N> temp;
                for (size_t i = 0; i < Src.size();) {
                    if (Src[i] == ' ') { ++i; continue; }

                    if (Src[i] == '?') {
                        temp[idx] = 0;
                        if (i + 1 < Src.size() && Src[i + 1] == '?') i += 2;
                        else ++i;
                    }
                    else {
                        temp[idx] = 1;
                        i += 2;
                    }
                    ++idx;
                }
                return temp;
            }();

        static bool MatchAt(uintptr_t start) {
            bool result = true;
            RepeatUntilFalse<N>(ALWAYS_INLINE_PRE[&result, start]<size_t Index>() ALWAYS_INLINE_MID ALWAYS_INLINE_POST {
                if constexpr (mask.test(Index)) {
                    if (bytes[Index] != *reinterpret_cast<const char*>(start + Index)) {
                        result = false;
                        return false;
                    }
                }
                return true;
            });
            return result;
        }

        static constexpr size_t size() noexcept { return N; }
    };

    template <fixstr::basic_fixed_string Src>
    struct Signature {
        bool MatchAt(uintptr_t start) const {
            return CompileTimeSignature<Src>::MatchAt(start);
        }
        size_t size() const noexcept {
            return CompileTimeSignature<Src>::size();
        };
    };

    template <typename>
    struct is_signature : std::false_type {};

    template <fixstr::basic_fixed_string Src>
    struct is_signature<Signature<Src>> : std::true_type {};

    // Finds single signature in [start,end) range and returns found address
    // Found address can be zero if range does not have specified signature
    template <typename T>
    static uintptr_t FindSignatureInRange(uintptr_t start, uintptr_t end, const T& sig) {
        static_assert(
            is_signature<T>::value,
            "sig must be of type Signature<...>"
            );
        if (start == 0 || end == 0 || end <= start) {
            return 0;
        }
        for (uintptr_t i = start; i < end - sig.size(); ++i)
        {
            if (sig.MatchAt(i))
            {
                return i;
            }
        }

        return 0;
    }

    // Finds all specified signatures in [start,end) range and returns first found address
    // Found address can be zero if range does not have any specified signatures
    template <typename... Sigs>
    static uintptr_t FindAnyInRange(uintptr_t start, uintptr_t end, const Sigs&... sigs) {
        static_assert(
            (is_signature<Sigs>::value && ...),
            "sigs must be of type Signature<...>"
            );
        if (start == 0 || end == 0 || end <= start) {
            return 0;
        }

        for (uintptr_t addr = start; addr < end; ++addr) {
            if (((addr + sigs.size() <= end && sigs.MatchAt(addr)) || ...)) {
                return addr;
            }
        }
        return 0;
    }

    // Finds all specified signatures in [start,end) range and returns an std::array with found addresses
    // One or more found address can be zero if range does not have this signature
    template <typename... Sigs>
    static auto FindAllInRange(uintptr_t start, uintptr_t end, const Sigs&... sigs) -> std::array<uintptr_t, sizeof...(sigs)> {
        static_assert(
            (is_signature<Sigs>::value && ...),
            "sigs must be of type Signature<...>"
            );
        constexpr std::size_t size = sizeof...(sigs);

        std::array<uintptr_t, size> results{};
        if (start == 0 || end == 0 || end <= start) {
            return results;
        }

        const auto signatures = std::tie(sigs...);

        for (uintptr_t addr = start; addr < end; ++addr) {
            bool found_all = true;
            Repeat<size>(ALWAYS_INLINE_PRE [&]<size_t Index>() ALWAYS_INLINE_MID ALWAYS_INLINE_POST {
                if (results[Index] != 0) return;

                found_all = false;

                const auto& sig = std::get<Index>(signatures);

                if (addr + sig.size() > end) return;

                if (sig.MatchAt(addr)) {
                    results[Index] = addr;
                }
            });
            
            if (found_all)
            {
                break;
            }
        }
        return results;
    }

    inline namespace literals {
        template <fixstr::basic_fixed_string Src>
        consteval auto operator""_sig() {
            return Signature<Src>();
        }
    }
}
#endif // SegSearch_HPP
