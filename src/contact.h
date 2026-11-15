#pragma once
#include <string>
#include <vector>

// 联系方式类型：电话 / 邮箱 / 社交账号 / 通讯地址
enum class MethodType {
    Phone,
    Email,
    Social,
    Address
};

// 类型 <-> 字符串互转（用于展示与 CSV 列名）
std::string methodTypeToString(MethodType type);
MethodType parseMethodType(const std::string& text);

struct ContactMethod {
    MethodType type;
    std::string value;
};

struct Contact {
    int id = 0;
    std::string name;
    bool favorite = false;
    std::vector<ContactMethod> methods;

    // 取出指定类型的全部取值，多个用 "; " 拼接（用于 Excel 导出）
    std::string valuesByType(MethodType type) const;
};
