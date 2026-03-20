#pragma once
#include <optional>
#include <string>
#include <variant>

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
    int process_id;
    bool reads = false;
    bool writes = false;
    bool deletes = false;
};

struct Parsed {
    RequestType request_type{};
    std::variant<JobsQueryOpts, ExecsQueryOpts, ProcessesQueryOpts,
                 FileQueryOpts>
        opts;
};
