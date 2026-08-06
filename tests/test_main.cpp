#include <library/Book.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using namespace library;

namespace
{

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

} // namespace
