#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

enum class SysOp {
    ProcessStart,
    ProcessEnd,
    Write,
    Read,
    Transfer,
    Rename,
    Exec,
    ExecFork,
    ExecFail,
    Unlink
};

struct EventHeader {
    uint64_t ts = 0;
    SysOp op;
    std::string raw_type;
};
struct AccessIn {
    std::string path_in;
    uint32_t count = 0;
};
struct AccessOut {
    std::string path_out;
    uint32_t count = 0;
};
struct Transfer {
    std::string path_read;
    std::string path_write;
};
struct Rename {
    std::string original_path;
    std::string new_path;
};
struct ExecCall {
    std::optional<uint64_t> pid;
};
struct SpawnCall {
    uint64_t child_pid = -1;
    std::string target;
};
struct ForkCall {
    uint64_t child_pid = -1;
};
struct ProcessStart {
    uint64_t pid = 0;
    uint64_t ppid = 0;
    std::string process_name;
    std::string env_variables;
    // uint64_t process_hash;
    std::optional<uint64_t> env_variables_hash;
    // std::string step_id;
    // std::string step_name;
};
struct ProcessEnd {};

using EventPayload = std::variant<std::string, Transfer, Rename, ExecCall,
                                  ProcessStart, ProcessEnd>;

struct Event {
    uint64_t ts = 0;
    SysOp operation;
    std::string process_id;
    EventPayload event_payload;
};

using EventsByFile = std::vector<std::vector<Event>>;

/*struct BackupOperations {
    bool read = false;
    bool write = false;
    bool execute = false;
};*/

// using OperationsDataBackupFormat =
//     std::unordered_map<std::string, BackupOperations>;

using EnvVariablesHashToVariables = std::unordered_map<uint64_t, std::string>;
/*uint64_t hash;
std::string env_variables;
}
;*/

using ExecuteSetMap =
    std::unordered_map<std::string, std::unordered_set<std::string>>;

struct Operations {
    bool read = false;
    bool write = false;
    bool deleted = false;
    std::string start_checksum;
    std::string end_checksum;
};

struct LinuxProcess {
    uint64_t ppid;
    uint64_t start_time;
    uint64_t end_time;
    std::string process_id;
    // std::string process_name;
    // uint64_t env_variable_hash;
};

using LinuxProcessMap = std::unordered_map<uint64_t, std::vector<LinuxProcess>>;

struct Process {
    std::string process_name;
    uint64_t env_variable_hash;
    // std::vector<std::string> process_json_operation_objects_;
    std::unordered_map<std::string, Operations> operation_map;
};
using ProcessMap = std::unordered_map<std::string, Process>;
struct ProcessedExecData {
    ProcessMap process_map;
    std::unordered_map<std::string, std::string> rename_map;
    ExecuteSetMap execute_set_map;
    std::unordered_map<uint64_t, std::string> env_variables_hash_to_variables;
};
struct ProcessedInjectorData {
    ProcessedExecData processed_exec_data;
};

using ChecksumsByFiles = std::unordered_map<std::string, std::string>;
