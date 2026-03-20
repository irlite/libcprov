#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

//--- PROV ---
enum class InjectorDataType { Start, End, Exec };
enum class OperationType { Read, Write, Link, Symlink, Delete };

struct ProcessStart {
    std::string process_name;
    uint64_t env_variables_hash;
};
struct Read {
    std::string path_in;
};
struct Write {
    std::string path_out;
};
struct Delete {
    std::string deleted_path;
};
using ExecuteSetMap =
    std::unordered_map<std::string, std::unordered_set<std::string>>;
using ExecuteSetMapDB =
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>>;
using RenameMap = std::unordered_map<std::string, std::string>;
/*
struct Rename {
    std::string original_path;
    std::string new_path;
};
struct Link {
    std::string original_path;
    std::string new_path;
};
struct Symlink {
    std::string original_path;
    std::string new_path;
};*/

/*struct Operation {
    OperationType operation_type;
    // std::variant<Read, Write, Link, Symlink, Delete> operation_payload;
    std::variant<Read, Write, Delete> operation_payload;
};*/

struct Operations {
    bool read = false;
    bool write = false;
    bool deleted = false;
};

struct Process {
    std::string process_command;
    // uint64_t process_hash;
    uint64_t env_variable_hash;
    // std::vector<Operation> operations;
    std::unordered_map<std::string, Operations> operation_map;
};

using EnvVariableHashPairs = std::unordered_map<uint64_t, std::string>;
using ProcessMap = std::unordered_map<std::string, Process>;
using ProcessMapDB = std::unordered_map<uint64_t, Process>;
struct ExecData {
    std::optional<uint64_t> exec_id;
    std::optional<uint64_t> start_time;
    ProcessMap process_map;
    ProcessMapDB process_map_db;
    ExecuteSetMap execute_set_map;
    ExecuteSetMapDB execute_set_map_db;
    RenameMap rename_map;
    EnvVariableHashPairs env_variables_hash_to_variables;
    std::string json;
    std::string path;
    std::string command;
};

struct StartData {
    std::string job_name;
    std::string username;
    std::string json;
    std::string path;
};

struct ParsedInjectorData {
    InjectorDataType injector_data_type;
    uint64_t job_id;
    std::string cluster_name;
    uint64_t timestamp;
    std::optional<std::variant<StartData, ExecData>> payload;
};

struct JobData {
    bool succeded;
    std::string job_name;
    std::string username;
    uint64_t start_time;
    uint64_t end_time;
    std::string path;
    std::string json;
    std::vector<ExecData> exec_data_vector;
};

//--- GRAPH ---
struct ParsedGraphRequestData {
    uint64_t job_id;
    std::string cluster_name;
};

//--- DB_INTERFACE ---
enum class RequestType { JobsQuery, ExecsQuery, ProcessesQuery, FileQuery };

struct JobsQueryOpts {
    std::string user;
    std::optional<std::string> before;
    std::optional<std::string> after;
};

struct ExecsQueryOpts {
    uint64_t job_id;
    std::string cluster;
    bool list_with_processes = false;
    bool list_with_files = false;
};

struct ProcessesQueryOpts {
    uint64_t exec_id;
    bool list_with_files = false;
};

struct FileQueryOpts {
    uint64_t process_id;
    bool reads = false;
    bool writes = false;
    bool deletes = false;
};

struct ParsedDBInterfaceRequestData {
    RequestType request_type{};
    std::variant<JobsQueryOpts, ExecsQueryOpts, ProcessesQueryOpts,
                 FileQueryOpts>
        opts;
};

struct DBOperations {
    std::string filename;
    std::vector<Operations> operations;
};

struct ProcessDataInterface {
    std::optional<std::string> exec_id;
    uint64_t process_id;
    std::string launch_command;
    std::optional<std::vector<DBOperations>> db_operations;
};

struct ExecDataInterface {
    std::optional<std::string> job_id;
    std::optional<std::string> cluster_name;
    uint64_t exec_id;
    uint64_t start_time;
    std::string json;
    std::string path;
    std::string command;
    std::optional<std::vector<ProcessDataInterface>> process_data_interface;
};

struct JobDataInterface {
    uint64_t job_id;
    std::string cluster_name;
    std::string job_name;
    std::string username;
    uint64_t start_time;
    uint64_t end_time;
    std::string path;
    std::string json;
};

using JobInterfaceDataRows = std::vector<JobDataInterface>;
using ExecDataInterfaceRows = std::vector<ExecData>;
using DBOperationsRows = std::unordered_map<std::string, Operations>;

struct DBInterfaceData {
    RequestType request_type{};
    std::variant<JobInterfaceDataRows, ExecDataInterfaceRows, ProcessMapDB,
                 DBOperationsRows>
        db_data;
};
