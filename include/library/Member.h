#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <library/Book.h>
#include <library/Results.h>

namespace library
{

/// Member holds everything common to all member types. Subclasses vary the
/// borrow limit and nothing else.
class Member
{
  public:
    Member(std::string aName, member_id aMemberId);
    virtual ~Member() = default;

    // Polymorphic base: non-copyable and non-movable to rule out slicing.
    // Members are always handled through shared_ptr, so this costs nothing.
    Member(const Member &) = delete;
    Member &operator=(const Member &) = delete;
    Member(Member &&) = delete;
    Member &operator=(Member &&) = delete;

    [[nodiscard]] const std::string &Name() const noexcept
    {
        return mName;
    }
    [[nodiscard]] member_id MemberId() const noexcept
    {
        return mMemberId;
    }

    /// Maximum books this member may hold at once. The single extension point.
    [[nodiscard]] virtual std::size_t MaxBooks() const noexcept = 0;

    /// Human-readable member category, for display.
    [[nodiscard]] virtual std::string_view Category() const noexcept = 0;

    /// @brief Returns the list of books currently borrowed by the member.
    /// @return A vector of weak pointers to the borrowed books.
    [[nodiscard]] const std::vector<std::weak_ptr<Book>> &ListOfBorrowedBooks() const noexcept
    {
        return mListOfBorrowedBooks;
    }

    /// Number of books currently held. Counts only live entries, so a book
    /// that disappeared never permanently consumes a slot.
    [[nodiscard]] std::size_t BorrowedCount() const noexcept;

    /// @brief  Checks if the member is at its borrowing limit.
    /// @return True if the member is at its borrowing limit, false otherwise.
    [[nodiscard]] bool ReachedLimit() const noexcept
    {
        return BorrowedCount() >= MaxBooks();
    }

    /// @brief Checks if the member has borrowed a book with the given ISBN.
    /// @param aISBN The ISBN of the book to check.
    /// @return True if the member has borrowed the book, false otherwise.
    [[nodiscard]] bool HasBorrowed(const isbn &aISBN) const;

    /// @brief Records a borrowed book.
    /// @param aBook The book to borrow.
    /// \returns MemberBorrowResult::Success if the book was borrowed.
    /// \returns MemberBorrowResult::AlreadyHeld if the book is already held.
    /// \returns MemberBorrowResult::LimitReached if the member is at its limit.
    /// \returns MemberBorrowResult::BookUnavailable if the book is expired.
    [[nodiscard]] MemberBorrowResult BorrowBook(std::weak_ptr<Book> aBook);

    /// @brief Records a returned book.
    /// @param aISBN The ISBN of the book to return.
    /// \returns false if this member is not holding \p aISBN.
    [[nodiscard]] bool ReturnBook(const isbn &aISBN);

  private:
    std::string                      mName;                ///< The member's name. Non-empty, non-blank.
    member_id                        mMemberId;            ///< The member's ID. Unique within the library.
    std::vector<std::weak_ptr<Book>> mListOfBorrowedBooks; ///< The books currently borrowed by this member.
};

} // namespace library
