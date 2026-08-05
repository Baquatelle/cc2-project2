#include <library/InMemoryBookRepository.h>

#include <stdexcept>
#include <utility>

namespace library
{

bool InMemoryBookRepository::Add(std::shared_ptr<Book> aBook)
{
    if (!aBook)
    {
        throw std::invalid_argument("InMemoryBookRepository::Add: aBook must not be null");
    }

    const std::string isbn = aBook->ISBN();
    return mBooks.emplace(isbn, std::move(aBook)).second;
}

std::weak_ptr<Book> InMemoryBookRepository::Find(const isbn &aISBN) const
{
    const auto it = mBooks.find(aISBN);

    if (it == mBooks.end())
    {
        return {};
    }

    return it->second; // shared_ptr -> weak_ptr: hand out a non-owning view
}

bool InMemoryBookRepository::Contains(const isbn &aISBN) const
{
    return mBooks.find(aISBN) != mBooks.end();
}

bool InMemoryBookRepository::Remove(const isbn &aISBN)
{
    return mBooks.erase(aISBN) > 0;
}

std::size_t InMemoryBookRepository::size() const noexcept
{
    return mBooks.size();
}

std::vector<std::weak_ptr<Book>> InMemoryBookRepository::GetAll() const
{
    std::vector<std::weak_ptr<Book>> Results;
    Results.reserve(mBooks.size());

    for (const auto &entry : mBooks)
    {
        Results.emplace_back(entry.second);
    }

    return Results;
}

} // namespace library
