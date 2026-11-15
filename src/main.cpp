#include "address_book.h"
#include "excel_io.h"
#include "contact.h"

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

const char* kDataFile = "data/contacts.csv";

void setupConsoleUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int readInt(const std::string& prompt, int fallback) {
    std::string line = readLine(prompt);
    try {
        return std::stoi(line);
    } catch (...) {
        return fallback;
    }
}

void printContact(const Contact& c) {
    std::cout << "  #" << c.id << "  " << c.name
              << (c.favorite ? "  [★ 收藏]" : "") << "\n";
    if (c.methods.empty()) {
        std::cout << "      (暂无联系方式)\n";
    }
    for (size_t i = 0; i < c.methods.size(); ++i) {
        const auto& m = c.methods[i];
        std::cout << "      " << (i + 1) << ". [" << methodTypeToString(m.type)
                  << "] " << m.value << "\n";
    }
}

void listContacts(const std::vector<const Contact*>& list, const std::string& emptyMsg) {
    if (list.empty()) {
        std::cout << emptyMsg << "\n";
        return;
    }
    for (const Contact* c : list) printContact(*c);
}

MethodType askMethodType() {
    std::cout << "  联系方式类型：1-电话  2-邮箱  3-社交账号  4-地址\n";
    int t = readInt("  请选择(1-4): ", 1);
    switch (t) {
        case 2: return MethodType::Email;
        case 3: return MethodType::Social;
        case 4: return MethodType::Address;
        default: return MethodType::Phone;
    }
}

void addContactFlow(AddressBook& book) {
    Contact c;
    c.id = book.nextId();
    c.name = readLine("  姓名: ");
    if (c.name.empty()) { std::cout << "  姓名不能为空，已取消。\n"; return; }

    std::string fav = readLine("  是否收藏? (y/N): ");
    c.favorite = (fav == "y" || fav == "Y" || fav == "是");

    std::cout << "  请填写联系方式（可留空，类型可多次添加）\n";
    char more = 'y';
    while (more == 'y' || more == 'Y') {
        MethodType type = askMethodType();
        std::string value = readLine("  内容: ");
        if (!value.empty()) c.methods.push_back({type, value});
        std::string again = readLine("  继续添加联系方式? (y/N): ");
        more = again.empty() ? 'n' : again[0];
    }
    book.addContact(c);
    std::cout << "  已添加联系人 #" << c.id << " " << c.name << "\n";
}

void exportFlow(const AddressBook& book) {
    std::string path = readLine("  导出路径(回车默认 data/contacts.csv): ");
    if (path.empty()) path = kDataFile;
    std::string err;
    if (excel_io::exportCsv(book, path, err))
        std::cout << "  导出成功: " << path << "（共 " << book.all().size() << " 条）\n";
    else
        std::cout << "  导出失败: " << err << "\n";
}

void importFlow(AddressBook& book) {
    std::string path = readLine("  导入路径(回车默认 data/contacts.csv): ");
    if (path.empty()) path = kDataFile;
    std::string err;
    int n = 0;
    if (excel_io::importCsv(book, path, err, n))
        std::cout << "  导入成功，共 " << n << " 条记录。\n";
    else
        std::cout << "  导入失败: " << err << "\n";
}

void printMenu() {
    std::cout << "\n====== 通讯录 Address Book ======\n"
              << " 1. 添加联系人\n"
              << " 2. 查看全部联系人\n"
              << " 3. 查看收藏联系人\n"
              << " 4. 按姓名搜索\n"
              << " 5. 标记 / 取消收藏\n"
              << " 6. 为联系人添加联系方式\n"
              << " 7. 删除某条联系方式\n"
              << " 8. 导出 Excel (CSV)\n"
              << " 9. 导入 Excel (CSV)\n"
              << "10. 删除联系人\n"
              << " 0. 保存并退出\n"
              << "=================================\n";
}

// ---------- 内置自测：导入导出往返、收藏、多联系方式 ----------
int runSelfTest(const fs::path& tmpDir) {
    int failures = 0;
    auto CHECK = [&](bool cond, const std::string& msg) {
        std::cout << (cond ? "[OK] " : "[FAIL] ") << msg << "\n";
        if (!cond) ++failures;
    };

    AddressBook book;
    Contact alice;
    alice.id = book.nextId();
    alice.name = "Alice";
    alice.favorite = true;
    alice.methods = {
        {MethodType::Phone, "13800000000"},
        {MethodType::Phone, "13900000000"},
        {MethodType::Email, "alice@example.com"},
        {MethodType::Social, "wx_alice"},
        {MethodType::Address, "Fuzhou, Fujian"}
    };
    Contact bob;
    bob.id = book.nextId();
    bob.name = "Bob";
    bob.favorite = false;
    bob.methods = {{MethodType::Phone, "10086"}};
    book.addContact(alice);
    book.addContact(bob);

    CHECK(book.all().size() == 2, "初始添加 2 条联系人");
    CHECK(book.favorites().size() == 1, "收藏人数为 1");

    fs::create_directories(tmpDir);
    fs::path file1 = tmpDir / "round1.csv";
    std::string err;
    CHECK(excel_io::exportCsv(book, file1.string(), err), "第一次导出成功");

    AddressBook book2;
    int imported = 0;
    CHECK(excel_io::importCsv(book2, file1.string(), err, imported), "第一次导入成功");
    CHECK(imported == 2, "导入条数 = 2");
    CHECK(book2.all().size() == 2, "导入后总数 = 2");
    CHECK(book2.favorites().size() == 1, "导入后收藏人数 = 1");

    const Contact* a = book2.searchByName("alice").empty() ? nullptr : book2.searchByName("alice")[0];
    CHECK(a != nullptr, "能搜索到 Alice");
    if (a) {
        CHECK(a->methods.size() == 5, "Alice 多联系方式共 5 条");
        CHECK(a->valuesByType(MethodType::Phone) == "13800000000; 13900000000",
              "同一类型多个电话正确拼接");
        CHECK(a->favorite == true, "Alice 收藏标记保留");
    }
    const Contact* b = book2.searchByName("bob").empty() ? nullptr : book2.searchByName("bob")[0];
    CHECK(b != nullptr && b->methods.size() == 1, "Bob 联系方式 1 条");

    fs::path file2 = tmpDir / "round2.csv";
    CHECK(excel_io::exportCsv(book2, file2.string(), err), "第二次导出成功（往返幂等）");

    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    std::ostringstream s1, s2;
    s1 << f1.rdbuf(); s2 << f2.rdbuf();
    CHECK(s1.str() == s2.str(), "两次导出内容完全一致（往返无丢失）");

    std::cout << (failures == 0 ? "\n自测全部通过" : "\n存在失败用例") << "\n";
    return failures;
}

} // namespace

int main(int argc, char** argv) {
    setupConsoleUtf8();

    // 命令行自测模式：address-book.exe --selftest [tmpDir]
    if (argc > 1 && std::string(argv[1]) == "--selftest") {
        fs::path tmp = (argc > 2) ? argv[2] : fs::temp_directory_path() / "address_book_selftest";
        return runSelfTest(tmp);
    }

    AddressBook book;
    std::string err;
    int imported = 0;
    if (fs::exists(kDataFile)) {
        if (excel_io::importCsv(book, kDataFile, err, imported))
            std::cout << "已从 " << kDataFile << " 加载 " << imported << " 条联系人。\n";
        else
            std::cout << "本地数据加载失败: " << err << "\n";
    }

    int choice = -1;
    while (choice != 0) {
        printMenu();
        choice = readInt("请输入选项: ", -1);
        switch (choice) {
            case 1: addContactFlow(book); break;
            case 2: {
                std::vector<const Contact*> all;
                for (const auto& c : book.all()) all.push_back(&c);
                listContacts(all, "通讯录为空。");
                break;
            }
            case 3: listContacts(book.favorites(), "暂无收藏联系人。"); break;
            case 4: {
                std::string kw = readLine("  输入姓名关键字: ");
                listContacts(book.searchByName(kw), "未找到匹配联系人。");
                break;
            }
            case 5: {
                int id = readInt("  输入联系人编号: ", -1);
                if (book.toggleFavorite(id)) std::cout << "  收藏状态已切换。\n";
                else std::cout << "  未找到该编号。\n";
                break;
            }
            case 6: {
                int id = readInt("  输入联系人编号: ", -1);
                MethodType type = askMethodType();
                std::string value = readLine("  内容: ");
                if (book.addMethod(id, {type, value})) std::cout << "  联系方式已添加。\n";
                else std::cout << "  未找到该编号。\n";
                break;
            }
            case 7: {
                int id = readInt("  输入联系人编号: ", -1);
                int idx = readInt("  联系方式序号: ", -1) - 1;
                if (book.removeMethod(id, idx)) std::cout << "  已删除该联系方式。\n";
                else std::cout << "  删除失败（编号或序号无效）。\n";
                break;
            }
            case 8: exportFlow(book); break;
            case 9: importFlow(book); break;
            case 10: {
                int id = readInt("  输入联系人编号: ", -1);
                if (book.removeContact(id)) std::cout << "  联系人已删除。\n";
                else std::cout << "  未找到该编号。\n";
                break;
            }
            case 0: break;
            default: std::cout << "  无效选项，请重试。\n"; break;
        }
    }

    fs::create_directories(fs::path(kDataFile).parent_path());
    if (excel_io::exportCsv(book, kDataFile, err))
        std::cout << "已保存到 " << kDataFile << "\n";
    else
        std::cout << "保存失败: " << err << "\n";
    return 0;
}
