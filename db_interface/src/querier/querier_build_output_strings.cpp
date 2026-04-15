#include <unistd.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include "querier/querier_model.hpp"

static bool supports_color_stdout() {
    if (!isatty(STDOUT_FILENO)) return false;
    if (std::getenv("NO_COLOR")) return false;
    const char* term = std::getenv("TERM");
    if (!term) return false;
    if (std::string_view(term) == "dumb") return false;
    return true;
}

static std::string c_reset(bool enable) { return enable ? "\033[0m" : ""; }
static std::string c_bold(bool enable) { return enable ? "\033[1m" : ""; }
static std::string c_blue(bool enable) { return enable ? "\033[34m" : ""; }
static std::string c_green(bool enable) { return enable ? "\033[32m" : ""; }
static std::string c_yellow(bool enable) { return enable ? "\033[33m" : ""; }

static std::string get_operations_string(const FileOperations& file_operations,
                                         bool color_enabled) {
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
    return c_green(color_enabled) + out + c_reset(color_enabled);
}

static std::string format_kv(const std::string& key, const std::string& value,
                             bool color_enabled) {
    return c_blue(color_enabled) + key + c_reset(color_enabled) + ": " +
           c_green(color_enabled) + value + c_reset(color_enabled);
}

std::string get_jobs_query_output_string(ParsedQuery parsed_query) {
    bool color_enabled = supports_color_stdout();
    std::string jobs_query_output_string = "";
    for (const ParsedJobsQueryData& parsed_jobs_query_data :
         std::get<ParsedJobsQuery>(parsed_query)) {
        jobs_query_output_string +=
            c_bold(color_enabled) + c_yellow(color_enabled) + "[JOB] " +
            parsed_jobs_query_data.job_name + c_reset(color_enabled) + "\n" +
            "├── " +
            format_kv("id", std::to_string(parsed_jobs_query_data.job_id),
                      color_enabled) +
            "\n" + "├── " +
            format_kv("cluster", parsed_jobs_query_data.cluster_name,
                      color_enabled) +
            "\n" + "├── " +
            format_kv("user", parsed_jobs_query_data.username, color_enabled) +
            "\n" + "├── " +
            format_kv("start",
                      std::to_string(parsed_jobs_query_data.start_time),
                      color_enabled) +
            "\n" + "├── " +
            format_kv("end", std::to_string(parsed_jobs_query_data.end_time),
                      color_enabled) +
            "\n" + "├── " +
            format_kv("path", parsed_jobs_query_data.path, color_enabled) +
            "\n" + "└── " +
            format_kv("json", parsed_jobs_query_data.json, color_enabled) +
            "\n";
    }
    return jobs_query_output_string;
}

std::string get_execs_query_output_string(ParsedQuery parsed_query) {
    bool color_enabled = supports_color_stdout();
    std::string execs_query_output_string = "";
    for (const ParsedExecsQueryData& parsed_execs_query_data :
         std::get<ParsedExecsQuery>(parsed_query)) {
        execs_query_output_string +=
            c_bold(color_enabled) + c_yellow(color_enabled) +
            std::string("[EXEC]") + c_reset(color_enabled) + "\n" + "├── " +
            format_kv("id", std::to_string(parsed_execs_query_data.exec_id),
                      color_enabled) +
            "\n" + "├── " +
            format_kv("start",
                      std::to_string(parsed_execs_query_data.start_time),
                      color_enabled) +
            "\n" + "├── " +
            format_kv("path", parsed_execs_query_data.path, color_enabled) +
            "\n" + "├── " +
            format_kv("command", parsed_execs_query_data.command,
                      color_enabled) +
            "\n" + "├── " +
            format_kv("json", parsed_execs_query_data.json, color_enabled) +
            "\n";
        execs_query_output_string += "├── " + c_blue(color_enabled) +
                                     "executes" + c_reset(color_enabled) +
                                     ":\n";
        if (parsed_execs_query_data.executes.empty()) {
            execs_query_output_string += "│   (none)\n";
        } else {
            for (const auto& [parent_process_id, child_process_id_vector] :
                 parsed_execs_query_data.executes) {
                execs_query_output_string += "│   " + c_blue(color_enabled) +
                                             std::to_string(parent_process_id) +
                                             c_reset(color_enabled) + " -> ";
                bool first = true;
                for (uint64_t child_process_id : child_process_id_vector) {
                    if (!first) execs_query_output_string += ",";
                    first = false;
                    execs_query_output_string +=
                        c_green(color_enabled) +
                        std::to_string(child_process_id) +
                        c_reset(color_enabled);
                }
                execs_query_output_string += "\n";
            }
        }
        execs_query_output_string += "├── " + c_blue(color_enabled) +
                                     "rename_map" + c_reset(color_enabled) +
                                     ":\n";
        if (parsed_execs_query_data.rename_map.empty()) {
            execs_query_output_string += "│   (none)\n";
        } else {
            for (const auto& [original_path, new_path] :
                 parsed_execs_query_data.rename_map) {
                execs_query_output_string +=
                    "│   " + c_blue(color_enabled) + original_path +
                    c_reset(color_enabled) + " -> " + c_green(color_enabled) +
                    new_path + c_reset(color_enabled) + "\n";
            }
        }
        execs_query_output_string += "└── " + c_blue(color_enabled) +
                                     "processes" + c_reset(color_enabled) +
                                     ":\n";
        if (parsed_execs_query_data.processes.empty()) {
            execs_query_output_string += "    (none)\n";
        } else {
            for (const ParsedProcessesQueryData& parsed_processes_query_data :
                 parsed_execs_query_data.processes) {
                execs_query_output_string +=
                    "    " + c_bold(color_enabled) + c_yellow(color_enabled) +
                    "[PROCESS] " + parsed_processes_query_data.process_command +
                    c_reset(color_enabled) + "\n" + "    ├── " +
                    format_kv(
                        "id",
                        std::to_string(parsed_processes_query_data.process_id),
                        color_enabled) +
                    "\n";
                if (parsed_processes_query_data.operations.empty()) {
                    execs_query_output_string +=
                        "    └── " + c_blue(color_enabled) + "operations" +
                        c_reset(color_enabled) + ": (none)\n";
                } else {
                    execs_query_output_string +=
                        "    └── " + c_blue(color_enabled) + "operations" +
                        c_reset(color_enabled) + ":\n";
                    for (const ParsedFilesQueryData& parsed_files_query_data :
                         parsed_processes_query_data.operations) {
                        if (parsed_files_query_data.file_operations.empty()) {
                            execs_query_output_string += "        (none)\n";
                        } else {
                            for (const auto& [path, file_operations] :
                                 parsed_files_query_data.file_operations) {
                                execs_query_output_string +=
                                    "        " + c_blue(color_enabled) + path +
                                    c_reset(color_enabled) + " [" +
                                    get_operations_string(file_operations,
                                                          color_enabled) +
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
    bool color_enabled = supports_color_stdout();
    std::string processes_query_output_string = "";
    for (const ParsedProcessesQueryData& parsed_processes_query_data :
         std::get<ParsedProcessesQuery>(parsed_query)) {
        processes_query_output_string +=
            c_bold(color_enabled) + c_yellow(color_enabled) + "[PROCESS] " +
            parsed_processes_query_data.process_command +
            c_reset(color_enabled) + "\n" + "├── " +
            format_kv("id",
                      std::to_string(parsed_processes_query_data.process_id),
                      color_enabled) +
            "\n";
        if (parsed_processes_query_data.operations.empty()) {
            processes_query_output_string +=
                "└── " + c_blue(color_enabled) + "operations" +
                c_reset(color_enabled) + ": (none)\n";
        } else {
            processes_query_output_string += "└── " + c_blue(color_enabled) +
                                             "operations" +
                                             c_reset(color_enabled) + ":\n";
            for (const ParsedFilesQueryData& parsed_files_query_data :
                 parsed_processes_query_data.operations) {
                if (parsed_files_query_data.file_operations.empty()) {
                    processes_query_output_string += "    (none)\n";
                } else {
                    for (const auto& [path, file_operations] :
                         parsed_files_query_data.file_operations) {
                        processes_query_output_string +=
                            "    " + c_blue(color_enabled) + path +
                            c_reset(color_enabled) + " [" +
                            get_operations_string(file_operations,
                                                  color_enabled) +
                            "]\n";
                    }
                }
            }
        }
    }
    return processes_query_output_string;
}

std::string get_files_query_output_string(ParsedQuery parsed_query) {
    bool color_enabled = supports_color_stdout();
    std::string files_query_output_string = "";
    for (const ParsedFilesQueryData& parsed_files_query_data :
         std::get<ParsedFilesQuery>(parsed_query)) {
        if (parsed_files_query_data.file_operations.empty()) {
            files_query_output_string += "(none)\n";
        } else {
            for (const auto& [path, file_operations] :
                 parsed_files_query_data.file_operations) {
                files_query_output_string +=
                    c_blue(color_enabled) + path + c_reset(color_enabled) +
                    " [" +
                    get_operations_string(file_operations, color_enabled) +
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
