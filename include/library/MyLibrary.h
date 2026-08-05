#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <library/AbstractLibrary.h>
#include <library/Book.h>
#include <library/IBookRepository.h>
#include <library/Member.h>
#include <library/Results.h>
#include <library/types.h>

namespace library
{

/// The tangible library.
///
/// ===================== THE ONE OWNERSHIP RULE =====================
///
///   Exactly one strong owner per object. Every other reference is weak.
///
/// Apply that and the strong-reference graph is acyclic *by construction*:
/// the classic shared_ptr cycle is not "broken" here, it never exists.
/// No raw pointers are stored anywhere; lifetime is encoded in the types.
///
///   MyLibrary  --unique_ptr-->  IBookRepository   COMPOSITION
///   repository --shared_ptr-->  Book              COMPOSITION (sole owner)
///   client     --shared_ptr-->  Member            client owns members
///   MyLibrary  --weak_ptr---->  Member            AGGREGATION
///   Member     --weak_ptr---->  Book              ASSOCIATION
///   MyLibrary  --weak_ptr x2->  Book + Member     ASSOCIATION (bidirectional)
///
/// The type system *is* the UML diagram: a `unique_ptr` member is the
/// filled diamond, a `weak_ptr` is the plain line.
class MyLibrary final : public AbstractLibrary
{
  public:
    /// ==================== ASSOCIATION (library side) ====================
    ///
    /// Both ends are non-owning. The library observes the relationship
    /// between a Book and a Member without owning either.
    struct Loan
    {
        std::weak_ptr<Book>   mBook;     // owned by the repository
        std::weak_ptr<Member> mBorrower; // owned by the client
    };

    /// Classic composition: the whole creates its own part.
    MyLibrary();

    /// @note Ownership is *transferred in*, which is still
    /// composition: UML composition is about exclusive lifetime ownership,
    /// not about who called `new`. A `unique_ptr` member cannot be shared
    /// and dies with the library, which is exactly the filled diamond.
    /// (A `shared_ptr` member would instead mean aggregation.)
    ///
    /// \throws std::invalid_argument if \p repository is null.
    explicit MyLibrary(std::unique_ptr<IBookRepository> aRepository);

    // --- AbstractLibrary ---
    // [[nodiscard]] is repeated on the overrides: it is not inherited when a
    // call is made through the derived type.
    [[nodiscard]] AddBookResult AddBook(std::shared_ptr<Book> aBook) override;
    [[nodiscard]] BorrowResult  BorrowBook(member_id aMemberId, const isbn &aISBN) override;
    [[nodiscard]] ReturnResult  ReturnBook(member_id aMemberId, const isbn &aISBN) override;

    // --- AGGREGATION: members come and go ---

    /// Stores only a `weak_ptr`: the library does not own its members.
    /// \returns false if that member_id is already registered.
    /// \throws std::invalid_argument if \p member is null.
    [[nodiscard]] bool RegisterMember(const std::shared_ptr<Member> &member);

    /// \returns false if the member was not registered.
    [[nodiscard]] bool UnregisterMember(member_id aMemberId);

    [[nodiscard]] bool IsRegistered(member_id aMemberId) const;

    /// Counts registered members that are still alive.
    [[nodiscard]] std::size_t RegisteredMemberCount() const;

    /// Self-healing sweep for members destroyed without unregistering.
    /// Their outstanding loans are released back into circulation, keeping
    /// the two sides of the association consistent.
    /// \returns how many departed members were removed.
    std::size_t PruneDepartedMembers();

    // --- Concrete-only extras (kept off AbstractLibrary for ISP) ---

    /// Refuses to remove a book that is currently on loan, which is what
    /// keeps "the repository is the sole owner" safe in practice.
    [[nodiscard]] RemoveBookResult RemoveBook(const isbn &aISBN);

    [[nodiscard]] bool IsBorrowed(const isbn &aISBN) const;

    [[nodiscard]] std::optional<Loan> FindLoan(const isbn &aISBN) const;

    [[nodiscard]] std::size_t ActiveLoanCount() const noexcept
    {
        return mLoans.size();
    }

    [[nodiscard]] const IBookRepository &GetRepository() const noexcept
    {
        return *mRepository;
    }

  private:
    /// Promote -> use -> drop. Returns null if the member is unregistered
    /// or has been destroyed.
    [[nodiscard]] std::shared_ptr<Member> LookupMember(member_id aMemberId) const;

    std::unique_ptr<IBookRepository>                     mRepository; // COMPOSITION
    std::unordered_map<member_id, std::weak_ptr<Member>> mMembers;    // AGGREGATION
    std::unordered_map<isbn, Loan>                       mLoans;      // ASSOCIATION
    // Keying mLoans by ISBN makes the container enforce the business rule
    // "at most one active loan per copy", and answers availability in O(1).
};

} // namespace library
