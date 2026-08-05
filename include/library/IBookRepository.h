#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <library/Book.h>

namespace library
{

/// ================================ DIP ================================
///
/// MyLibrary depends on this abstraction, never on a concrete storage
/// class. That is what makes the library unit-testable: a test can inject
/// a stub or instrumented repository without touching real storage.
///
/// Note the return types: `Add` takes a `shared_ptr` (ownership moves IN,
/// the repository becomes the book's sole strong owner) while lookups hand
/// back `weak_ptr` (non-owning observers). The interface therefore makes
/// "the repository owns the books" impossible to misread.
class IBookRepository
{
  public:
    virtual ~IBookRepository() = default;

    IBookRepository(const IBookRepository &) = delete;
    IBookRepository &operator=(const IBookRepository &) = delete;
    IBookRepository(IBookRepository &&) = delete;
    IBookRepository &operator=(IBookRepository &&) = delete;

    /// @brief Takes ownership.
    /// @param aBook The book to add.
    /// \returns false if the ISBN is already present.
    [[nodiscard]] virtual bool Add(std::shared_ptr<Book> aBook) = 0;

    /// @brief Finds a book by ISBN.
    /// @param aISBN The ISBN to look up.
    /// \returns a non-owning observer, empty if absent.
    [[nodiscard]] virtual std::weak_ptr<Book> Find(const isbn &aISBN) const = 0;

    /// @brief Checks if a book is present by ISBN.
    /// @param aISBN The ISBN to look up.
    /// \returns true if the ISBN is present.
    [[nodiscard]] virtual bool Contains(const isbn &aISBN) const = 0;

    /// @brief Destroys the book, expiring every outstanding observer.
    /// @param aISBN The ISBN to remove.
    /// \returns false if the ISBN was not present.
    [[nodiscard]] virtual bool Remove(const isbn &aISBN) = 0;

    /// @brief  Returns the number of books in the repository.
    /// @throws std::runtime_error if the repository is in an invalid state.
    /// @return The number of books in the repository.
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    /// @brief Returns all books in the repository.
    /// @throws std::runtime_error if the repository is in an invalid state.
    /// @return A vector of weak pointers to all books in the repository.
    [[nodiscard]] virtual std::vector<std::weak_ptr<Book>> GetAll() const = 0;

  protected:
    /// @brief  Default constructor for the IBookRepository interface.
    /// note: This constructor is protected to prevent direct instantiation of the
    /// interface.
    IBookRepository() = default;
};

} // namespace library
