#include "excel_io.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <string>

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string toLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
    return out;
}

// RFC 4180 字段转义：含逗号、引号、换行时用双引号包裹，内部引号翻倍
std::string csvEscape(const std::string& field) {
    if (field.find_first_of(",\"\n\r") == std::string::npos) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

// 将整段 CSV 文本解析为二维表（支持引号内逗号、转义引号、引号内换行）
std::vector<std::vector<std::string>> parseCsv(const std::string& text) {
    std::vector<std::vector<std::string>> records;
    std::vector<std::string> row;
    std::string field;
    bool inQuote = false;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (inQuote) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuote = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                inQuote = true;
            } else if (c == ',') {
                row.push_back(field);
                field.clear();
            } else if (c == '\n') {
                row.push_back(field);
                field.clear();
                records.push_back(row);
                row.clear();
            } else if (c != '\r') {
                field += c;
            }
        }
    }
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        records.push_back(row);
    }
    return records;
}

// 在表头中查找列索引，匹配任一别名；找不到返回 -1
int findColumn(const std::vector<std::string>& header, std::initializer_list<const char*> aliases) {
    for (size_t i = 0; i < header.size(); ++i) {
        std::string h = toLower(trim(header[i]));
        for (const char* alias : aliases) {
            if (h == toLower(alias)) return static_cast<int>(i);
        }
    }
    return -1;
}

// 按分号拆分多个联系方式
std::vector<std::string> splitValues(const std::string& cell) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : cell) {
        if (c == ';' || c == '；') {
            std::string t = trim(cur);
            if (!t.empty()) out.push_back(t);
            cur.clear();
        } else {
            cur += c;
        }
    }
    std::string t = trim(cur);
    if (!t.empty()) out.push_back(t);
    return out;
}

bool isFavorite(const std::string& cell) {
    std::string s = toLower(trim(cell));
    return s == "1" || s == "true" || s == "yes" || s == "y" || s == "是" || s == "收藏";
}

std::string readAll(const std::string& path, std::string& err) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { err = "无法打开文件: " + path; return ""; }
    std::ostringstream oss;
    oss << in.rdbuf();
    std::string text = oss.str();
    // 去掉 UTF-8 BOM
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
    return text;
}

} // namespace

namespace excel_io {

bool exportCsv(const AddressBook& book, const std::string& path, std::string& err) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) { err = "无法写入文件: " + path; return false; }

    // 写 UTF-8 BOM，保证 Excel 打开中文不乱码
    out.put(static_cast<char>(0xEF));
    out.put(static_cast<char>(0xBB));
    out.put(static_cast<char>(0xBF));

    out << "Name,Bookmark,Phone,Email,Social,Address\n";
    for (const auto& c : book.all()) {
        std::string line;
        line += csvEscape(c.name) + ",";
        line += (c.favorite ? "1" : "0") + std::string(",");
        line += csvEscape(c.valuesByType(MethodType::Phone)) + ",";
        line += csvEscape(c.valuesByType(MethodType::Email)) + ",";
        line += csvEscape(c.valuesByType(MethodType::Social)) + ",";
        line += csvEscape(c.valuesByType(MethodType::Address));
        out << line << "\n";
    }
    if (!out) { err = "写入过程中出错: " + path; return false; }
    return true;
}

bool importCsv(AddressBook& book, const std::string& path, std::string& err, int& importedCount) {
    importedCount = 0;
    std::string text = readAll(path, err);
    if (!err.empty()) return false;

    auto records = parseCsv(text);
    if (records.empty()) { err = "文件为空或没有有效记录"; return false; }

    const auto& header = records[0];
    int colName  = findColumn(header, {"name", "姓名"});
    int colBook  = findColumn(header, {"bookmark", "favorite", "收藏", "书签"});
    int colPhone = findColumn(header, {"phone", "tel", "电话", "手机"});
    int colEmail = findColumn(header, {"email", "mail", "邮箱"});
    int colSocial= findColumn(header, {"social", "社交", "账号"});
    int colAddr  = findColumn(header, {"address", "addr", "地址", "住址"});

    if (colName == -1) { err = "缺少 Name/姓名 列，无法导入"; return false; }

    auto cell = [](const std::vector<std::string>& row, int idx) -> std::string {
        if (idx < 0 || idx >= static_cast<int>(row.size())) return "";
        return trim(row[idx]);
    };

    for (size_t r = 1; r < records.size(); ++r) {
        const auto& row = records[r];
        std::string name = cell(row, colName);
        if (name.empty()) continue; // 跳过空行

        Contact c;
        c.id = book.nextId();
        c.name = name;
        c.favorite = colBook >= 0 && isFavorite(cell(row, colBook));

        for (const auto& v : splitValues(cell(row, colPhone)))
            c.methods.push_back({MethodType::Phone, v});
        for (const auto& v : splitValues(cell(row, colEmail)))
            c.methods.push_back({MethodType::Email, v});
        for (const auto& v : splitValues(cell(row, colSocial)))
            c.methods.push_back({MethodType::Social, v});
        for (const auto& v : splitValues(cell(row, colAddr)))
            c.methods.push_back({MethodType::Address, v});

        book.addContact(c);
        ++importedCount;
    }
    return true;
}

} // namespace excel_io
