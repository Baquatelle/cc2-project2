#include <library/Book.h>
#include <library/IBookRepository.h>
#include <library/InMemoryBookRepository.h>
#include <library/Member.h>
#include <library/MyLibrary.h>
#include <library/PremiumMember.h>
#include <library/RegularMember.h>
#include <library/Results.h>

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace library;

namespace
{

[[nodiscard]] std::shared_ptr<Book> make_book(const isbn &aISBN)
{
    return std::make_shared<Book>("Title " + aISBN, "Author " + aISBN, aISBN);
}

/// Stocks the library with \p count books, ISBNs "1".. "count".
void stock(MyLibrary &lib, int count)
{
    for (int i = 1; i <= count; ++i)
    {
        EXPECT_EQ(lib.AddBook(make_book(std::to_string(i))), AddBookResult::Success);
    }
}

// ------------------------------------------------------------------
// ENCAPSULATION
// ------------------------------------------------------------------

TEST(BookTest, Encapsulation)
{
    Book book("Dune", "Herbert", "978-0441");
    EXPECT_EQ(book.Title(), std::string("Dune"));
    EXPECT_EQ(book.Author(), std::string("Herbert"));
    EXPECT_EQ(book.ISBN(), isbn("978-0441"));

    book.SetTitle("Dune Messiah");
    book.SetAuthor("Frank Herbert");
    EXPECT_EQ(book.Title(), std::string("Dune Messiah"));
    EXPECT_EQ(book.Author(), std::string("Frank Herbert"));

    // Invariants are enforced, so a Book cannot be driven into a bad state.
    EXPECT_THROW(Book("", "Herbert", "x"), std::invalid_argument);
    EXPECT_THROW(Book("Dune", "   ", "x"), std::invalid_argument);
    EXPECT_THROW(Book("Dune", "Herbert", ""), std::invalid_argument);
    EXPECT_THROW(book.SetTitle(""), std::invalid_argument);
}

// ------------------------------------------------------------------
// INHERITANCE / OCP / LSP
// ------------------------------------------------------------------

/// The member owns its policy and reports WHY it refused, so the library
/// never has to guess. Exercised directly at the member level here.
TEST(MemberTest, ReportsItsOwnRefusalReason)
{
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto one = make_book("1");
    const auto two = make_book("2");
    const auto three = make_book("3");
    const auto four = make_book("4");

    EXPECT_EQ(ana->BorrowBook(one), MemberBorrowResult::Success);
    EXPECT_EQ(ana->BorrowBook(one), MemberBorrowResult::AlreadyHeld);
    EXPECT_EQ(ana->BorrowBook(two), MemberBorrowResult::Success);
    EXPECT_EQ(ana->BorrowBook(three), MemberBorrowResult::Success);
    EXPECT_EQ(ana->BorrowBook(four), MemberBorrowResult::LimitReached);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{3});

    // A book that no longer exists is reported distinctly rather than being
    // lumped in with a policy refusal.
    std::weak_ptr<Book> gone;
    {
        const auto temporary = make_book("gone");
        gone = temporary;
    }
    EXPECT_TRUE(gone.expired());
    EXPECT_EQ(ana->BorrowBook(gone), MemberBorrowResult::BookUnavailable);
}

TEST(MemberTest, LimitsArePolymorphic)
{
    const auto regular = std::make_shared<RegularMember>("Ana", 1);
    const auto premium = std::make_shared<PremiumMember>("Ben", 2);

    EXPECT_EQ(regular->MaxBooks(), std::size_t{3});
    EXPECT_EQ(premium->MaxBooks(), std::size_t{5});

    // LSP: identical code path through the base reference works for both,
    // differing only in the limit value.
    const Member &as_base_regular = *regular;
    const Member &as_base_premium = *premium;
    EXPECT_TRUE(as_base_regular.MaxBooks() < as_base_premium.MaxBooks());
    EXPECT_EQ(as_base_regular.BorrowedCount(), std::size_t{0});
    EXPECT_TRUE(!as_base_regular.ReachedLimit());
}

TEST(MemberTest, RegularMemberCappedAtThree)
{
    MyLibrary lib;
    stock(lib, 5);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    EXPECT_TRUE(lib.RegisterMember(ana));

    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "2"), BorrowResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "3"), BorrowResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{3});
    EXPECT_TRUE(ana->ReachedLimit());

    // The fourth is refused by the member's own policy.
    EXPECT_EQ(lib.BorrowBook(1, "4"), BorrowResult::BorrowLimitReached);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{3});
    // ...and the refusal left no half-written loan behind.
    EXPECT_EQ(lib.ActiveLoanCount(), std::size_t{3});
    EXPECT_TRUE(!lib.IsBorrowed("4"));
}

TEST(MemberTest, PremiumMemberCappedAtFive)
{
    MyLibrary lib;
    stock(lib, 7);
    const auto ben = std::make_shared<PremiumMember>("Ben", 2);
    EXPECT_TRUE(lib.RegisterMember(ben));

    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(lib.BorrowBook(2, std::to_string(i)), BorrowResult::Success);
    }
    EXPECT_EQ(ben->BorrowedCount(), std::size_t{5});
    EXPECT_EQ(lib.BorrowBook(2, "6"), BorrowResult::BorrowLimitReached);
}

// ------------------------------------------------------------------
// Library operations
// ------------------------------------------------------------------

TEST(LibraryOperationsTest, AddBookRejectsDuplicateIsbn)
{
    MyLibrary lib;
    EXPECT_EQ(lib.AddBook(make_book("same")), AddBookResult::Success);
    EXPECT_EQ(lib.AddBook(make_book("same")), AddBookResult::DuplicateIsbn);
    EXPECT_EQ(lib.GetRepository().size(), std::size_t{1});
    EXPECT_THROW((void)lib.AddBook(nullptr), std::invalid_argument);
}

TEST(LibraryOperationsTest, BorrowFailureModes)
{
    MyLibrary lib;
    stock(lib, 2);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto ben = std::make_shared<RegularMember>("Ben", 2);
    EXPECT_TRUE(lib.RegisterMember(ana));

    EXPECT_EQ(lib.BorrowBook(1, "nope"), BorrowResult::BookNotFound);
    EXPECT_EQ(lib.BorrowBook(99, "1"), BorrowResult::MemberNotRegistered);

    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::BookAlreadyBorrowed);

    // Also unavailable to a different member.
    EXPECT_TRUE(lib.RegisterMember(ben));
    EXPECT_EQ(lib.BorrowBook(2, "1"), BorrowResult::BookAlreadyBorrowed);
}

TEST(LibraryOperationsTest, ReturnBookFlow)
{
    MyLibrary lib;
    stock(lib, 2);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto ben = std::make_shared<RegularMember>("Ben", 2);
    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_TRUE(lib.RegisterMember(ben));

    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);

    EXPECT_EQ(lib.ReturnBook(1, "nope"), ReturnResult::BookNotFound);
    EXPECT_EQ(lib.ReturnBook(99, "1"), ReturnResult::MemberNotRegistered);
    EXPECT_EQ(lib.ReturnBook(1, "2"), ReturnResult::NotBorrowedByMember);
    EXPECT_EQ(lib.ReturnBook(2, "1"), ReturnResult::NotBorrowedByMember);

    EXPECT_EQ(lib.ReturnBook(1, "1"), ReturnResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{0});
    EXPECT_TRUE(!lib.IsBorrowed("1"));

    // Returning twice is not a success.
    EXPECT_EQ(lib.ReturnBook(1, "1"), ReturnResult::NotBorrowedByMember);

    // Back in circulation for somebody else.
    EXPECT_EQ(lib.BorrowBook(2, "1"), BorrowResult::Success);
}

TEST(LibraryOperationsTest, ReturnedBookFreesASlot)
{
    MyLibrary lib;
    stock(lib, 4);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    EXPECT_TRUE(lib.RegisterMember(ana));

    for (int i = 1; i <= 3; ++i)
    {
        EXPECT_EQ(lib.BorrowBook(1, std::to_string(i)), BorrowResult::Success);
    }
    EXPECT_EQ(lib.BorrowBook(1, "4"), BorrowResult::BorrowLimitReached);

    EXPECT_EQ(lib.ReturnBook(1, "2"), ReturnResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "4"), BorrowResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{3});
}

// ------------------------------------------------------------------
// ASSOCIATION consistency
// ------------------------------------------------------------------

TEST(AssociationTest, IsConsistentOnBothSides)
{
    MyLibrary lib;
    stock(lib, 2);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);

    // Member side sees the book...
    EXPECT_TRUE(ana->HasBorrowed("1"));
    EXPECT_EQ(ana->ListOfBorrowedBooks().size(), std::size_t{1});

    // ...and the library side sees both ends of the same relationship.
    const auto loan = lib.FindLoan("1");
    EXPECT_TRUE(loan.has_value());
    if (loan.has_value())
    {
        const auto borrowed_book = loan->mBook.lock();
        const auto borrower = loan->mBorrower.lock();
        EXPECT_TRUE(borrowed_book != nullptr);
        EXPECT_TRUE(borrower == ana);
        if (borrowed_book)
        {
            EXPECT_EQ(borrowed_book->ISBN(), std::string("1"));
        }
    }
}

// ------------------------------------------------------------------
// AGGREGATION: members come and go
// ------------------------------------------------------------------

TEST(AggregationTest, LibraryDoesNotOwnItsMembers)
{
    MyLibrary lib;
    stock(lib, 3);

    std::weak_ptr<Member> observer;
    {
        const auto visitor = std::make_shared<RegularMember>("Visitor", 7);
        observer = visitor;
        EXPECT_TRUE(lib.RegisterMember(visitor));
        EXPECT_EQ(lib.BorrowBook(7, "1"), BorrowResult::Success);
        EXPECT_EQ(lib.RegisteredMemberCount(), std::size_t{1});
        EXPECT_TRUE(!observer.expired());
    } // the client's shared_ptr dies here -- the member leaves

    // Because the library only held a weak_ptr, the member really is gone.
    // This is the proof that the relationship is aggregation, not composition.
    EXPECT_TRUE(observer.expired());
    EXPECT_EQ(lib.RegisteredMemberCount(), std::size_t{0});
    EXPECT_TRUE(!lib.IsRegistered(7));

    // The departure is detected rather than dangling, and pruning releases
    // the orphaned loan back into circulation.
    EXPECT_EQ(lib.PruneDepartedMembers(), std::size_t{1});
    EXPECT_EQ(lib.ActiveLoanCount(), std::size_t{0});
    EXPECT_TRUE(!lib.IsBorrowed("1"));

    // The book itself was never owned by the member, so it survives.
    EXPECT_TRUE(lib.GetRepository().Contains("1"));
    EXPECT_EQ(lib.GetRepository().size(), std::size_t{3});
}

TEST(AggregationTest, RegisterAndUnregister)
{
    MyLibrary lib;
    stock(lib, 2);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto duplicate_id = std::make_shared<PremiumMember>("Impostor", 1);

    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_TRUE(!lib.RegisterMember(duplicate_id)); // id already taken
    EXPECT_THROW((void)lib.RegisterMember(nullptr), std::invalid_argument);

    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);

    // Leaving releases held books, so no stale loan survives -- on EITHER side.
    EXPECT_TRUE(lib.UnregisterMember(1));
    EXPECT_TRUE(!lib.IsBorrowed("1"));
    EXPECT_EQ(lib.ActiveLoanCount(), std::size_t{0});
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{0});
    EXPECT_TRUE(!lib.UnregisterMember(1));
    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::MemberNotRegistered);
}

/// Regression test. The library releasing its loans is only half the job:
/// the departing member outlives its registration (the client still owns
/// it), so its own list has to be cleared too. Otherwise the member keeps
/// claiming books that are back on the shelf -- burning its borrow slots
/// forever and letting one copy be "held" by two people at once.
TEST(AggregationTest, UnregisteringLeavesNoStaleState)
{
    MyLibrary lib;
    stock(lib, 3);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto ben = std::make_shared<RegularMember>("Ben", 2);
    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_TRUE(lib.RegisterMember(ben));

    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "2"), BorrowResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{2});

    EXPECT_TRUE(lib.UnregisterMember(1));

    // Both ends of the association agree that Ana holds nothing.
    EXPECT_EQ(lib.ActiveLoanCount(), std::size_t{0});
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{0});
    EXPECT_TRUE(!ana->HasBorrowed("1"));
    EXPECT_EQ(ana->ListOfBorrowedBooks().size(), std::size_t{0});

    // The released copies really are back in circulation for someone else.
    EXPECT_EQ(lib.BorrowBook(2, "1"), BorrowResult::Success);

    // And re-joining gives Ana a clean slate rather than stale state.
    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_EQ(lib.BorrowBook(1, "2"), BorrowResult::Success);
    EXPECT_EQ(lib.BorrowBook(1, "3"), BorrowResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{2});
    EXPECT_TRUE(!ana->ReachedLimit());
}

/// A member is owned by the client, not by a library, so nothing stops it
/// joining two libraries. Leaving one must not disturb books borrowed from
/// the other -- which is why unregistering hands books back one ISBN at a
/// time instead of clearing the member's entire list.
TEST(AggregationTest, UnregisteringFromOneLibraryLeavesTheOtherIntact)
{
    MyLibrary town;
    MyLibrary campus;
    EXPECT_EQ(town.AddBook(make_book("town-1")), AddBookResult::Success);
    EXPECT_EQ(campus.AddBook(make_book("campus-1")), AddBookResult::Success);

    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    EXPECT_TRUE(town.RegisterMember(ana));
    EXPECT_TRUE(campus.RegisterMember(ana));

    EXPECT_EQ(town.BorrowBook(1, "town-1"), BorrowResult::Success);
    EXPECT_EQ(campus.BorrowBook(1, "campus-1"), BorrowResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{2});

    // Leaving the town library releases only the town book.
    EXPECT_TRUE(town.UnregisterMember(1));
    EXPECT_EQ(town.ActiveLoanCount(), std::size_t{0});
    EXPECT_TRUE(!ana->HasBorrowed("town-1"));

    // The campus loan is untouched -- on BOTH sides of the association.
    EXPECT_EQ(campus.ActiveLoanCount(), std::size_t{1});
    EXPECT_TRUE(ana->HasBorrowed("campus-1"));
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{1});
    EXPECT_TRUE(campus.IsBorrowed("campus-1"));

    // ...and it can still be returned normally.
    EXPECT_EQ(campus.ReturnBook(1, "campus-1"), ReturnResult::Success);
    EXPECT_EQ(ana->BorrowedCount(), std::size_t{0});
    EXPECT_EQ(campus.ActiveLoanCount(), std::size_t{0});
}

// ------------------------------------------------------------------
// COMPOSITION: the repository owns the books
// ------------------------------------------------------------------

TEST(CompositionTest, RepositoryIsSoleOwnerOfBooks)
{
    InMemoryBookRepository repo;

    std::weak_ptr<Book> observer;
    {
        auto book = make_book("9");
        observer = book;
        // Move in, so the repository really is the ONLY strong owner.
        EXPECT_TRUE(repo.Add(std::move(book)));
    }

    // The book survives the caller's handle going away: the repo owns it.
    EXPECT_TRUE(!observer.expired());
    EXPECT_TRUE(repo.Contains("9"));
    EXPECT_TRUE(!repo.Find("9").expired());

    // Removing from the sole owner genuinely destroys the book, and every
    // outstanding observer reports expiry instead of dangling.
    EXPECT_TRUE(repo.Remove("9"));
    EXPECT_TRUE(observer.expired());
    EXPECT_TRUE(!repo.Contains("9"));
    EXPECT_EQ(repo.size(), std::size_t{0});
    EXPECT_TRUE(!repo.Remove("9"));
}

TEST(CompositionTest, RemoveBookRefusesWhileOnLoan)
{
    MyLibrary lib;
    stock(lib, 2);
    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    EXPECT_TRUE(lib.RegisterMember(ana));
    EXPECT_EQ(lib.BorrowBook(1, "1"), BorrowResult::Success);

    EXPECT_EQ(lib.RemoveBook("nope"), RemoveBookResult::BookNotFound);
    EXPECT_EQ(lib.RemoveBook("1"), RemoveBookResult::BookIsBorrowed);
    EXPECT_EQ(lib.RemoveBook("2"), RemoveBookResult::Success);

    EXPECT_EQ(lib.ReturnBook(1, "1"), ReturnResult::Success);
    EXPECT_EQ(lib.RemoveBook("1"), RemoveBookResult::Success);
    EXPECT_EQ(lib.GetRepository().size(), std::size_t{0});
}

} // namespace
