#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <library/Book.h>
#include <library/IBookRepository.h>
#include <library/types.h>

namespace library
{

/// ==================== COMPOSITION (repository -> Book) ====================
///
/// Wraps and delegates to a standard container, as the brief asks.
///
/// `std::unordered_map<isbn, shared_ptr<Book>>`:
///   * ISBN is the natural primary key -> O(1) lookup, and duplicate ISBNs
///     become impossible by construction.
///   * `shared_ptr` is required, not incidental: `weak_ptr` observers can
///     only be created from a `shared_ptr`, and the association needs them.
///   * A `vector<Book>` would be wrong here -- reallocation invalidates
///     references, and the whole design depends on stable identity.
///
/// This repository is the *sole* strong owner of every Book. Everyone else
/// holds `weak_ptr`, so removing a book really does destroy it and every
/// outstanding observer correctly reports `expired()`.
class InMemoryBookRepository final : public IBookRepository
{
  public:
    InMemoryBookRepository() = default;

    [[nodiscard]] bool Add(std::shared_ptr<Book> aBook) override;

    [[nodiscard]] std::weak_ptr<Book> Find(const isbn &aISBN) const override;

    [[nodiscard]] bool Contains(const isbn &aISBN) const override;

    [[nodiscard]] bool Remove(const isbn &aISBN) override;

    [[nodiscard]] std::size_t size() const noexcept override;

    [[nodiscard]] std::vector<std::weak_ptr<Book>> GetAll() const override;

  private:
    std::unordered_map<isbn, std::shared_ptr<Book>> mBooks; ///< The sole owner of every book in the repository.
};

} // namespace library
