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

    template <fixstr::basic_fixed_string Src>
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
            else if(can_read(1)) {
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

            if (opId >= opCount) {
                throw std::exception("shit");
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
            else if(can_read(1)) {
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

    template <fixstr::basic_fixed_string Src>
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
            Repeat<size>(ALWAYS_INLINE_PRE[&]<size_t Index>() ALWAYS_INLINE_MID ALWAYS_INLINE_POST {
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
