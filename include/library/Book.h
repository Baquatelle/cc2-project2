#pragma once

#include <string>

#include <library/types.h>

namespace library
{

/// @brief Represents a book in the library.
class Book
{
  public:
    Book(std::string aTitle, std::string aAuthor, isbn aISBN);

    [[nodiscard]] const std::string &Title() const noexcept
    {
        return mTitle;
    }

    [[nodiscard]] const std::string &Author() const noexcept
    {
        return mAuthor;
    }

    [[nodiscard]] const isbn &ISBN() const noexcept
    {
        return mISBN;
    }

    /// @brief Sets the title of the book.
    /// @param aTitle The new title for the book.
    void SetTitle(std::string aTitle);

    /// @brief Sets the author of the book.
    /// @param aAuthor The new author for the book.
    void SetAuthor(std::string aAuthor);

    // Intentionally NO set_isbn(): the ISBN is the Book's identity and is
    // used as the repository's key. Allowing mutation would silently
    // desynchronise the container from its contents. Immutable identity is
    // an encapsulation decision, not an oversight.

  private:
    std::string mTitle;  //< The title of the book.
    std::string mAuthor; //< The author of the book.
    isbn        mISBN;   //< The ISBN of the book, which is the natural primary key for a book.
};

} // namespace library
