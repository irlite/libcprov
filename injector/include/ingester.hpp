#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "model.hpp"

namespace fs = std::filesystem;

using ChecksumsByFiles = std::unordered_map<std::string, std::string>;

sqlite3* create_and_open_db(const std::string& base_path);

ChecksumsByFiles collect_files_after_processing(
    const fs::path& root, const std::vector<std::string>& excluded_dirs,
    const std::vector<std::string>& excluded_files,
    const ProcessedExecData& processed_exec_data);

void attach_checksums_after_processing(ChecksumsByFiles& checksums_by_files,
                                       ProcessedExecData& processed_exec_data);

ChecksumsByFiles collect_directory_files(
    const fs::path& root, const std::vector<std::string>& excluded_dirs,
    const std::vector<std::string>& excluded_files);

void process_files(sqlite3* db, const std::string& checksums_dir,
                   const ChecksumsByFiles& map);

std::pair<std::vector<std::string>, std::vector<std::string>> parse_ignore_file(
    const fs::path& project_root);

void close_db_connection(sqlite3* db);
