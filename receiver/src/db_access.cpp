#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_set>

#include "db.hpp"
#include "model.hpp"

std::unordered_map<std::string, uint64_t> add_processes(
    DB& db, uint64_t exec_id, ProcessMap process_map) {
    std::unordered_map<std::string, uint64_t> process_id_to_process_db_id;
    for (auto& [process_id, process] : process_map) {
        process_id_to_process_db_id[process_id] = db.add_process(
            exec_id, process.process_command, process.env_variable_hash);
    }
    return process_id_to_process_db_id;
}

void add_execute_mappings(
    DB& db, uint64_t exec_id, const ExecuteSetMap& execute_set_map,
    std::unordered_map<std::string, uint64_t> process_id_to_process_db_id) {
    for (auto& [parent_process_id, child_process_ids] : execute_set_map) {
        uint64_t parent_process_db_id =
            process_id_to_process_db_id[parent_process_id];
        uint64_t child_process_db_id;
        for (std::string child_process_id : child_process_ids) {
            child_process_db_id = process_id_to_process_db_id[child_process_id];
            db.add_execute_mapping(exec_id, parent_process_db_id,
                                   child_process_db_id);
        }
    }
}

void add_operations(
    DB& db, uint64_t exec_id, ProcessMap process_map,
    std::unordered_map<std::string, uint64_t> process_id_to_process_db_id) {
    for (auto& [process_id, process] : process_map) {
        uint64_t process_db_id = process_id_to_process_db_id[process_id];
        for (auto& [path, operations] : process.operation_map) {
            db.add_operations(process_db_id, path, operations.read,
                              operations.write, operations.deleted,
                              operations.start_checksum,
                              operations.end_checksum);
        }
    }
}

void add_renames(DB& db, uint64_t exec_id, RenameMap rename_map) {
    for (auto& [original_path, new_path] : rename_map) {
        db.add_renames(exec_id, original_path, new_path);
    }
}

void add_variable_hash_pairs(DB& db,
                             EnvVariableHashPairs env_variable_hash_pairs) {
    for (auto& [env_variable_hash, env_variables_json] :
         env_variable_hash_pairs) {
        db.add_variable_hash_pair(env_variable_hash, env_variables_json);
    }
}

void handleExecCase(DB& db, uint64_t job_id, const std::string& cluster_name,
                    uint64_t timestamp, const ExecData& exec_data) {}

void save_db_data(DB& db, const ParsedInjectorData& parsed_injector_data) {
    uint64_t job_id = parsed_injector_data.job_id;
    std::string cluster_name = parsed_injector_data.cluster_name;
    uint64_t timestamp = parsed_injector_data.timestamp;
    switch (parsed_injector_data.injector_data_type) {
        case InjectorDataType::End:
            db.set_current_job(job_id, cluster_name);
            db.set_job_end_time(job_id, cluster_name, timestamp);
            db.commit_job();
            db.finish_job(job_id, cluster_name);
            break;
        case InjectorDataType::Start: {
            db.init_job(job_id, cluster_name);
            StartData start_data =
                std::get<StartData>(parsed_injector_data.payload.value());
            std::string json_start = start_data.json;
            std::string path_start = start_data.path;
            db.set_current_job(job_id, cluster_name);
            db.add_job(job_id, cluster_name, timestamp, start_data.job_name,
                       start_data.username, path_start, json_start);
            db.commit_job();
            break;
        }
        case InjectorDataType::Exec: {
            ExecData exec_data =
                std::get<ExecData>(parsed_injector_data.payload.value());
            db.set_current_job(job_id, cluster_name);
            uint64_t exec_id =
                db.add_exec(job_id, cluster_name, timestamp, exec_data.path,
                            exec_data.json, exec_data.command);
            add_variable_hash_pairs(db,
                                    exec_data.env_variables_hash_to_variables);
            std::unordered_map<std::string, uint64_t>
                process_id_to_process_db_id =
                    add_processes(db, exec_id, exec_data.process_map);
            add_operations(db, exec_id, exec_data.process_map,
                           process_id_to_process_db_id);
            add_execute_mappings(db, exec_id, exec_data.execute_set_map,
                                 process_id_to_process_db_id);
            add_renames(db, exec_id, exec_data.rename_map);
            db.commit_job();
        }
    }
}

JobData fetch_graph_db_data(JobIdentifier parsed_graph_request_data) {
    DB db = DB();
    return db.get_job_data(parsed_graph_request_data.job_id,
                           parsed_graph_request_data.cluster_name);
}

static inline uint64_t ddmmyyyy_to_ns_u64_utc(const std::string& ddmmyyyy) {
    int day = 0, month = 0, year = 0;
    if (std::sscanf(ddmmyyyy.c_str(), "%d.%d.%d", &day, &month, &year) != 3)
        return 0;
    std::chrono::year_month_day ymd{std::chrono::year{year},
                                    std::chrono::month{(unsigned)month},
                                    std::chrono::day{(unsigned)day}};
    if (!ymd.ok()) return 0;
    std::chrono::sys_days days_since_epoch{ymd};
    std::chrono::sys_time<std::chrono::nanoseconds> time_point =
        days_since_epoch;
    return (uint64_t)time_point.time_since_epoch().count();
}

DBInterfaceData fetch_db_interface_db_data(
    ParsedDBInterfaceRequestData parsed_db_interface_request_data) {
    DBInterfaceData db_interface_data;
    RequestType request_type = parsed_db_interface_request_data.request_type;
    db_interface_data.request_type = request_type;
    DB db = DB();
    sqlite3* sqlite3_db = db.open_db();
    switch (request_type) {
        case (RequestType::JobsQuery): {
            JobsQueryOpts job_query_opts =
                std::get<JobsQueryOpts>(parsed_db_interface_request_data.opts);
            uint64_t before_atomic_ts = 0;
            if (job_query_opts.before) {
                before_atomic_ts =
                    ddmmyyyy_to_ns_u64_utc(job_query_opts.before.value());
            }
            uint64_t after_atomic_ts = 0;
            if (job_query_opts.after) {
                after_atomic_ts =
                    ddmmyyyy_to_ns_u64_utc(job_query_opts.after.value());
            }
            db_interface_data.db_data =
                db.get_job_interface_data(sqlite3_db, job_query_opts.user,
                                          before_atomic_ts, after_atomic_ts);
            break;
        }
        case (RequestType::ExecsQuery): {
            ExecsQueryOpts exec_query_opts =
                std::get<ExecsQueryOpts>(parsed_db_interface_request_data.opts);
            db_interface_data.db_data = db.read_execs(
                sqlite3_db, exec_query_opts.job_id, exec_query_opts.cluster,
                false, exec_query_opts.list_with_processes,
                exec_query_opts.list_with_files);
            break;
        }
        case (RequestType::ProcessesQuery): {
            ProcessesQueryOpts processes_query_opts =
                std::get<ProcessesQueryOpts>(
                    parsed_db_interface_request_data.opts);
            db_interface_data.db_data =
                db.read_processes(sqlite3_db, processes_query_opts.exec_id,
                                  processes_query_opts.list_with_files);
            break;
        }
        case (RequestType::FileQuery): {
            FileQueryOpts file_query_opts =
                std::get<FileQueryOpts>(parsed_db_interface_request_data.opts);
            db_interface_data.db_data =
                db.read_operation_map(sqlite3_db, file_query_opts.process_id);
            break;
        }
    }
    db.close_db(sqlite3_db);
    return db_interface_data;
}

DBRetrieverData fetch_retriever_db_data(
    ParsedRetrieverData parsed_retriever_data) {
    std::variant<JobIdentifier, uint64_t, std::string>
        parsed_retriever_request =
            parsed_retriever_data.parsed_retriever_request;
    DBRetrieverData db_retriever_data;
    RetrieverRequestType retriever_request_type =
        parsed_retriever_data.request_type;
    db_retriever_data.request_type = retriever_request_type;
    DB db = DB();
    switch (retriever_request_type) {
        case (RetrieverRequestType::Job): {
            JobIdentifier job_identifier =
                std::get<JobIdentifier>(parsed_retriever_request);
            db_retriever_data.operations_by_exec = db.fetch_job_operations(
                job_identifier.job_id, job_identifier.cluster_name);
            break;
        }
        case (RetrieverRequestType::ExecId): {
            JobIdentifier job_identifier = db.fetch_job_identifier_from_exec_id(
                std::get<uint64_t>(parsed_retriever_request));
            db_retriever_data.operations_by_exec = db.fetch_job_operations(
                job_identifier.job_id, job_identifier.cluster_name);
            break;
        }
        case (RetrieverRequestType::Checksum): {
            db_retriever_data.operations_by_exec = db.check_if_checksum_exists(
                std::get<std::string>(parsed_retriever_request));
            break;
        }
    }
    return db_retriever_data;
}
