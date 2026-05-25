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

std::unordered_set<std::string> get_internally_created_paths_before_exec(
    const OrderedOperationsPerExecs& ordered_operations_per_execs,
    uint64_t exec_id, const std::vector<uint64_t>& ordered_exec_ids) {
    std::unordered_set<std::string> internally_created_paths;
    std::unordered_set<std::string> resolved_paths;
    OperationMap current_operation_map;
    for (uint64_t current_exec_id : ordered_exec_ids) {
        if (current_exec_id >= exec_id) {
            break;
        }
        current_operation_map =
            ordered_operations_per_execs.at(current_exec_id);
        for (auto& [path, operations] : current_operation_map) {
            if (resolved_paths.contains(path)) {
                continue;
            }
            resolved_paths.insert(path);
            if (operations.start_checksum == "") {
                internally_created_paths.insert(path);
            }
        }
    }
    return internally_created_paths;
}

std::unordered_map<std::string, std::string> get_state_before_exec(
    const OrderedOperationsPerExecs& ordered_operations_per_execs,
    uint64_t exec_id, const std::vector<uint64_t>& ordered_exec_ids) {
    auto internally_created_paths = get_internally_created_paths_before_exec(
        ordered_operations_per_execs, exec_id, ordered_exec_ids);
    auto it = std::lower_bound(ordered_exec_ids.begin(), ordered_exec_ids.end(),
                               exec_id);
    std::vector<uint64_t> exec_ids_before_exec_reversed(
        std::make_reverse_iterator(it), ordered_exec_ids.rend());
    std::unordered_set<std::string> deleted_paths;
    std::unordered_set<std::string> resolved_paths;
    std::unordered_map<std::string, std::string> checksums_to_files;
    OperationMap current_operation_map;
    std::string start_checksum;
    std::string end_checksum;
    for (uint64_t current_exec_id : exec_ids_before_exec_reversed) {
        current_operation_map =
            ordered_operations_per_execs.at(current_exec_id);
        for (auto& [path, operations] : current_operation_map) {
            if (resolved_paths.contains(path)) {
                continue;
            }
            resolved_paths.insert(path);
            if (internally_created_paths.contains(path)) {
                continue;
            }
            start_checksum = operations.start_checksum;
            end_checksum = operations.end_checksum;
            if (start_checksum == "" && end_checksum == "") {
                continue;
            }
            if (operations.deleted) {
                deleted_paths.insert(path);
                continue;
            }
            if (deleted_paths.contains(path)) {
                continue;
            }
            if (end_checksum != "") {
                checksums_to_files[end_checksum] = path;
            } else if (start_checksum != "") {
                checksums_to_files[start_checksum] = path;
            }
        }
    }
    return checksums_to_files;
}

std::unordered_map<std::string, std::string> get_state_after_exec(
    const OrderedOperationsPerExecs& ordered_operations_per_execs,
    uint64_t exec_id, const std::vector<uint64_t>& ordered_exec_ids) {
    auto it = std::lower_bound(ordered_exec_ids.begin(), ordered_exec_ids.end(),
                               exec_id);
    std::vector<uint64_t> exec_ids_after_exec;
    if (it == ordered_exec_ids.end()) {
        exec_ids_after_exec = ordered_exec_ids;
    } else {
        if (*it == exec_id) {
            ++it;
        }
        exec_ids_after_exec = std::vector<uint64_t>(it, ordered_exec_ids.end());
    }
    std::unordered_map<std::string, std::string> checksums_to_files;
    std::unordered_set<std::string> resolved_paths;
    std::unordered_set<std::string> excluded_paths;
    OperationMap current_operation_map;
    std::string start_checksum;
    for (uint64_t current_exec_id : exec_ids_after_exec) {
        current_operation_map =
            ordered_operations_per_execs.at(current_exec_id);
        for (auto& [path, operations] : current_operation_map) {
            if (resolved_paths.contains(path) ||
                excluded_paths.contains(path)) {
                continue;
            }
            start_checksum = operations.start_checksum;
            if (start_checksum == "") {
                excluded_paths.insert(path);
                continue;
            }
            checksums_to_files[start_checksum] = path;
            resolved_paths.insert(path);
        }
    }
    return checksums_to_files;
}

void move_artifact(const std::string& prov_artifacts_path,
                   const std::string& checksum,
                   const std::string& target_path) {
    fs::path src = fs::path(prov_artifacts_path) / "artifact_files" /
                   checksum.substr(0, 2) / checksum;
    fs::path dst(target_path);
    fs::create_directories(dst.parent_path());
    if (!fs::exists(dst)) {
        fs::copy_file(src, dst);
    }
}

void rebuild_state(const std::string& prov_artifacts_path,
                   std::unordered_map<std::string, std::string> state) {
    for (auto& [checksum, path] : state) {
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
