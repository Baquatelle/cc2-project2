#include <library/Book.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "utils.h"

namespace library
{

Book::Book(std::string aTitle, std::string aAuthor, isbn aISBN)
{
    REQUIRE_NOT_BLANK(aTitle);
    REQUIRE_NOT_BLANK(aAuthor);
    REQUIRE_NOT_BLANK(aISBN);

    mTitle = std::move(aTitle);
    mAuthor = std::move(aAuthor);
    mISBN = std::move(aISBN);
}

void Book::SetTitle(std::string aTitle)
{
    REQUIRE_NOT_BLANK(aTitle);
    mTitle = std::move(aTitle);
}

void Book::SetAuthor(std::string aAuthor)
{
    REQUIRE_NOT_BLANK(aAuthor);
    mAuthor = std::move(aAuthor);
}

} // namespace library
