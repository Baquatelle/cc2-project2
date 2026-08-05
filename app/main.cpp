#include <library/Book.h>
#include <library/Member.h>
#include <library/PremiumMember.h>
#include <library/RegularMember.h>

#include <format>
#include <iostream>
#include <memory>

using namespace library;

int main()
{
    std::cout << "Library Management System (C++20)\n";

    const auto dune = std::make_shared<Book>("Dune", "Frank Herbert", "978-0441013593");
    std::cout << std::format("book: {} by {} [{}]\n", dune->Title(), dune->Author(), dune->ISBN());

    const auto ana = std::make_shared<RegularMember>("Ana", 1);
    const auto ben = std::make_shared<PremiumMember>("Ben", 2);
    std::cout << std::format("{} is a {} member (limit {})\n", ana->Name(), ana->Category(), ana->MaxBooks());
    std::cout << std::format("{} is a {} member (limit {})\n", ben->Name(), ben->Category(), ben->MaxBooks());

    const MemberBorrowResult result = ana->BorrowBook(dune);
    std::cout << std::format("{} borrows {} -> {}\n", ana->Name(), dune->Title(),
                             to_string(result));
    std::cout << std::format("{} now holds {}/{} book(s)\n", ana->Name(), ana->BorrowedCount(), ana->MaxBooks());

    std::cout << "\nDone.\n";
    return 0;
}
