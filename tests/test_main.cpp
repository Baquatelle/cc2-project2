#include <library/Book.h>
#include <library/Member.h>
#include <library/MyLibrary.h>
#include <library/PremiumMember.h>
#include <library/RegularMember.h>
#include <library/Results.h>

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

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

} // namespace
