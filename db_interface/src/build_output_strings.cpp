#include <string>

#include "model.hpp"

static std::string get_operations_string(
    const FileOperations& file_operations) {
    std::string out;
    bool first = true;
    if (file_operations.reads) {
        out += first ? "read" : ",read";
        first = false;
    }
    if (file_operations.writes) {
        out += first ? "write" : ",write";
        first = false;
    }
    if (file_operations.deletes) {
        out += first ? "deleted" : ",deleted";
        first = false;
    }
    return out;
}

std::string get_jobs_query_output_string(ParsedQuery parsed_query) {
    std::string jobs_query_output_string = "";
    for (const ParsedJobsQueryData& parsed_jobs_query_data :
         std::get<ParsedJobsQuery>(parsed_query)) {
        jobs_query_output_string +=
            "[JOB] " + parsed_jobs_query_data.job_name + "\n" +
            "├── id: " + std::to_string(parsed_jobs_query_data.job_id) + "\n" +
            "├── cluster: " + parsed_jobs_query_data.cluster_name + "\n" +
            "├── user: " + parsed_jobs_query_data.username + "\n" +
            "├── start: " + std::to_string(parsed_jobs_query_data.start_time) +
            "\n" +
            "├── end: " + std::to_string(parsed_jobs_query_data.end_time) +
            "\n" + "├── path: " + parsed_jobs_query_data.path + "\n" +
            "└── json: " + parsed_jobs_query_data.json + "\n";
    }
    return jobs_query_output_string;
}

std::string get_execs_query_output_string(ParsedQuery parsed_query) {
    std::string execs_query_output_string = "";
    for (const ParsedExecsQueryData& parsed_execs_query_data :
         std::get<ParsedExecsQuery>(parsed_query)) {
        execs_query_output_string +=
            std::string("[EXEC]\n") +
            "├── id: " + std::to_string(parsed_execs_query_data.exec_id) +
            "\n" +
            "├── start: " + std::to_string(parsed_execs_query_data.start_time) +
            "\n" + "├── path: " + parsed_execs_query_data.path + "\n" +
            "├── command: " + parsed_execs_query_data.command + "\n" +
            "├── json: " + parsed_execs_query_data.json + "\n";
        execs_query_output_string += "├── executes:\n";
        if (parsed_execs_query_data.executes.empty()) {
            execs_query_output_string += "│   (none)\n";
        } else {
            for (const auto& [parent_process_id, child_process_id_vector] :
                 parsed_execs_query_data.executes) {
                execs_query_output_string +=
                    "│   " + std::to_string(parent_process_id) + " -> ";
                bool first = true;
                for (uint64_t child_process_id : child_process_id_vector) {
                    if (!first) execs_query_output_string += ",";
                    first = false;
                    execs_query_output_string +=
                        std::to_string(child_process_id);
                }
                execs_query_output_string += "\n";
            }
        }
        execs_query_output_string += "├── rename_map:\n";
        if (parsed_execs_query_data.rename_map.empty()) {
            execs_query_output_string += "│   (none)\n";
        } else {
            for (const auto& [original_path, new_path] :
                 parsed_execs_query_data.rename_map) {
                execs_query_output_string +=
                    "│   " + original_path + " -> " + new_path + "\n";
            }
        }
        execs_query_output_string += "└── processes:\n";
        if (parsed_execs_query_data.processes.empty()) {
            execs_query_output_string += "    (none)\n";
        } else {
            for (const ParsedProcessesQueryData& parsed_processes_query_data :
                 parsed_execs_query_data.processes) {
                execs_query_output_string +=
                    "    [PROCESS] " +
                    parsed_processes_query_data.process_command + "\n" +
                    "    ├── id: " +
                    std::to_string(parsed_processes_query_data.process_id) +
                    "\n";
                if (parsed_processes_query_data.operations.empty()) {
                    execs_query_output_string += "    └── operations: (none)\n";
                } else {
                    execs_query_output_string += "    └── operations:\n";
                    for (const ParsedFilesQueryData& parsed_files_query_data :
                         parsed_processes_query_data.operations) {
                        if (parsed_files_query_data.file_operations.empty()) {
                            execs_query_output_string += "        (none)\n";
                        } else {
                            for (const auto& [path, file_operations] :
                                 parsed_files_query_data.file_operations) {
                                execs_query_output_string +=
                                    "        " + path + " [" +
                                    get_operations_string(file_operations) +
                                    "]\n";
                            }
                        }
                    }
                }
            }
        }
    }
    return execs_query_output_string;
}

std::string get_processes_query_output_string(ParsedQuery parsed_query) {
    std::string processes_query_output_string = "";
    for (const ParsedProcessesQueryData& parsed_processes_query_data :
         std::get<ParsedProcessesQuery>(parsed_query)) {
        processes_query_output_string +=
            "[PROCESS] " + parsed_processes_query_data.process_command + "\n" +
            "├── id: " +
            std::to_string(parsed_processes_query_data.process_id) + "\n";
        if (parsed_processes_query_data.operations.empty()) {
            processes_query_output_string += "└── operations: (none)\n";
        } else {
            processes_query_output_string += "└── operations:\n";
            for (const ParsedFilesQueryData& parsed_files_query_data :
                 parsed_processes_query_data.operations) {
                if (parsed_files_query_data.file_operations.empty()) {
                    processes_query_output_string += "    (none)\n";
                } else {
                    for (const auto& [path, file_operations] :
                         parsed_files_query_data.file_operations) {
                        processes_query_output_string +=
                            "    " + path + " [" +
                            get_operations_string(file_operations) + "]\n";
                    }
                }
            }
        }
    }
    return processes_query_output_string;
}

std::string get_files_query_output_string(ParsedQuery parsed_query) {
    std::string files_query_output_string = "";
    for (const ParsedFilesQueryData& parsed_files_query_data :
         std::get<ParsedFilesQuery>(parsed_query)) {
        if (parsed_files_query_data.file_operations.empty()) {
            files_query_output_string += "(none)\n";
        } else {
            for (const auto& [path, file_operations] :
                 parsed_files_query_data.file_operations) {
                files_query_output_string +=
                    path + " [" + get_operations_string(file_operations) +
                    "]\n";
            }
        }
    }
    return files_query_output_string;
}

std::string get_output_string(ParsedQuery parsed_query,
                              RequestType request_type) {
    switch (request_type) {
        case RequestType::JobsQuery: {
            return (get_jobs_query_output_string(parsed_query));
        }
        case RequestType::ExecsQuery: {
            return (get_execs_query_output_string(parsed_query));
        }
        case RequestType::ProcessesQuery: {
            return (get_processes_query_output_string(parsed_query));
        }
        case RequestType::FileQuery: {
            return (get_files_query_output_string(parsed_query));
        }
    }
    return "";
}
