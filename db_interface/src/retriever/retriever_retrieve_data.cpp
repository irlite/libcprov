#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config_parser.hpp"
#include "retriever/retriever_model.hpp"

namespace fs = std::filesystem;

std::string get_prov_artifacts_path() {
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    return config.prov_artifacts_path;
}

std::unordered_map<std::string, std::string> get_state_before_exec(
    OrderedOperationsPerExecs ordered_operations_per_execs, uint64_t exec_id,
    std::vector<uint64_t> ordered_exec_ids) {
    auto it = std::lower_bound(ordered_exec_ids.begin(), ordered_exec_ids.end(),
                               exec_id);
    std::vector<uint64_t> exec_ids_before_exec_reversed(
        std::make_reverse_iterator(it), ordered_exec_ids.rend());
    std::unordered_set<std::string> deleted_checksums;
    std::unordered_set<std::string> changed_checksums;
    std::unordered_map<std::string, std::string> checksums_to_files;
    OperationMap current_operation_map;
    std::string start_checksum;
    std::string end_checksum;
    std::string checksum_to_be_added;
    for (uint64_t current_exec_id : exec_ids_before_exec_reversed) {
        current_operation_map = ordered_operations_per_execs[current_exec_id];
        for (auto& [path, operations] : current_operation_map) {
            start_checksum = operations.start_checksum;
            end_checksum = operations.end_checksum;
            if (start_checksum == "" && end_checksum == "") {
                continue;
            }
            checksum_to_be_added =
                end_checksum != "" ? end_checksum : start_checksum;
            if (!checksums_to_files.contains(checksum_to_be_added) &&
                !deleted_checksums.contains(checksum_to_be_added) &&
                !changed_checksums.contains(checksum_to_be_added)) {
                if (operations.deleted) {
                    deleted_checksums.insert(start_checksum);
                } else {
                    if (end_checksum != "") {
                        checksums_to_files[end_checksum] = path;
                        if (start_checksum != "") {
                            changed_checksums.insert(start_checksum);
                        }
                    } else {
                        checksums_to_files[start_checksum] = path;
                    }
                }
            }
        }
    }
    return checksums_to_files;
}

std::unordered_map<std::string, std::string> get_state_after_exec(
    OrderedOperationsPerExecs ordered_operations_per_execs, uint64_t exec_id,
    std::vector<uint64_t> ordered_exec_ids) {
    auto it = std::lower_bound(ordered_exec_ids.begin(), ordered_exec_ids.end(),
                               exec_id);
    std::vector<uint64_t> exec_ids_after_exec(it, ordered_exec_ids.end());
    std::unordered_set<std::string> write_checksums;
    std::unordered_map<std::string, std::string> checksums_to_files;
    OperationMap current_operation_map;
    std::string start_checksum;
    std::string end_checksum;
    for (uint64_t current_exec_id : exec_ids_after_exec) {
        current_operation_map = ordered_operations_per_execs[current_exec_id];
        for (auto& [path, operations] : current_operation_map) {
            start_checksum = operations.start_checksum;
            end_checksum = operations.end_checksum;
            if ((start_checksum != "" && end_checksum != "") &&
                !checksums_to_files.contains(start_checksum) &&
                !write_checksums.contains(start_checksum)) {
                if (operations.write) {
                    write_checksums.insert(end_checksum);
                }
                if (start_checksum != "") {
                    checksums_to_files[start_checksum] = path;
                }
            }
        }
    }
    return checksums_to_files;
}

void move_artifact(const std::string& prov_artifacts_path,
                   const std::string& checksum,
                   const std::string& target_path) {
    fs::path src =
        fs::path(prov_artifacts_path) / checksum.substr(0, 2) / checksum;
    fs::path dst(target_path);
    fs::create_directories(dst.parent_path());
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
}

void rebuild_state(const std::string& prov_artifacts_path,
                   std::unordered_map<std::string, std::string> state) {
    for (auto& [path, checksum] : state) {
        move_artifact(prov_artifacts_path, checksum, path);
    }
}

void retrieve_exec_state(OrderedOperationsPerExecs ordered_operations_per_execs,
                         uint64_t exec_id) {
    std::vector<uint64_t> ordered_exec_ids;
    for (auto& [exec_id, _] : ordered_operations_per_execs) {
        ordered_exec_ids.push_back(exec_id);
    }
    std::sort(ordered_exec_ids.begin(), ordered_exec_ids.end());
    std::string prov_artifacts_path = get_prov_artifacts_path();
    std::unordered_map<std::string, std::string> state_before_exec =
        get_state_before_exec(ordered_operations_per_execs, exec_id,
                              ordered_exec_ids);
    rebuild_state(prov_artifacts_path, std::move(state_before_exec));
    std::unordered_map<std::string, std::string> state_after_exec =
        get_state_after_exec(ordered_operations_per_execs, exec_id,
                             ordered_exec_ids);
    rebuild_state(prov_artifacts_path, std::move(state_after_exec));
};

void retrieve_single_file(FileData file_data) {
    std::string prov_artifacts_path = get_prov_artifacts_path();
    move_artifact(prov_artifacts_path, file_data.checksum, file_data.file_path);
}
