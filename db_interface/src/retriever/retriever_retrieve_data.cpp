#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "config_parser.hpp"
#include "retriever/retriever_model.hpp"

namespace fs = std::filesystem;

std::string get_prov_artifacts_path() {
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    return config.prov_artifacts_path;
}

void erase_path_from_state(
    std::unordered_map<std::string, std::string>& checksums_to_files,
    const std::string& path) {
    for (auto it = checksums_to_files.begin();
         it != checksums_to_files.end();) {
        if (it->second == path) {
            it = checksums_to_files.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<uint64_t> get_ordered_exec_ids(
    const OrderedOperationsPerExecs& ordered_operations_per_execs) {
    std::vector<uint64_t> ordered_exec_ids;
    ordered_exec_ids.reserve(ordered_operations_per_execs.size());
    for (const auto& [exec_id, _] : ordered_operations_per_execs) {
        ordered_exec_ids.push_back(exec_id);
    }
    std::sort(ordered_exec_ids.begin(), ordered_exec_ids.end());
    return ordered_exec_ids;
}

/*
 * Returns the filesystem state before the first tracked exec.
 *
 * A file is part of the initial state iff its first occurrence in the job
 * has a non-empty start_checksum. If the first occurrence has an empty
 * start_checksum, the file was created during the job and must not be restored
 * for exec 0.
 */
std::unordered_map<std::string, std::string>
get_initial_state_before_first_exec(
    const OrderedOperationsPerExecs& ordered_operations_per_execs,
    const std::vector<uint64_t>& ordered_exec_ids) {
    std::unordered_map<std::string, std::string> checksums_to_files;
    std::unordered_map<std::string, bool> path_seen;

    for (uint64_t current_exec_id : ordered_exec_ids) {
        const OperationMap& current_operation_map =
            ordered_operations_per_execs.at(current_exec_id);

        for (const auto& [path, operations] : current_operation_map) {
            if (path_seen.contains(path)) {
                continue;
            }
            path_seen[path] = true;

            if (!operations.start_checksum.empty()) {
                checksums_to_files[operations.start_checksum] = path;
            }
        }
    }

    return checksums_to_files;
}

/*
 * Returns the cumulative filesystem state immediately after exec_id concluded.
 *
 * For each file path:
 * - deleted files are removed from the state
 * - modified / created files use end_checksum if available
 * - read-only files keep their existing state; if they were first observed with
 *   a start_checksum and are not yet in state, they are inserted with that
 * checksum
 */
std::unordered_map<std::string, std::string> get_state_after_exec(
    const OrderedOperationsPerExecs& ordered_operations_per_execs,
    uint64_t exec_id, const std::vector<uint64_t>& ordered_exec_ids) {
    std::unordered_map<std::string, std::string> checksums_to_files =
        get_initial_state_before_first_exec(ordered_operations_per_execs,
                                            ordered_exec_ids);

    for (uint64_t current_exec_id : ordered_exec_ids) {
        if (current_exec_id > exec_id) {
            break;
        }

        const OperationMap& current_operation_map =
            ordered_operations_per_execs.at(current_exec_id);

        for (const auto& [path, operations] : current_operation_map) {
            if (operations.deleted) {
                erase_path_from_state(checksums_to_files, path);
                continue;
            }

            if (!operations.end_checksum.empty()) {
                erase_path_from_state(checksums_to_files, path);
                checksums_to_files[operations.end_checksum] = path;
                continue;
            }

            if (!operations.start_checksum.empty()) {
                bool path_already_present = false;
                for (const auto& [checksum, existing_path] :
                     checksums_to_files) {
                    if (existing_path == path) {
                        path_already_present = true;
                        break;
                    }
                }

                if (!path_already_present) {
                    checksums_to_files[operations.start_checksum] = path;
                }
            }
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
                   const std::unordered_map<std::string, std::string>& state) {
    for (const auto& [checksum, path] : state) {
        move_artifact(prov_artifacts_path, checksum, path);
    }
}

void retrieve_exec_state(OrderedOperationsPerExecs ordered_operations_per_execs,
                         uint64_t exec_id) {
    std::vector<uint64_t> ordered_exec_ids =
        get_ordered_exec_ids(ordered_operations_per_execs);

    std::string prov_artifacts_path = get_prov_artifacts_path();

    std::unordered_map<std::string, std::string> state;
    if (exec_id == 0) {
        state = get_initial_state_before_first_exec(
            ordered_operations_per_execs, ordered_exec_ids);
    } else {
        state = get_state_after_exec(ordered_operations_per_execs, exec_id,
                                     ordered_exec_ids);
    }

    rebuild_state(prov_artifacts_path, state);
}

void retrieve_single_file(FileData file_data) {
    std::string prov_artifacts_path = get_prov_artifacts_path();
    move_artifact(prov_artifacts_path, file_data.checksum, file_data.file_path);
}
