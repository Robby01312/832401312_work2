#include "address_book.h"
#include <algorithm>
#include <cctype>

namespace {
std::string toLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
    return out;
}
} // namespace

void AddressBook::addContact(const Contact& contact) {
    contacts_.push_back(contact);
}

bool AddressBook::removeContact(int id) {
    auto it = std::find_if(contacts_.begin(), contacts_.end(),
        [id](const Contact& c) { return c.id == id; });
    if (it == contacts_.end()) return false;
    contacts_.erase(it);
    return true;
}

Contact* AddressBook::findById(int id) {
    for (auto& c : contacts_) if (c.id == id) return &c;
    return nullptr;
}

const Contact* AddressBook::findById(int id) const {
    for (const auto& c : contacts_) if (c.id == id) return &c;
    return nullptr;
}

std::vector<const Contact*> AddressBook::searchByName(const std::string& keyword) const {
    std::vector<const Contact*> result;
    const std::string kw = toLower(keyword);
    for (const auto& c : contacts_) {
        if (kw.empty() || toLower(c.name).find(kw) != std::string::npos)
            result.push_back(&c);
    }
    return result;
}

bool AddressBook::toggleFavorite(int id) {
    Contact* c = findById(id);
    if (!c) return false;
    c->favorite = !c->favorite;
    return true;
}

bool AddressBook::addMethod(int id, ContactMethod method) {
    Contact* c = findById(id);
    if (!c) return false;
    c->methods.push_back(std::move(method));
    return true;
}

bool AddressBook::removeMethod(int id, int methodIndex) {
    Contact* c = findById(id);
    if (!c) return false;
    if (methodIndex < 0 || methodIndex >= static_cast<int>(c->methods.size())) return false;
    c->methods.erase(c->methods.begin() + methodIndex);
    return true;
}

const std::vector<Contact>& AddressBook::all() const {
    return contacts_;
}

std::vector<const Contact*> AddressBook::favorites() const {
    std::vector<const Contact*> result;
    for (const auto& c : contacts_)
        if (c.favorite) result.push_back(&c);
    return result;
}

int AddressBook::nextId() const {
    int maxId = 0;
    for (const auto& c : contacts_) maxId = std::max(maxId, c.id);
    return maxId + 1;
}
