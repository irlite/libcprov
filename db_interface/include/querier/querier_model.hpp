#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

enum class RequestType { JobsQuery, ExecsQuery, ProcessesQuery, FileQuery };

struct JobsQueryOpts {
    std::optional<std::string> user;
    std::optional<std::string> before;
    std::optional<std::string> after;
    bool list_with_files = false;
};

struct ExecsQueryOpts {
    std::string job_id;
    std::string cluster;
    bool list_with_files = false;
};

struct ProcessesQueryOpts {
    std::string exec_id;
    bool list_with_files = false;
};

struct FileQueryOpts {
    int process_id = 0;
    bool reads = false;
    bool writes = false;
    bool deletes = false;
};

struct Parsed {
    RequestType request_type{};
    std::variant<JobsQueryOpts, ExecsQueryOpts, ProcessesQueryOpts,
                 FileQueryOpts>
        opts;
    bool print_json = false;
};

struct FileOperations {
    bool reads = false;
    bool writes = false;
    bool deletes = false;
};

struct ParsedFilesQueryData {
    std::unordered_map<std::string, FileOperations> file_operations;
};

struct ParsedProcessesQueryData {
    std::string process_command;
    uint64_t process_id;
    std::vector<ParsedFilesQueryData> operations;
};

struct ParsedExecsQueryData {
    uint64_t start_time;
    uint64_t exec_id;
    std::vector<ParsedProcessesQueryData> processes;
    std::unordered_map<uint64_t, std::vector<uint64_t>> executes;
    std::unordered_map<std::string, std::string> rename_map;
    std::string json;
    std::string path;
    std::string command;
};

struct ParsedJobsQueryData {
    uint64_t job_id;
    std::string cluster_name;
    std::string job_name;
    std::string username;
    uint64_t start_time;
    uint64_t end_time;
    std::string path;
    std::string json;
};

using ParsedJobsQuery = std::vector<ParsedJobsQueryData>;
using ParsedExecsQuery = std::vector<ParsedExecsQueryData>;
using ParsedProcessesQuery = std::vector<ParsedProcessesQueryData>;
using ParsedFilesQuery = std::vector<ParsedFilesQueryData>;

using ParsedQuery = std::variant<ParsedJobsQuery, ParsedExecsQuery,
                                 ParsedProcessesQuery, ParsedFilesQuery>;
