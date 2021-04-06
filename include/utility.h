#ifndef UTILITY_H
#define UTILITY_H

#include <utility>
#include <string_view>

namespace utils
{
    template<typename Type>
    [[nodiscard]] constexpr std::pair<Type, Type> GetMinMax(const Type first, const Type second) noexcept
    {
        if (first > second)
        {
            return { second, first };
        }

        return { first, second };
    }

    template<typename Type>
    [[nodiscard]] constexpr auto GetSignedDiff(const Type lhs, const Type rhs) noexcept
    {
        using type = typename std::make_signed_t<Type>;
        return static_cast<type>(lhs) - static_cast<type>(rhs);
    }

    template<typename ... Args> struct MakeVisitor : Args... { using Args::operator()...; };
    template<typename ... Args> MakeVisitor(Args...) -> MakeVisitor<Args...>;

    [[nodiscard]] inline std::wstring_view GetFileName(const std::wstring_view filePath) noexcept
    {
        const auto it = std::find_if(filePath.rbegin(), filePath.rend(), [] (const wchar_t c) { return c == L'/' || c == L'\\'; } );

        if (it != filePath.rend())
        {
            const auto index = std::distance(filePath.begin(), it.base());
            return { filePath.data() + index, filePath.size() - index };
        }

        return {};
    }
}




#endif