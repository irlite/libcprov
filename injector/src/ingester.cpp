#include <sqlite3.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "model.hpp"
#include "picosha2.h"

namespace fs = std::filesystem;
using ChecksumsByFiles = std::unordered_map<std::string, std::string>;

sqlite3* create_and_open_db(const std::string& base_path) {
    fs::create_directories(base_path);
    std::string db_path = base_path + "/checksums.db";
    sqlite3* db = nullptr;
    sqlite3_open(db_path.c_str(), &db);
    const char* sql =
        "CREATE TABLE IF NOT EXISTS checksums (checksum TEXT PRIMARY KEY);";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    return db;
}

std::vector<bool> get_missing_checksums(
    sqlite3* db, const std::vector<std::string>& checksums) {
    std::vector<bool> missing;
    missing.reserve(checksums.size());
    const char* sql = "SELECT 1 FROM checksums WHERE checksum=? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    for (const auto& c : checksums) {
        sqlite3_bind_text(stmt, 1, c.c_str(), -1, SQLITE_TRANSIENT);
        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
        missing.push_back(!exists);
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
    return missing;
}

void insert_missing_checksums(sqlite3* db,
                              const std::vector<std::string>& checksums,
                              const std::vector<bool>& missing) {
    const char* sql = "INSERT OR IGNORE INTO checksums (checksum) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    for (size_t i = 0; i < checksums.size(); ++i) {
        if (!missing[i]) continue;
        sqlite3_bind_text(stmt, 1, checksums[i].c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
}

bool is_excluded_file(const fs::path& path,
                      const std::vector<std::string>& excluded_files) {
    auto name = path.filename().string();
    for (const auto& f : excluded_files)
        if (name == f) return true;
    return false;
}

bool is_in_excluded_dir(const fs::path& path,
                        const std::vector<std::string>& excluded_dirs) {
    for (const auto& part : path.parent_path()) {
        std::string name = part.string();
        for (const auto& d : excluded_dirs)
            if (name == d) return true;
    }
    return false;
}

bool is_in_root_dir(const fs::path& path, const fs::path& root) {
    auto rel = path.lexically_relative(root);
    return !rel.empty() && rel.string().find("..") != 0;
}

std::string sha256_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    picosha2::hash256_one_by_one hasher;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
        hasher.process(buffer, buffer + file.gcount());
    hasher.finish();
    std::vector<unsigned char> hash(picosha2::k_digest_size);
    hasher.get_hash_bytes(hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

void add_file(const fs::path& path, ChecksumsByFiles& map) {
    std::string hash = sha256_file(path);
    if (!hash.empty()) map[path.string()] = hash;
}

ChecksumsByFiles collect_files_after_processing(
    const fs::path& root, const std::vector<std::string>& excluded_dirs,
    const std::vector<std::string>& excluded_files,
    const ProcessedExecData& processed_exec_data) {
    ChecksumsByFiles checksums_by_files;
    for (const auto& [process_id, operation] :
         processed_exec_data.process_map) {
        for (const auto& [path, operations] : operation.operation_map) {
            if (operations.write && !operations.deleted &&
                is_in_root_dir(fs::path(path), root) &&
                !is_excluded_file(path, excluded_files) &&
                !is_in_excluded_dir(path, excluded_dirs)) {
                add_file(path, checksums_by_files);
            }
        }
    }
    return checksums_by_files;
}

void attach_checksums_after_processing(ChecksumsByFiles& checksums_by_files_end,
                                       ProcessedExecData& processed_exec_data) {
    for (auto& [process_id, operation] : processed_exec_data.process_map) {
        for (auto& [path, operations] : operation.operation_map) {
            if (operations.write && !operations.deleted) {
                operations.end_checksum = checksums_by_files_end[path];
            }
        }
    }
}

ChecksumsByFiles collect_directory_files(
    const fs::path& root, const std::vector<std::string>& excluded_dirs,
    const std::vector<std::string>& excluded_files) {
    ChecksumsByFiles checksums_by_files;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied);
    for (auto& entry : it) {
        const fs::path& path = entry.path();
        if (entry.is_directory()) {
            auto name = path.filename().string();
            for (const auto& d : excluded_dirs)
                if (name == d) {
                    it.disable_recursion_pending();
                    break;
                }
            continue;
        }
        if (!entry.is_regular_file()) continue;
        if (is_excluded_file(path, excluded_files)) continue;
        add_file(path, checksums_by_files);
    }
    return checksums_by_files;
}

void process_files(sqlite3* db, const std::string& checksums_dir,
                   const ChecksumsByFiles& cheksums_by_files) {
    std::vector<std::pair<std::string, std::string>> entries(
        cheksums_by_files.begin(), cheksums_by_files.end());
    std::vector<std::string> checksums;
    checksums.reserve(entries.size());
    for (const auto& [path, checksum] : entries) checksums.push_back(checksum);
    std::vector<bool> missing = get_missing_checksums(db, checksums);
    insert_missing_checksums(db, checksums, missing);
    for (size_t i = 0; i < entries.size(); ++i) {
        if (!missing[i]) continue;
        const auto& [path_str, checksum] = entries[i];
        fs::path path(path_str);
        fs::path target_dir =
            fs::path(checksums_dir) / "artifact_files" / checksum.substr(0, 2);
        fs::create_directories(target_dir);
        fs::copy_file(path, target_dir / checksum,
                      fs::copy_options::overwrite_existing);
    }
}

std::pair<std::vector<std::string>, std::vector<std::string>> parse_ignore_file(
    const fs::path& path) {
    std::vector<std::string> files, dirs;
    std::ifstream file(path);
    if (!file.is_open()) return {files, dirs};
    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (!line.empty() && line.back() == '/') {
            line.pop_back();
            dirs.push_back(line);
        } else
            files.push_back(line);
    }
    return {files, dirs};
}

void close_db_connection(sqlite3* db) { sqlite3_close(db); }
