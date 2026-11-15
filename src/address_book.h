#pragma once
#include "contact.h"
#include <vector>
#include <string>

// 通讯录核心业务逻辑：增删改查、收藏、多联系方式管理
class AddressBook {
public:
    void addContact(const Contact& contact);
    bool removeContact(int id);

    Contact* findById(int id);
    const Contact* findById(int id) const;

    // 按姓名关键字模糊查找（大小写不敏感）
    std::vector<const Contact*> searchByName(const std::string& keyword) const;

    bool toggleFavorite(int id);
    bool addMethod(int id, ContactMethod method);
    bool removeMethod(int id, int methodIndex);

    const std::vector<Contact>& all() const;
    std::vector<const Contact*> favorites() const;
    int nextId() const;

private:
    std::vector<Contact> contacts_;
};
