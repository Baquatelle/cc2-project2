#include <library/Results.h>

#include <ostream>

namespace library
{

std::string_view to_string(AddBookResult result) noexcept
{
    switch (result)
    {
    case AddBookResult::Success:
        return "Success";
    case AddBookResult::DuplicateIsbn:
        return "DuplicateIsbn";
    }
    return "Unknown"; // silences MSVC C4715 on /W4
}

std::string_view to_string(BorrowResult result) noexcept
{
    switch (result)
    {
    case BorrowResult::Success:
        return "Success";
    case BorrowResult::BookNotFound:
        return "BookNotFound";
    case BorrowResult::BookAlreadyBorrowed:
        return "BookAlreadyBorrowed";
    case BorrowResult::MemberNotRegistered:
        return "MemberNotRegistered";
    case BorrowResult::BorrowLimitReached:
        return "BorrowLimitReached";
    }
    return "Unknown";
}

std::string_view to_string(ReturnResult result) noexcept
{
    switch (result)
    {
    case ReturnResult::Success:
        return "Success";
    case ReturnResult::BookNotFound:
        return "BookNotFound";
    case ReturnResult::NotBorrowedByMember:
        return "NotBorrowedByMember";
    case ReturnResult::MemberNotRegistered:
        return "MemberNotRegistered";
    }
    return "Unknown";
}

std::string_view to_string(RemoveBookResult result) noexcept
{
    switch (result)
    {
    case RemoveBookResult::Success:
        return "Success";
    case RemoveBookResult::BookNotFound:
        return "BookNotFound";
    case RemoveBookResult::BookIsBorrowed:
        return "BookIsBorrowed";
    }
    return "Unknown";
}

std::string_view to_string(MemberBorrowResult result) noexcept
{
    switch (result)
    {
    case MemberBorrowResult::Success:
        return "Success";
    case MemberBorrowResult::AlreadyHeld:
        return "AlreadyHeld";
    case MemberBorrowResult::LimitReached:
        return "LimitReached";
    case MemberBorrowResult::BookUnavailable:
        return "BookUnavailable";
    }
    return "Unknown";
}

} // namespace library
