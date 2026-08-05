#pragma once

#include <library/Member.h>

namespace library
{

/// Premium members may hold up to 5 books.
class PremiumMember final : public Member
{
  public:
    static constexpr std::size_t kMaxBooks = 5;

    PremiumMember(std::string aName, int aMemberId) : Member(std::move(aName), aMemberId)
    {
    }

    [[nodiscard]] std::size_t MaxBooks() const noexcept override
    {
        return kMaxBooks;
    }

    [[nodiscard]] std::string_view Category() const noexcept override
    {
        return "Premium";
    }
};

} // namespace library
