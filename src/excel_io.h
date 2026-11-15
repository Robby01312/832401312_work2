#pragma once
#include "address_book.h"
#include <string>

// Excel 导入导出模块。
// 采用与 Excel 完全兼容的 UTF-8 CSV（.csv）格式：
//   一行一个联系人，列依次为 Name, Bookmark, Phone, Email, Social, Address；
//   同一类型的多个联系方式用分号 "; " 分隔。
namespace excel_io {

// 将通讯录导出到指定路径；成功返回 true，失败时 err 写入原因。
bool exportCsv(const AddressBook& book, const std::string& path, std::string& err);

// 从指定 CSV 文件读入联系人并追加到通讯录；importedCount 为成功导入条数。
bool importCsv(AddressBook& book, const std::string& path, std::string& err, int& importedCount);

} // namespace excel_io
