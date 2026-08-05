#include <library/AbstractLibrary.h>
#include <library/Book.h>
#include <library/InMemoryBookRepository.h>
#include <library/Member.h>
#include <library/MyLibrary.h>
#include <library/PremiumMember.h>
#include <library/RegularMember.h>
#include <library/Results.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace library;

namespace
{

void heading(std::string_view aText)
{
    std::cout << std::format("\n{:=^70}\n", std::format(" {} ", aText));
}

void show_member(const Member &aMember)
{
    std::cout << std::format("  {} (#{}, {}) holds {}/{}\n", aMember.Name(), aMember.MemberId(), aMember.Category(),
                             aMember.BorrowedCount(), aMember.MaxBooks());
    for (const std::weak_ptr<Book> &entry : aMember.ListOfBorrowedBooks())
    {
        // Promote -> use -> drop: never dereference an unlocked weak_ptr.
        if (const std::shared_ptr<Book> book = entry.lock())
        {
            std::cout << std::format("      - {} [{}]\n", book->Title(), book->ISBN());
        }
        else
        {
            std::cout << "      - <book no longer exists>\n";
        }
    }
}

void try_borrow(AbstractLibrary &aLib, const Member &aMember, const isbn &aISBN)
{
    const BorrowResult result = aLib.BorrowBook(aMember.MemberId(), aISBN);
    std::cout << std::format("  borrow {:<14} by {:<6} -> {}\n", aISBN, aMember.Name(), to_string(result));
}

void try_return(AbstractLibrary &aLib, const Member &aMember, const isbn &aISBN)
{
    const ReturnResult result = aLib.ReturnBook(aMember.MemberId(), aISBN);
    std::cout << std::format("  return {:<14} by {:<6} -> {}\n", aISBN, aMember.Name(), to_string(result));
}

/// Lists what is on the shelf and who holds it, read back from the library
/// itself rather than from a local list -- so the output reflects real state.
/// Sorted by title because the repository's map order is unspecified.
void show_shelf(const MyLibrary &lib)
{
    std::vector<std::shared_ptr<Book>> shelf;
    for (const std::weak_ptr<Book> &entry : lib.GetRepository().GetAll())
    {
        if (std::shared_ptr<Book> book = entry.lock())
        {
            shelf.push_back(std::move(book));
        }
    }
    std::sort(shelf.begin(), shelf.end(), [](const std::shared_ptr<Book> &lhs, const std::shared_ptr<Book> &rhs) {
        return lhs->Title() < rhs->Title();
    });

    for (const std::shared_ptr<Book> &book : shelf)
    {
        if (const auto loan = lib.FindLoan(book->ISBN()))
        {
            const std::shared_ptr<Member> borrower = loan->mBorrower.lock();
            std::cout << std::format("  {:<18} on loan to {}\n", book->Title(),
                                     borrower ? borrower->Name() : "<departed>");
        }
        else
        {
            std::cout << std::format("  {:<18} available\n", book->Title());
        }
    }
}

} // namespace

int main()
{
    std::cout << "Library Management System (C++20)\n";

    // COMPOSITION + DIP: ownership of the repository is transferred in, so
    // the library exclusively owns it (filled diamond) while still depending
    // only on the IBookRepository abstraction.
    MyLibrary Library(std::make_unique<InMemoryBookRepository>());

    // Clients talk to the general concept where they can.
    AbstractLibrary &as_concept = Library;

    heading("Stocking the shelves");
    const std::vector<std::pair<std::string, std::string>> catalogue{
        {"Dune", "978-0441013593"},     {"Neuromancer", "978-0441569595"},      {"Snow Crash", "978-0553380958"},
        {"Hyperion", "978-0553283686"}, {"Foundation", "978-0553293357"},       {"Ubik", "978-0547572291"},
        {"Solaris", "978-0156027601"},  {"The Dispossessed", "978-0060512750"}, {"Blindsight", "978-0765319647"},
        {"Anathem", "978-0061474094"},
    };
    for (const auto &[Title, isbn] : catalogue)
    {
        const AddBookResult result = as_concept.AddBook(std::make_shared<Book>(Title, "Various", isbn));
        std::cout << std::format("  add {:<18} [{}] -> {}\n", Title, isbn, to_string(result));
    }
    // Duplicate ISBNs are impossible: the container key enforces it.
    std::cout << std::format(
        "  add {:<18} [{}] -> {}   (same ISBN twice)\n", "Dune", "978-0441013593",
        to_string(as_concept.AddBook(std::make_shared<Book>("Dune", "Herbert", "978-0441013593"))));

    heading("Members arrive (aggregation)");
    // The client owns the members via shared_ptr; the library only observes
    // them through weak_ptr. Members come and go, the library persists.
    const auto                                 ana = std::make_shared<RegularMember>("Ana", 1);
    const auto                                 ben = std::make_shared<PremiumMember>("Ben", 2);
    const std::vector<std::shared_ptr<Member>> arrivals{ana, ben};
    for (const std::shared_ptr<Member> &member : arrivals)
    {
        const bool registered = Library.RegisterMember(member);
        std::cout << std::format("  register {:<6} ({} member, limit {}) -> {}\n", member->Name(), member->Category(),
                                 member->MaxBooks(), registered ? "ok" : "already registered");
    }

    heading("Regular member is capped at 3");
    try_borrow(as_concept, *ana, "978-0441013593"); // Dune
    try_borrow(as_concept, *ana, "978-0441569595"); // Neuromancer
    try_borrow(as_concept, *ana, "978-0553380958"); // Snow Crash
    try_borrow(as_concept, *ana,
               "978-0553283686"); // Hyperion -- available, but she is full
    show_member(*ana);

    heading("Premium member is capped at 5");
    // Identical code path, different limit -- OCP and LSP in action.
    try_borrow(as_concept, *ben, "978-0553283686"); // Hyperion
    try_borrow(as_concept, *ben, "978-0553293357"); // Foundation
    try_borrow(as_concept, *ben, "978-0547572291"); // Ubik
    try_borrow(as_concept, *ben, "978-0156027601"); // Solaris
    try_borrow(as_concept, *ben,
               "978-0060512750"); // The Dispossessed -- his fifth
    try_borrow(as_concept, *ben,
               "978-0765319647"); // Blindsight -- available, but he is full
    show_member(*ben);

    heading("Expected failure modes");
    try_borrow(as_concept, *ben, "978-0441013593"); // Ana is holding Dune
    try_borrow(as_concept, *ben, "000-0000000000"); // no such book
    try_return(as_concept, *ben, "978-0441013593"); // not his to return
    const auto ghost = std::make_shared<RegularMember>("Ghost", 99);
    try_borrow(as_concept, *ghost, "978-0441013593"); // never registered

    heading("Returning frees a slot");
    try_return(as_concept, *ana, "978-0441569595"); // hands back Neuromancer
    try_borrow(as_concept, *ana, "978-0765319647"); // a different book now fits
    show_member(*ana);

    heading("Association is visible from the library side");
    show_shelf(Library);

    heading("A member leaves without returning anything");
    {
        const auto visitor = std::make_shared<RegularMember>("Cleo", 3);
        const bool registered = Library.RegisterMember(visitor);
        std::cout << std::format("  register Cleo -> {}\n", registered ? "ok" : "failed");
        try_borrow(as_concept, *visitor, "978-0061474094"); // Anathem
        std::cout << std::format("  members alive: {}, active loans: {}\n", Library.RegisteredMemberCount(),
                                 Library.ActiveLoanCount());
    } // Cleo's shared_ptr dies here -- she walks out still holding Anathem

    // The library held only a weak_ptr, so Cleo is genuinely gone. Her
    // absence is *detected*, not dangling -- that is the payoff of weak_ptr
    // over a raw pointer.
    std::cout << std::format("  after Cleo leaves -> members alive: {}\n", Library.RegisteredMemberCount());
    const std::size_t pruned = Library.PruneDepartedMembers();
    std::cout << std::format("  pruned {} departed member(s), releasing their loans\n", pruned);
    std::cout << std::format("  active loans now: {}   (Anathem is back on the shelf)\n", Library.ActiveLoanCount());

    heading("Books outlive their borrowers");
    // Books are owned by the repository, never by a member, so Cleo leaving
    // destroyed nothing on the shelf.
    std::cout << std::format("  books in repository: {}\n", Library.GetRepository().size());
    std::cout << std::format("  remove Dune    (Ana is holding it) -> {}\n",
                             to_string(Library.RemoveBook("978-0441013593")));
    std::cout << std::format("  remove Anathem (nobody holds it)   -> {}\n",
                             to_string(Library.RemoveBook("978-0061474094")));
    std::cout << std::format("  books in repository: {}\n", Library.GetRepository().size());

    std::cout << "\nDone.\n";
    return 0;
}
