#include "contact.h"
#include <cctype>

std::string methodTypeToString(MethodType type) {
    switch (type) {
        case MethodType::Phone:   return "Phone";
        case MethodType::Email:   return "Email";
        case MethodType::Social:  return "Social";
        case MethodType::Address: return "Address";
    }
    return "Unknown";
}

MethodType parseMethodType(const std::string& text) {
    std::string s;
    s.reserve(text.size());
    for (char c : text) s.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));

    if (s == "phone" || s == "tel" || s == "mobile" || s == "电话" || s == "手机")
        return MethodType::Phone;
    if (s == "email" || s == "e-mail" || s == "mail" || s == "邮箱" || s == "邮件")
        return MethodType::Email;
    if (s == "social" || s == "wechat" || s == "weibo" || s == "qq" || s == "社交" || s == "账号")
        return MethodType::Social;
    if (s == "address" || s == "addr" || s == "住址" || s == "地址" || s == "通讯地址")
        return MethodType::Address;
    // 未知类型默认归为电话，避免导入丢数据
    return MethodType::Phone;
}

std::string Contact::valuesByType(MethodType type) const {
    std::string out;
    bool first = true;
    for (const auto& m : methods) {
        if (m.type == type && !m.value.empty()) {
            if (!first) out += "; ";
            out += m.value;
            first = false;
        }
    }
    return out;
}
