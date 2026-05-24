#include "processor.hpp"

#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <stack>
#include <string>
#include <unordered_set>

#include "model.hpp"
#include "xxhash.h"

LinuxProcessMap init_processing(EventsByFile& events_by_file) {
    LinuxProcessMap linux_process_map;
    bool ignore_file;
    for (std::vector<Event>& events : events_by_file) {
        LinuxProcess linux_process;
        uint64_t pid;
        std::string process_name;
        uint64_t env_variables_hash;
        std::string process_id;
        ignore_file = false;
        for (Event& event : events) {
            SysOp op = event.operation;
            if (op == SysOp::ProcessStart) {
                ProcessStart& process_start =
                    std::get<ProcessStart>(event.event_payload);
                /*if (process_start.process_name.rfind("sh -c ", 0) == 0) {
                    ignore_file = true;
                    break;
                }*/
                std::string env_variables = process_start.env_variables;
                env_variables_hash =
                    XXH64(env_variables.data(), env_variables.size(), 0);
                process_name = process_start.process_name;
                process_id = process_name + std::to_string(env_variables_hash);
                linux_process = {process_start.ppid, event.ts, 0, process_id};
                pid = process_start.pid;
                process_start.env_variables_hash = env_variables_hash;
                break;
            }
        }
        if (ignore_file) {
            events.clear();
            continue;
        }
        for (auto it = events.rbegin(); it != events.rend(); ++it) {
            const Event& event = *it;
            SysOp op = event.operation;
            if (op == SysOp::ProcessEnd || op == SysOp::Exec) {
                linux_process.end_time = event.ts;
                break;
            }
        }
        for (Event& event : events) {
            event.process_id = process_id;
            if (event.operation == SysOp::Exec) {
                event.event_payload = ExecCall{pid};
            }
        }
        linux_process_map[pid].push_back(linux_process);
    }
    return linux_process_map;
}

std::vector<Event> combine_events(EventsByFile events_by_file) {
    std::vector<Event> all_events;
    for (std::vector<Event>& events : events_by_file) {
        all_events.reserve(all_events.size() + events.size());
        std::ranges::move(events, std::back_inserter(all_events));
    }
    return all_events;
}

ExecuteSetMap resolve_forks(LinuxProcessMap& linux_process_map,
                            std::vector<Event>& events) {
    ExecuteSetMap execute_set_map;
    for (const auto& [pid, linux_processes] : linux_process_map) {
        for (const LinuxProcess& linux_process : linux_processes) {
            const std::vector<LinuxProcess> potential_parent_processes =
                linux_process_map[linux_process.ppid];
            for (const LinuxProcess potential_parent_process :
                 potential_parent_processes) {
                if (potential_parent_process.start_time <
                        linux_process.start_time &&
                    potential_parent_process.end_time >
                        linux_process.end_time) {
                    execute_set_map[potential_parent_process.process_id].insert(
                        linux_process.process_id);
                    break;
                }
            }
        }
    }
    return execute_set_map;
}

void sort_linux_processes(
    std::vector<LinuxProcess>& potential_child_processes) {
    std::sort(potential_child_processes.begin(),
              potential_child_processes.end(),
              [](const LinuxProcess& a, const LinuxProcess& b) {
                  return a.start_time < b.start_time;
              });
}

ExecuteSetMap resolve_exec(Event exec_event, LinuxProcessMap& linux_process_map,
                           ExecuteSetMap& execute_set_map) {
    const auto& exec = std::get<ExecCall>(exec_event.event_payload);
    std::vector<LinuxProcess>& potential_child_processes =
        linux_process_map[exec.pid.value()];
    sort_linux_processes(potential_child_processes);
    for (const LinuxProcess potential_child_process :
         potential_child_processes) {
        if (potential_child_process.process_id != exec_event.process_id &&
            potential_child_process.start_time > exec_event.ts) {
            execute_set_map[exec_event.process_id].insert(
                potential_child_process.process_id);
            break;
        }
    }
    return execute_set_map;
}

ExecuteSetMap resolve_execs(
    std::unordered_map<std::string, std::stack<Event>> enqueued_execs,
    LinuxProcessMap& linux_process_map, ExecuteSetMap& execute_set_map) {
    for (auto& [_, enqueued_exec] : enqueued_execs) {
        while (!enqueued_exec.empty()) {
            Event event = enqueued_exec.top();
            enqueued_exec.pop();
            execute_set_map =
                resolve_exec(event, linux_process_map, execute_set_map);
        }
    }
    return execute_set_map;
}

void sort_events(std::vector<Event>& events) {
    std::sort(events.begin(), events.end(),
              [](const Event& a, const Event& b) { return a.ts < b.ts; });
}

void log_process_start(Event event, ProcessedExecData& processed_exec_data) {
    const auto& process_start = std::get<ProcessStart>(event.event_payload);
    if (!processed_exec_data.process_map.contains(event.process_id)) {
        std::string process_name = process_start.process_name;
        processed_exec_data.process_map[event.process_id] = Process{
            .process_name = process_name,
            .env_variable_hash = process_start.env_variables_hash.value(),
            .operation_map = {}};
        processed_exec_data.env_variables_hash_to_variables
            [process_start.env_variables_hash.value()] =
            process_start.env_variables;
        std::string::size_type first_space_pos = process_name.find(' ');
        /*std::string executable = (first_space_pos == std::string::npos)
                                     ? process_name
                                     : process_name.substr(0,
           first_space_pos);*/
        // operations_data_backup_format[executable] =
        //     BackupOperations{.read = false, .write = false, .execute = true};
    }
}

void log_operation(const std::string& path, const std::string& process_id,
                   ProcessedExecData& processed_exec_data, SysOp op_type,
                   std::string checksum) {
    /*std::string resolved_path = path;
    auto it = processed_exec_data.rename_map.find(path);
    if (it != processed_exec_data.rename_map.end()) {
        resolved_path = it->second;
    }*/
    Operations& selected_operations =
        processed_exec_data.process_map[process_id].operation_map[path];
    // BackupOperations& selected_backup_operations =
    //     operations_data_backup_format[resolved_path];
    if (checksum != "") {
        selected_operations.start_checksum = checksum;
    }
    switch (op_type) {
        case SysOp::Write: {
            selected_operations.write = true;
            break;
        }
        case SysOp::Read: {
            selected_operations.read = true;
            break;
        }
        case SysOp::Unlink: {
            selected_operations.deleted = true;
            break;
        }
    }
}

void log_rename(const Event& event, ProcessedExecData& processed_exec_data) {
    const Rename& rename = std::get<Rename>(event.event_payload);
    const std::string& original_path = rename.original_path;
    const std::string& new_path = rename.new_path;
    std::unordered_map<std::string, std::string>& rename_map =
        processed_exec_data.rename_map;
    auto it = rename_map.find(original_path);
    if (it == rename_map.end()) {
        rename_map[new_path] = original_path;
    } else {
        rename_map[new_path] = it->second;
        rename_map.erase(it);
    }
    for (auto& [process_id, process] : processed_exec_data.process_map) {
        auto it = process.operation_map.find(original_path);
        if (it != process.operation_map.end()) {
            process.operation_map[new_path] = it->second;
            process.operation_map.erase(it);
        }
    }
    // processed_exec_data.process_map[process_id].operation_map[path];
};

std::string get_checksum(
    ChecksumsByFiles& original_checksums_by_files,
    std::unordered_set<std::string>& seen_files, const std::string& path,
    std::unordered_map<std::string, std::string>& rename_map) {
    std::string resolved_path = path;
    auto it = rename_map.find(path);
    if (it != rename_map.end()) {
        resolved_path = it->second;
    }
    if (!seen_files.contains(resolved_path)) {
        seen_files.insert(resolved_path);
        if (original_checksums_by_files.contains(resolved_path)) {
            return original_checksums_by_files[resolved_path];
        }
    }
    return "";
}

ProcessedExecData process_events(EventsByFile& events_by_file,
                                 ChecksumsByFiles original_checksums_by_files) {
    LinuxProcessMap linux_process_map = init_processing(events_by_file);
    std::vector<Event> events = combine_events(std::move(events_by_file));
    ExecuteSetMap execute_set_map = resolve_forks(linux_process_map, events);
    sort_events(events);
    std::unordered_map<std::string, std::stack<Event>> enqueued_execs;
    ProcessedExecData processed_exec_data;
    std::unordered_set<std::string> seen_files;
    // bool file_already_seen;
    std::string checksum;
    for (const Event& event : events) {
        checksum = "";
        SysOp op = event.operation;
        const EventPayload& payload = event.event_payload;
        // file_already_seen = true;
        switch (op) {
            case SysOp::ProcessStart: {
                log_process_start(event, processed_exec_data);
                break;
            }
            case SysOp::Write:
            case SysOp::Read:
            case SysOp::Unlink: {
                const std::string& path =
                    std::get<std::string>(event.event_payload);
                std::string process_id = event.process_id;
                /*if (!seen_files.contains(path)) {
                    seen_files.insert(path);
                    checksum = original_checksums_by_files[path];
                }*/
                checksum = get_checksum(original_checksums_by_files, seen_files,
                                        path, processed_exec_data.rename_map);
                log_operation(path, process_id, processed_exec_data, op,
                              checksum);
                break;
            }
            case SysOp::Transfer: {
                const Transfer& transfer =
                    std::get<Transfer>(event.event_payload);
                const std::string& path_read = transfer.path_read;
                /*if (!seen_files.contains(path_read)) {
                    seen_files.insert(path_read);
                    checksum = original_checksums_by_files[path_read];
                }*/
                checksum =
                    get_checksum(original_checksums_by_files, seen_files,
                                 path_read, processed_exec_data.rename_map);
                log_operation(path_read, event.process_id, processed_exec_data,
                              SysOp::Read, checksum);
                log_operation(transfer.path_write, event.process_id,
                              processed_exec_data, SysOp::Write, "");
                break;
            }
            case SysOp::Rename: {
                log_rename(event, processed_exec_data);
                break;
            }
            case SysOp::Exec: {
                enqueued_execs[event.process_id].push(event);
                break;
            }
            case SysOp::ExecFail: {
                enqueued_execs[event.process_id].pop();
                break;
            }
            default:
                break;
        }
    }
    processed_exec_data.execute_set_map =
        resolve_execs(enqueued_execs, linux_process_map, execute_set_map);
    return processed_exec_data;
}
