#pragma once
#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.hpp"

class DB {
   public:
    DB();
    void build_tables();
    void set_current_job(uint64_t job_id, const std::string& cluster_name);
    void init_job(uint64_t job_id, const std::string& cluster_name);
    void finish_job(uint64_t job_id, const std::string& cluster_name);
    void commit_job();
    void add_job(uint64_t job_id, const std::string& cluster_name,
                 uint64_t start_time, const std::string& job_name,
                 const std::string& username, const std::string& path,
                 const std::string& json);
    void set_job_end_time(uint64_t job_id, const std::string& cluster_name,
                          uint64_t end_time);
    uint64_t add_exec(uint64_t job_id, const std::string& cluster_name,
                      uint64_t start_time, const std::string& path,
                      const std::string& json, const std::string& command);
    uint64_t add_process(uint64_t exec_id, const std::string& launch_command,
                         uint64_t env_variables_hash);
    void add_execute_mapping(uint64_t exec_id, uint64_t parent_process_id,
                             uint64_t child_process_id);
    void add_operations(uint64_t process_id, const std::string& path, bool read,
                        bool write, bool deleted,
                        const std::string& start_checksum,
                        const std::string& end_checksum);
    void add_renames(uint64_t exec_id, const std::string& original_path,
                     const std::string& new_path);
    void add_variable_hash_pair(uint64_t env_variables_hash,
                                const std::string& env_variables_json);
    /*struct JobData {
        uint64_t slurm_id;
        std::string cluster_name;
        std::string path;
        std::string json;
        uint64_t start_time;
        uint64_t end_time;
        std::vector<ExecData> execs;
        EnvVariableHashPairs env_variabl_hash_pairs;
    };*/
    // JobData get_job_data(uint64_t slurm_id, const std::string& cluster_name);
    JobData get_job_data(uint64_t job_id, const std::string& cluster_name);
    JobInterfaceDataRows get_job_interface_data(sqlite3* db, std::string user,
                                                uint64_t before,
                                                uint64_t after);
    ProcessMapDB read_processes(sqlite3* db, uint64_t exec_id, bool add_files);
    std::vector<ExecData> read_execs(sqlite3* db, uint64_t job_id,
                                     const std::string& cluster_name,
                                     bool from_visiualizer, bool add_processes,
                                     bool add_files);
    std::unordered_map<std::string, Operations> read_operation_map(
        sqlite3* db, uint64_t process_id);
    std::unordered_map<uint64_t, std::unordered_map<std::string, Operations>>
    fetch_job_operations(uint64_t job_id, const std::string& cluster_name);
    JobIdentifier fetch_job_identifier_from_exec_id(
        uint64_t parsed_retriever_request);
    bool check_if_checksum_exists(std::string checksum);
    sqlite3* open_db();
    void close_db(sqlite3* db);

   private:
    std::string db_file_;
    struct JobDBContext {
        sqlite3* db = nullptr;
        sqlite3_stmt* insert_job = nullptr;
        sqlite3_stmt* update_end_time_job = nullptr;
        sqlite3_stmt* insert_exec = nullptr;
        sqlite3_stmt* insert_process = nullptr;
        sqlite3_stmt* insert_execute_mapping = nullptr;
        sqlite3_stmt* insert_operations = nullptr;
        sqlite3_stmt* insert_rename = nullptr;
        sqlite3_stmt* insert_variable_hash_pair = nullptr;
    };
    std::unordered_map<std::string, JobDBContext> active_jobs_;
    JobDBContext current_job_;
    std::string col_text(sqlite3_stmt* st, int i);
    uint64_t col_u64(sqlite3_stmt* st, int i);
    bool col_bool(sqlite3_stmt* st, int i);
    ExecuteSetMapDB read_execute_set_map(sqlite3* db, uint64_t exec_id);
    RenameMap read_rename_map(sqlite3* db, uint64_t exec_id);
    EnvVariableHashPairs read_env_pairs_for_exec(sqlite3* db, uint64_t exec_id);
    uint64_t pick_exec_id(sqlite3* db, uint64_t job_id,
                          const std::string& cluster_name);
};
