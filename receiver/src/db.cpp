#include <cstdint>
#include <db.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "model.hpp"
#include "sqlite3.h"

DB::DB() {
    std::string base_path = "/dev/shm/libcprov";
    std::filesystem::create_directories(base_path);
    // db_file_ = (base_path + "/libcprov.db").c_str();
    db_file_ = "/dev/shm/libcprov/libcprov.db";
}

void DB::build_tables() {
    sqlite3* db = nullptr;
    int db_connection = sqlite3_open(db_file_.c_str(), &db);
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    char* err = nullptr;
    const char* db_tables =
        "CREATE TABLE IF NOT EXISTS env_variables_hash_pairs ("
        "  env_variables_hash INTEGER PRIMARY KEY,"
        "  env_variables_json TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS jobs ("
        "  job_id INTEGER NOT NULL,"
        "  cluster_name TEXT NOT NULL,"
        "  job_name TEXT NOT NULL,"
        "  username TEXT NOT NULL,"
        "  start_time INTEGER NOT NULL,"
        "  end_time INTEGER NOT NULL,"
        "  path TEXT NOT NULL,"
        "  json TEXT NOT NULL,"
        "  PRIMARY KEY (job_id, cluster_name)"
        ");"
        "CREATE TABLE IF NOT EXISTS execs ("
        "  exec_id INTEGER PRIMARY KEY,"
        "  job_id INTEGER NOT NULL,"
        "  cluster_name TEXT NOT NULL,"
        "  start_time INTEGER NOT NULL,"
        "  path TEXT NOT NULL,"
        "  json TEXT NOT NULL,"
        "  command TEXT NOT NULL,"
        "  FOREIGN KEY (job_id, cluster_name)"
        "    REFERENCES jobs(job_id, cluster_name)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS processes ("
        "  process_id INTEGER PRIMARY KEY,"
        "  exec_id INTEGER NOT NULL,"
        "  launch_command TEXT NOT NULL,"
        "  env_variables_hash INTEGER NOT NULL,"
        "  FOREIGN KEY (exec_id)"
        "    REFERENCES execs(exec_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE,"
        "  FOREIGN KEY (env_variables_hash)"
        "    REFERENCES env_variables_hash_pairs(env_variables_hash)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS execute_mappings ("
        "  exec_id INTEGER NOT NULL,"
        "  parent_process_id INTEGER NOT NULL,"
        "  child_process_id INTEGER NOT NULL,"
        "  PRIMARY KEY (exec_id, parent_process_id, child_process_id),"
        "  FOREIGN KEY (exec_id)"
        "    REFERENCES execs(exec_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE,"
        "  FOREIGN KEY (parent_process_id)"
        "    REFERENCES processes(process_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE,"
        "  FOREIGN KEY (child_process_id)"
        "    REFERENCES processes(process_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS operations ("
        "  process_id INTEGER NOT NULL,"
        "  path TEXT NOT NULL,"
        "  read INTEGER NOT NULL DEFAULT 0,"
        "  write INTEGER NOT NULL DEFAULT 0,"
        "  deleted INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (process_id, path),"
        "  FOREIGN KEY (process_id)"
        "    REFERENCES processes(process_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS renames ("
        "  exec_id INTEGER NOT NULL,"
        "  original_path TEXT NOT NULL,"
        "  new_path TEXT NOT NULL,"
        "  PRIMARY KEY (exec_id, original_path, new_path),"
        "  FOREIGN KEY (exec_id)"
        "    REFERENCES execs(exec_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE CASCADE"
        ");";
    db_connection = sqlite3_exec(db, db_tables, nullptr, nullptr, &err);
    sqlite3_close(db);
}

void DB::set_current_job(uint64_t job_id, const std::string& cluster_name) {
    current_job_ = active_jobs_[std::to_string(job_id) + cluster_name];
}
void DB::init_job(uint64_t job_id, const std::string& cluster_name) {
    JobDBContext job_db_context;
    sqlite3_open(db_file_.c_str(), &job_db_context.db);
    sqlite3_exec(job_db_context.db, "PRAGMA foreign_keys = ON;", nullptr,
                 nullptr, nullptr);
    sqlite3_exec(job_db_context.db, "BEGIN TRANSACTION;", nullptr, nullptr,
                 nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO jobs(job_id, cluster_name, job_name, "
                       "username, start_time, end_time, path, json) "
                       "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                       -1, &job_db_context.insert_job, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "UPDATE jobs SET end_time = ? WHERE job_id = ? AND "
                       "cluster_name = ?;",
                       -1, &job_db_context.update_end_time_job, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO execs(job_id, cluster_name, start_time, "
                       "path, json, command) "
                       "VALUES (?, ?, ?, ?, ?, ?);",
                       -1, &job_db_context.insert_exec, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO processes(exec_id, launch_command, "
                       "env_variables_hash) VALUES "
                       "(?, ?, ?);",
                       -1, &job_db_context.insert_process, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO execute_mappings(exec_id, "
                       "parent_process_id, child_process_id) VALUES (?, ?, ?);",
                       -1, &job_db_context.insert_execute_mapping, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO operations(process_id, path, read, write, "
                       "deleted) VALUES (?, ?, ?, ?, ?);",
                       -1, &job_db_context.insert_operations, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO renames(exec_id, original_path, new_path)"
                       " VALUES (?, ?, ?);",
                       -1, &job_db_context.insert_rename, nullptr);
    sqlite3_prepare_v2(job_db_context.db,
                       "INSERT INTO "
                       "env_variables_hash_pairs(env_variables_hash, "
                       "env_variables_json ) VALUES (?, ?);",
                       -1, &job_db_context.insert_variable_hash_pair, nullptr);
    active_jobs_[std::to_string(job_id) + cluster_name] = job_db_context;
}
void DB::finish_job(uint64_t job_id, const std::string& cluster_name) {
    sqlite3_exec(current_job_.db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(current_job_.insert_job);
    sqlite3_finalize(current_job_.update_end_time_job);
    sqlite3_finalize(current_job_.insert_exec);
    sqlite3_finalize(current_job_.insert_process);
    sqlite3_finalize(current_job_.insert_execute_mapping);
    sqlite3_finalize(current_job_.insert_operations);
    sqlite3_finalize(current_job_.insert_rename);
    sqlite3_finalize(current_job_.insert_variable_hash_pair);
    sqlite3_close(current_job_.db);
    current_job_ = {};
    active_jobs_.erase(std::to_string(job_id) + cluster_name);
}
void DB::commit_job() {
    sqlite3_exec(current_job_.db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_exec(current_job_.db, "BEGIN TRANSACTION;", nullptr, nullptr,
                 nullptr);
}
void DB::add_job(uint64_t job_id, const std::string& cluster_name,
                 uint64_t start_time, const std::string& job_name,
                 const std::string& username, const std::string& path,
                 const std::string& json) {
    sqlite3_bind_int64(current_job_.insert_job, 1, job_id);
    sqlite3_bind_text(current_job_.insert_job, 2, cluster_name.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_job, 3, job_name.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_job, 4, username.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int64(current_job_.insert_job, 5, start_time);
    sqlite3_bind_int64(current_job_.insert_job, 6, -1);
    sqlite3_bind_text(current_job_.insert_job, 7, path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_job, 8, json.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_step(current_job_.insert_job);
    sqlite3_reset(current_job_.insert_job);
}
void DB::set_job_end_time(uint64_t job_id, const std::string& cluster_name,
                          uint64_t end_time) {
    sqlite3_bind_int64(current_job_.update_end_time_job, 1, end_time);
    sqlite3_bind_int64(current_job_.update_end_time_job, 2, job_id);
    sqlite3_bind_text(current_job_.update_end_time_job, 3, cluster_name.c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_step(current_job_.update_end_time_job);
    sqlite3_reset(current_job_.update_end_time_job);
}
uint64_t DB::add_exec(uint64_t job_id, const std::string& cluster_name,
                      uint64_t start_time, const std::string& path,
                      const std::string& json, const std::string& command) {
    sqlite3_bind_int64(current_job_.insert_exec, 1, job_id);
    sqlite3_bind_text(current_job_.insert_exec, 2, cluster_name.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int64(current_job_.insert_exec, 3, start_time);
    sqlite3_bind_text(current_job_.insert_exec, 4, path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_exec, 5, json.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_exec, 6, command.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_step(current_job_.insert_exec);
    uint64_t exec_id =
        static_cast<uint64_t>(sqlite3_last_insert_rowid(current_job_.db));
    sqlite3_reset(current_job_.insert_exec);
    return exec_id;
}
uint64_t DB::add_process(uint64_t exec_id, const std::string& launch_command,
                         uint64_t env_variables_hash) {
    sqlite3_bind_int64(current_job_.insert_process, 1, exec_id);
    sqlite3_bind_text(current_job_.insert_process, 2, launch_command.c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(current_job_.insert_process, 3, env_variables_hash);
    sqlite3_step(current_job_.insert_process);
    uint64_t process_id = (uint64_t)sqlite3_last_insert_rowid(current_job_.db);
    sqlite3_reset(current_job_.insert_process);
    sqlite3_clear_bindings(current_job_.insert_process);
    return process_id;
}
void DB::add_execute_mapping(uint64_t exec_id, uint64_t parent_process_id,
                             uint64_t child_process_id) {
    sqlite3_bind_int64(current_job_.insert_execute_mapping, 1, exec_id);
    sqlite3_bind_int64(current_job_.insert_execute_mapping, 2,
                       parent_process_id);
    sqlite3_bind_int64(current_job_.insert_execute_mapping, 3,
                       child_process_id);
    sqlite3_step(current_job_.insert_execute_mapping);
    sqlite3_reset(current_job_.insert_execute_mapping);
}
void DB::add_operations(uint64_t process_id, const std::string& path, bool read,
                        bool write, bool deleted) {
    sqlite3_bind_int64(current_job_.insert_operations, 1, process_id);
    sqlite3_bind_text(current_job_.insert_operations, 2, path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(current_job_.insert_operations, 3, read);
    sqlite3_bind_int(current_job_.insert_operations, 4, write);
    sqlite3_bind_int(current_job_.insert_operations, 5, deleted);
    sqlite3_step(current_job_.insert_operations);
    sqlite3_reset(current_job_.insert_operations);
}
void DB::add_renames(uint64_t exec_id, const std::string& original_path,
                     const std::string& new_path) {
    sqlite3_bind_int64(current_job_.insert_rename, 1, exec_id);
    sqlite3_bind_text(current_job_.insert_rename, 2, original_path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(current_job_.insert_rename, 3, new_path.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_step(current_job_.insert_rename);
    sqlite3_reset(current_job_.insert_rename);
}
void DB::add_variable_hash_pair(uint64_t env_variables_hash,
                                const std::string& env_variables_json) {
    sqlite3_bind_int64(current_job_.insert_variable_hash_pair, 1,
                       env_variables_hash);
    sqlite3_bind_text(current_job_.insert_variable_hash_pair, 2,
                      env_variables_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(current_job_.insert_variable_hash_pair);
    sqlite3_reset(current_job_.insert_variable_hash_pair);
}

sqlite3* DB::open_db() {
    sqlite3* db = nullptr;
    sqlite3_open(db_file_.c_str(), &db);
    return db;
}

void DB::close_db(sqlite3* db) { sqlite3_close(db); }

std::string DB::col_text(sqlite3_stmt* st, int i) {
    const unsigned char* p = sqlite3_column_text(st, i);
    return p ? reinterpret_cast<const char*>(p) : std::string{};
}

uint64_t DB::col_u64(sqlite3_stmt* st, int i) {
    return static_cast<uint64_t>(sqlite3_column_int64(st, i));
}

bool DB::col_bool(sqlite3_stmt* st, int i) {
    return sqlite3_column_int(st, i) != 0;
}

std::unordered_map<std::string, Operations> DB::read_operation_map(
    sqlite3* db, uint64_t process_id) {
    std::unordered_map<std::string, Operations> out;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT path, read, write, deleted FROM operations WHERE process_id=?;",
        -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)process_id);
    while (sqlite3_step(st) == SQLITE_ROW) {
        std::string path = col_text(st, 0);
        Operations ops;
        ops.read = col_bool(st, 1);
        ops.write = col_bool(st, 2);
        ops.deleted = col_bool(st, 3);
        out.emplace(std::move(path), ops);
    }
    sqlite3_finalize(st);
    return out;
}

ProcessMapDB DB::read_processes(sqlite3* db, uint64_t exec_id, bool add_files) {
    ProcessMapDB process_map_db;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
                       "SELECT process_id, launch_command, env_variables_hash "
                       "FROM processes WHERE exec_id=? ORDER BY process_id;",
                       -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)exec_id);
    Process process = {};
    while (sqlite3_step(st) == SQLITE_ROW) {
        process = {};
        uint64_t process_id = col_u64(st, 0);
        process.process_command = col_text(st, 1);
        process.env_variable_hash = col_u64(st, 2);
        process_map_db.emplace(process_id, std::move(process));
    }
    if (add_files) {
        for (auto& [process_id, process] : process_map_db) {
            process.operation_map = read_operation_map(db, process_id);
        }
    }
    sqlite3_finalize(st);
    return process_map_db;
}

ExecuteSetMapDB DB::read_execute_set_map(sqlite3* db, uint64_t exec_id) {
    ExecuteSetMapDB out;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT parent_process_id, child_process_id FROM execute_mappings "
        "WHERE exec_id=? ORDER BY parent_process_id, child_process_id;",
        -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)exec_id);
    while (sqlite3_step(st) == SQLITE_ROW) {
        uint64_t parent_id = col_u64(st, 0);
        uint64_t child_id = col_u64(st, 1);
        out[parent_id].insert(child_id);
    }
    sqlite3_finalize(st);
    return out;
}

RenameMap DB::read_rename_map(sqlite3* db, uint64_t exec_id) {
    RenameMap out;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(
        db, "SELECT original_path, new_path FROM renames WHERE exec_id=?;", -1,
        &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)exec_id);
    while (sqlite3_step(st) == SQLITE_ROW) {
        std::string orig = col_text(st, 0);
        std::string nw = col_text(st, 1);
        out.emplace(std::move(orig), std::move(nw));
    }
    sqlite3_finalize(st);
    return out;
}

EnvVariableHashPairs DB::read_env_pairs_for_exec(sqlite3* db,
                                                 uint64_t exec_id) {
    std::unordered_set<uint64_t> hashes;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT DISTINCT env_variables_hash FROM processes WHERE exec_id=?;",
        -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)exec_id);
    while (sqlite3_step(st) == SQLITE_ROW) hashes.insert(col_u64(st, 0));
    sqlite3_finalize(st);
    EnvVariableHashPairs out;
    if (hashes.empty()) return out;
    std::string q =
        "SELECT env_variables_hash, env_variables_json FROM "
        "env_variables_hash_pairs WHERE env_variables_hash IN (";
    bool first = true;
    for (size_t i = 0; i < hashes.size(); ++i) {
        if (!first) q += ",";
        q += "?";
        first = false;
    }
    q += ");";
    sqlite3_prepare_v2(db, q.c_str(), -1, &st, nullptr);
    int idx = 1;
    for (uint64_t h : hashes) sqlite3_bind_int64(st, idx++, (sqlite3_int64)h);
    while (sqlite3_step(st) == SQLITE_ROW)
        out.emplace(col_u64(st, 0), col_text(st, 1));
    sqlite3_finalize(st);
    return out;
}

std::vector<ExecData> DB::read_execs(sqlite3* db, uint64_t job_id,
                                     const std::string& cluster_name,
                                     bool from_visiualizer, bool add_processes,
                                     bool add_files) {
    std::vector<ExecData> exec_data_entries;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
                       "SELECT exec_id, start_time, path, json, command "
                       "FROM execs "
                       "WHERE job_id=? AND cluster_name=? ORDER BY exec_id;",
                       -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)job_id);
    sqlite3_bind_text(st, 2, cluster_name.c_str(), -1, SQLITE_TRANSIENT);
    ExecData exec = {};
    while (sqlite3_step(st) == SQLITE_ROW) {
        exec = {};
        exec.exec_id = col_u64(st, 0);
        exec.start_time = col_u64(st, 1);
        exec.path = col_text(st, 2);
        exec.json = col_text(st, 3);
        exec.command = col_text(st, 4);
        exec_data_entries.push_back(exec);
    }
    sqlite3_finalize(st);
    for (ExecData& exec : exec_data_entries) {
        uint64_t exec_id = exec.exec_id.value();
        if (from_visiualizer) {
            exec.process_map_db = read_processes(db, exec_id, true);
            exec.execute_set_map_db = read_execute_set_map(db, exec_id);
            exec.rename_map = read_rename_map(db, exec_id);
            exec.env_variables_hash_to_variables =
                read_env_pairs_for_exec(db, exec_id);
        } else if (add_processes || add_files) {
            exec.process_map_db = read_processes(db, exec_id, add_files);
        }
    }
    return exec_data_entries;
}

JobData DB::get_job_data(uint64_t job_id, const std::string& cluster_name) {
    JobData out{};
    out.succeded = false;
    sqlite3* db = nullptr;
    sqlite3_open(db_file_.c_str(), &db);
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT job_name, username, start_time, end_time, path, json "
        "FROM jobs WHERE job_id=? AND cluster_name=?;",
        -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, (sqlite3_int64)job_id);
    sqlite3_bind_text(st, 2, cluster_name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) == SQLITE_ROW) {
        out.succeded = true;
        out.job_name = col_text(st, 0);
        out.username = col_text(st, 1);
        out.start_time = col_u64(st, 2);
        out.end_time = col_u64(st, 3);
        out.path = col_text(st, 4);
        out.json = col_text(st, 5);
        out.exec_data_vector =
            read_execs(db, job_id, cluster_name, true, false, false);
    }
    sqlite3_finalize(st);
    sqlite3_close(db);
    return out;
}

JobInterfaceDataRows DB::get_job_interface_data(sqlite3* db, std::string user,
                                                uint64_t before,
                                                uint64_t after) {
    JobInterfaceDataRows job_data_interface_rows{};
    // sqlite3* db = nullptr;
    //  sqlite3_open(db_file_.c_str(), &db);
    sqlite3_stmt* st = nullptr;
    if (after != 0 && before != 0) {
        sqlite3_prepare_v2(
            db,
            "SELECT job_id, cluster_name, job_name, username, start_time, "
            "end_time, path, json "
            "FROM jobs WHERE username=? AND start_time>=? AND end_time<=?;",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 2, (sqlite3_int64)after);
        sqlite3_bind_int64(st, 3, (sqlite3_int64)before);
    } else if (after != 0) {
        sqlite3_prepare_v2(
            db,
            "SELECT job_id, cluster_name, job_name, username, start_time, "
            "end_time, path, json "
            "FROM jobs WHERE username=? AND start_time>=?;",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 2, (sqlite3_int64)after);
    } else if (before != 0) {
        sqlite3_prepare_v2(
            db,
            "SELECT job_id, cluster_name, job_name, username, start_time, "
            "end_time, path, json "
            "FROM jobs WHERE username=? AND end_time<=?;",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 2, (sqlite3_int64)before);
    } else {
        sqlite3_prepare_v2(
            db,
            "SELECT job_id, cluster_name, job_name, username, start_time, "
            "end_time, path, json "
            "FROM jobs WHERE username=?;",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
    }
    JobDataInterface job_data_interface = {};
    while (sqlite3_step(st) == SQLITE_ROW) {
        job_data_interface = {};
        job_data_interface.job_id = col_u64(st, 0);
        job_data_interface.cluster_name = col_text(st, 1);
        job_data_interface.job_name = col_text(st, 2);
        job_data_interface.username = col_text(st, 3);
        job_data_interface.start_time = col_u64(st, 4);
        job_data_interface.end_time = col_u64(st, 5);
        job_data_interface.path = col_text(st, 6);
        job_data_interface.json = col_text(st, 7);
        job_data_interface_rows.push_back(job_data_interface);
    }
    sqlite3_finalize(st);
    // sqlite3_close(db);
    return job_data_interface_rows;
}
