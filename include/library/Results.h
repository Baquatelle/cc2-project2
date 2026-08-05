#pragma once

#include <iosfwd>
#include <string_view>

namespace library
{

enum class AddBookResult
{
    Success,
    DuplicateIsbn,
};

enum class BorrowResult
{
    Success,
    BookNotFound,
    BookAlreadyBorrowed,
    MemberNotRegistered,
    BorrowLimitReached,
};

enum class ReturnResult
{
    Success,
    BookNotFound,
    NotBorrowedByMember,
    MemberNotRegistered,
};

enum class RemoveBookResult
{
    Success,
    BookNotFound,
    BookIsBorrowed,
};

/// Why a member accepted or refused a book, returned by Member::BorrowBook.
///
/// The member is the single authority on its own borrowing policy, so it
/// reports the reason rather than returning a bare bool and leaving the
/// library to re-derive it. That keeps the reported outcome correct when a
/// new member type with different rules is added (OCP).
enum class MemberBorrowResult
{
    Success,
    AlreadyHeld,
    LimitReached,
    BookUnavailable,
};

// Found by ADL on the enum's namespace, so generic code can print results.
[[nodiscard]] std::string_view to_string(AddBookResult result) noexcept;
[[nodiscard]] std::string_view to_string(BorrowResult result) noexcept;
[[nodiscard]] std::string_view to_string(ReturnResult result) noexcept;
[[nodiscard]] std::string_view to_string(RemoveBookResult result) noexcept;
[[nodiscard]] std::string_view to_string(MemberBorrowResult result) noexcept;

} // namespace library
