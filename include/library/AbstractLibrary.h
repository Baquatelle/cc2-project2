#pragma once

#include <memory>
#include <string>

#include <library/Results.h>
#include <library/types.h>

namespace library
{

class Book;

/// "A Library is a general concept, not a tangible library." This class
/// cannot be instantiated and holds no state -- an interface should not own
/// data. All storage lives in the concrete MyLibrary.
class AbstractLibrary
{
  public:
    virtual ~AbstractLibrary() = default;

    // Polymorphic base: block copy/move to prevent slicing.
    AbstractLibrary(const AbstractLibrary &) = delete;
    AbstractLibrary &operator=(const AbstractLibrary &) = delete;
    AbstractLibrary(AbstractLibrary &&) = delete;
    AbstractLibrary &operator=(AbstractLibrary &&) = delete;

    /// @brief Adds a book to the collection, taking ownership.
    /// @param aBook A shared pointer to the book to add.
    /// \returns AddBookResult::DuplicateIsbn if a book with the same ISBN is
    ///          already in the collection, otherwise AddBookResult::Success.
    [[nodiscard]] virtual AddBookResult AddBook(std::shared_ptr<Book> aBook) = 0;

    /// @brief Borrows a book from the collection.
    /// @param aMemberId The ID of the member borrowing the book.
    /// @param aISBN The ISBN of the book to borrow.
    /// @return BorrowResult indicating the outcome of the borrow operation.
    [[nodiscard]] virtual BorrowResult BorrowBook(member_id aMemberId, const isbn &aISBN) = 0;

    /// @brief Returns a book to the collection.
    /// @param aMemberId The ID of the member returning the book.
    /// @param aISBN The ISBN of the book to return.
    /// @return ReturnResult indicating the outcome of the return operation.
    [[nodiscard]] virtual ReturnResult ReturnBook(member_id aMemberId, const isbn &aISBN) = 0;

  protected:
    AbstractLibrary() = default;
};

} // namespace library
