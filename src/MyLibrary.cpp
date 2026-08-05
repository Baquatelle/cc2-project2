#include <library/MyLibrary.h>

#include <library/InMemoryBookRepository.h>

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace library
{

MyLibrary::MyLibrary() : mRepository(std::make_unique<InMemoryBookRepository>())
{
}

MyLibrary::MyLibrary(std::unique_ptr<IBookRepository> aRepository) : mRepository(std::move(aRepository))
{
    if (!mRepository)
    {
        throw std::invalid_argument("MyLibrary: repository must not be null");
    }
}

std::shared_ptr<Member> MyLibrary::LookupMember(member_id aMemberId) const
{
    const auto it = mMembers.find(aMemberId);

    return it == mMembers.end() ? nullptr : it->second.lock();
}

AddBookResult MyLibrary::AddBook(std::shared_ptr<Book> book)
{
    if (!book)
    {
        throw std::invalid_argument("MyLibrary::AddBook: book must not be null");
    }

    return mRepository->Add(std::move(book)) ? AddBookResult::Success : AddBookResult::DuplicateIsbn;
}

BorrowResult MyLibrary::BorrowBook(member_id aMemberId, const isbn &aISBN)
{
    // Order matters. Every check happens BEFORE either side is mutated, so
    // the member's list and mLoans can never disagree.
    const std::shared_ptr<Member> Member = LookupMember(aMemberId);
    if (!Member)
    {
        return BorrowResult::MemberNotRegistered;
    }

    const std::weak_ptr<Book>   BookRef = mRepository->Find(aISBN);
    const std::shared_ptr<Book> book = BookRef.lock();
    if (!book)
    {
        return BorrowResult::BookNotFound;
    }

    if (mLoans.contains(aISBN))
    {
        return BorrowResult::BookAlreadyBorrowed;
    }

    // The member is the single authority on its own policy: it decides and
    // reports why. Forwarding that reason (rather than re-deriving it here)
    // means a member type with different rules cannot make the reported
    // outcome go stale. The switch is exhaustive, so adding a reason later
    // is a compile error rather than a silent mislabel.
    switch (Member->BorrowBook(BookRef))
    {
    case MemberBorrowResult::Success:
        break;
    case MemberBorrowResult::LimitReached:
        return BorrowResult::BorrowLimitReached;
    case MemberBorrowResult::AlreadyHeld:
        return BorrowResult::BookAlreadyBorrowed;
    case MemberBorrowResult::BookUnavailable:
        return BorrowResult::BookNotFound;
    default:
        throw std::logic_error("MyLibrary::BorrowBook: unhandled MemberBorrowResult");
    }

    // Only now, with nothing left that can fail, commit the other end.
    mLoans.emplace(aISBN, Loan{BookRef, Member});
    return BorrowResult::Success;
}

ReturnResult MyLibrary::ReturnBook(member_id aMemberId, const isbn &aISBN)
{
    const std::shared_ptr<Member> MemberReturning = LookupMember(aMemberId);
    if (!MemberReturning)
    {
        return ReturnResult::MemberNotRegistered;
    }

    if (!mRepository->Contains(aISBN))
    {
        return ReturnResult::BookNotFound;
    }

    const auto LoanIt = mLoans.find(aISBN);
    if (LoanIt == mLoans.end())
    {
        return ReturnResult::NotBorrowedByMember; // not on loan at all
    }

    const std::shared_ptr<Member> Borrower = LoanIt->second.mBorrower.lock();
    if (Borrower != MemberReturning)
    {
        return ReturnResult::NotBorrowedByMember; // on loan to someone else
    }

    // Update both ends of the association together.
    if (!MemberReturning->ReturnBook(aISBN))
    {
        return ReturnResult::NotBorrowedByMember;
    }

    mLoans.erase(LoanIt);
    return ReturnResult::Success;
}

bool MyLibrary::RegisterMember(const std::shared_ptr<Member> &aMember)
{
    if (!aMember)
    {
        throw std::invalid_argument("MyLibrary::RegisterMember: member must not be null");
    }
    // Deliberately stores a weak_ptr, not the shared_ptr: aggregation.
    return mMembers.emplace(aMember->MemberId(), aMember).second;
}

bool MyLibrary::UnregisterMember(member_id aMemberId)
{
    const auto it = mMembers.find(aMemberId);
    if (it == mMembers.end())
    {
        return false;
    }

    const std::shared_ptr<Member> MemberToUnregister = it->second.lock();
    mMembers.erase(it);

    if (!MemberToUnregister)
    {
        // Already destroyed, so there is no member-side state to repair and
        // its loans are indistinguishable from any other departed member's.
        // Releasing orphaned loans is precisely what pruning does, so reuse it
        // rather than repeating the sweep.
        (void)PruneDepartedMembers();
        return true;
    }

    // Collect this member's loans first, then release them. Mutating the
    // member from inside an erase_if predicate would work, but keeping the
    // predicate side-effect-free is clearer.
    std::vector<std::string> to_release;
    for (const auto &entry : mLoans)
    {
        if (entry.second.mBorrower.lock() == MemberToUnregister)
        {
            to_release.push_back(entry.first);
        }
    }

    // Hand each book back through the normal member-side path, updating BOTH
    // ends together. Doing it per ISBN -- rather than clearing the member's
    // whole list -- matters because the client owns the member, not us:
    // nothing stops it being registered with another library, and those
    // loans must survive. Without this the member would keep claiming books
    // that are already back on the shelf, permanently burning its borrow
    // slots and letting one copy be held twice.
    for (const std::string &isbn : to_release)
    {
        (void)MemberToUnregister->ReturnBook(isbn);
        mLoans.erase(isbn);
    }

    return true;
}

bool MyLibrary::IsRegistered(member_id aMemberId) const
{
    return LookupMember(aMemberId) != nullptr;
}

std::size_t MyLibrary::RegisteredMemberCount() const
{
    return static_cast<std::size_t>(
        std::count_if(mMembers.begin(), mMembers.end(), [](const auto &entry) { return !entry.second.expired(); }));
}

std::size_t MyLibrary::PruneDepartedMembers()
{
    // A departed member's loans are orphaned: the borrower weak_ptr has
    // expired. Release those books back into circulation first.
    std::erase_if(mLoans, [](const auto &entry) { return entry.second.mBorrower.expired(); });

    return std::erase_if(mMembers, [](const auto &entry) { return entry.second.expired(); });
}

RemoveBookResult MyLibrary::RemoveBook(const isbn &aISBN)
{
    if (!mRepository->Contains(aISBN))
    {
        return RemoveBookResult::BookNotFound;
    }

    if (mLoans.contains(aISBN))
    {
        // Guarding this is what makes sole ownership safe: destroying a
        // borrowed book would expire the borrower's observer mid-loan.
        return RemoveBookResult::BookIsBorrowed;
    }

    return mRepository->Remove(aISBN) ? RemoveBookResult::Success : RemoveBookResult::BookNotFound;
}

bool MyLibrary::IsBorrowed(const isbn &aISBN) const
{
    return mLoans.contains(aISBN);
}

std::optional<MyLibrary::Loan> MyLibrary::FindLoan(const isbn &aISBN) const
{
    const auto it = mLoans.find(aISBN);

    return it == mLoans.end() ? std::nullopt : std::optional<Loan>{it->second};
}

} // namespace library
