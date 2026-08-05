#include <library/Member.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace library
{

Member::Member(std::string aName, member_id aMemberId) : mName(std::move(aName)), mMemberId(aMemberId)
{
    if (mName.empty())
    {
        throw std::invalid_argument("Member: name must not be empty");
    }
}

std::size_t Member::BorrowedCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(mListOfBorrowedBooks.begin(), mListOfBorrowedBooks.end(),
                                                  [](const std::weak_ptr<Book> &entry) { return !entry.expired(); }));
}

bool Member::HasBorrowed(const isbn &aISBN) const
{
    return std::any_of(mListOfBorrowedBooks.begin(), mListOfBorrowedBooks.end(),
                       [&aISBN](const std::weak_ptr<Book> &entry) {
                           // Promote -> use -> drop. Never deref an unlocked
                           // weak_ptr.
                           const std::shared_ptr<Book> book = entry.lock();
                           return book && book->ISBN() == aISBN;
                       });
}

MemberBorrowResult Member::BorrowBook(std::weak_ptr<Book> aBook)
{
    const std::shared_ptr<Book> LockedBook = aBook.lock();
    if (!LockedBook)
    {
        return MemberBorrowResult::BookUnavailable; // the book no longer exists
    }
    if (HasBorrowed(LockedBook->ISBN()))
    {
        return MemberBorrowResult::AlreadyHeld;
    }
    if (ReachedLimit())
    {
        return MemberBorrowResult::LimitReached; // the member's own policy refuses
    }

    mListOfBorrowedBooks.push_back(std::move(aBook));
    return MemberBorrowResult::Success;
}

bool Member::ReturnBook(const isbn &aISBN)
{
    const auto it = std::find_if(mListOfBorrowedBooks.begin(), mListOfBorrowedBooks.end(),
                                 [&aISBN](const std::weak_ptr<Book> &entry) {
                                     const std::shared_ptr<Book> book = entry.lock();
                                     return book && book->ISBN() == aISBN;
                                 });

    if (it == mListOfBorrowedBooks.end())
    {
        return false;
    }

    mListOfBorrowedBooks.erase(it);
    return true;
}

} // namespace library
