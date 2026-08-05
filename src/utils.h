#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace library
{
namespace utils
{

[[nodiscard]] bool is_blank(std::string_view aValue) noexcept
{
    return std::all_of(aValue.begin(), aValue.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
}

} // namespace utils
} // namespace library

/// Throw an exception is the field is empty or blank. This is a broken
/// invariant.
#define REQUIRE_NOT_BLANK(aField)                                                                                      \
    if ((aField).empty() || ::library::utils::is_blank(aField))                                                        \
    {                                                                                                                  \
        throw std::invalid_argument("Book: " + std::string(#aField) + " must not be empty or blank");                  \
    }
