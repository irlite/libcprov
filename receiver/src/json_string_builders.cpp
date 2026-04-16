#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.hpp"

std::string build_json_object(
    const std::unordered_map<std::string, std::string>& json_object_pairs,
    bool values_are_strings) {
    std::ostringstream json_object_string;
    json_object_string << "{";
    bool first = true;
    for (const auto& [key, value] : json_object_pairs) {
        if (!first) json_object_string << ",";
        first = false;
        if (values_are_strings) {
            json_object_string << R"(")" << key << R"(":")" << value << R"(")";
        } else {
            json_object_string << R"(")" << key << R"(":)" << value;
        }
    }
    json_object_string << "}";
    return json_object_string.str();
}

std::string build_json_array(
    const std::vector<std::string>& json_object_vector) {
    std::ostringstream json_array;
    json_array << "[";
    bool first = true;
    std::string maybe_comma;
    for (const std::string& json_object : json_object_vector) {
        maybe_comma = ",";
        if (first) {
            maybe_comma = "";
            first = false;
        }
        json_array << maybe_comma;
        json_array << json_object;
    }
    json_array << "]";
    return json_array.str();
}

std::string build_execute_map_json(const ExecuteSetMapDB& execute_set_map_db) {
    std::vector<std::string> execute_map_json_strings;
    for (auto& [parent_process_id, child_process_id_set] : execute_set_map_db) {
        std::vector<std::string> child_process_id_vector;
        child_process_id_vector.reserve(child_process_id_set.size());
        for (uint64_t child_process_id : child_process_id_set) {
            child_process_id_vector.push_back(std::to_string(child_process_id));
        }
        std::string execute_map_json =
            (R"({"parent_process_id":)" + std::to_string(parent_process_id) +
             R"(,"child_process_id_array":)" +
             build_json_array(child_process_id_vector) + "}");
        execute_map_json_strings.push_back(execute_map_json);
    }
    return build_json_array(execute_map_json_strings);
}

std::string build_env_variables_map_json(
    std::unordered_map<uint64_t, std::string> env_variables_hash_to_variables) {
    std::vector<std::string> env_variables_map_json_strings;
    for (auto& [env_variable_hash, env_variables_string] :
         env_variables_hash_to_variables) {
        std::string env_variables_map_json_string =
            R"({"env_variables_hash":)" + std::to_string(env_variable_hash) +
            R"(,"env_variables_array":)" + env_variables_string + R"(})";
        env_variables_map_json_strings.push_back(env_variables_map_json_string);
    }
    return build_json_array(env_variables_map_json_strings);
}

std::string build_operations_json(
    std::unordered_map<std::string, Operations> operation_map) {
    std::unordered_map<std::string, std::string> operation_map_json_format;
    for (auto& [path, operations] : operation_map) {
        std::vector<std::string> operation_strings;
        if (operations.read) {
            operation_strings.push_back(R"("read")");
        }
        if (operations.write) {
            operation_strings.push_back(R"("write")");
        }
        if (operations.deleted) {
            operation_strings.push_back(R"("deleted")");
        }
        operation_map_json_format[path] = build_json_array(operation_strings);
    }
    return build_json_object(operation_map_json_format, false);
}

std::string build_processes_data_json(ProcessMapDB process_map_db) {
    std::vector<std::string> process_json_vector;
    for (auto& [process_id, process] : process_map_db) {
        std::string process_json =
            R"({"process_command":")" + process.process_command +
            R"(","process_id":)" + std::to_string(process_id) +
            R"(,"env_variable_hash":)" +
            std::to_string(process.env_variable_hash) + R"(,"operations":)" +
            build_operations_json(process.operation_map) + R"(})";
        process_json_vector.push_back(process_json);
    }
    return build_json_array(process_json_vector);
}

std::string build_exec_json(ExecData exec_data) {
    std::string env_variable_hash_pair_array_string =
        build_env_variables_map_json(exec_data.env_variables_hash_to_variables);
    std::string execute_map_json =
        build_execute_map_json(exec_data.execute_set_map_db);
    std::string rename_map_json = build_json_object(exec_data.rename_map, true);
    std::string json_string =
        R"({"start_time":)" + std::to_string(exec_data.start_time.value()) +
        R"(,"exec_id":)" + std::to_string(exec_data.exec_id.value()) +
        R"(,"processes":)" +
        build_processes_data_json(std::move(exec_data.process_map_db)) +
        R"(,"execute_map":)" + execute_map_json + R"(,"rename_map":)" +
        rename_map_json + R"(,"env_variable_hash_pair_array":)" +
        env_variable_hash_pair_array_string + R"(,"json":")" + exec_data.json +
        R"(","path":")" + exec_data.path + R"(","command":")" +
        exec_data.command + R"("})";
    return json_string;
}

std::string convert_job_data_to_json(JobData job_data) {
    if (!job_data.succeded) {
        return R"({"type":"error","payload":{"error":")"
               "no job with the combination of this job id and cluster name "
               "exists."
               R"("}})";
    }
    std::vector<std::string> exec_strings;
    for (ExecData exec_data : job_data.exec_data_vector) {
        exec_strings.push_back(build_exec_json(exec_data));
    }
    std::string execs_json = build_json_array(exec_strings);
    return R"({"type":"prov_data","payload":{"execs":)" + execs_json +
           R"(,"job_name":")" + job_data.job_name + R"(","username":")" +
           job_data.username + R"(","start_time":)" +
           std::to_string(job_data.start_time) + R"(,"end_time":)" +
           std::to_string(job_data.end_time) + R"(,"path":")" + job_data.path +
           R"(","json":")" + job_data.json + R"("}})";
}

std::string convert_db_interface_data_to_json(
    DBInterfaceData db_interface_data) {
    switch (db_interface_data.request_type) {
        case RequestType::JobsQuery: {
            const auto& rows =
                std::get<JobInterfaceDataRows>(db_interface_data.db_data);
            std::vector<std::string> jobs_json;
            jobs_json.reserve(rows.size());
            for (const auto& row : rows) {
                std::string job_json =
                    R"({"job_id":)" + std::to_string(row.job_id) +
                    R"(,"cluster_name":")" + row.cluster_name +
                    R"(","job_name":")" + row.job_name + R"(","username":")" +
                    row.username + R"(","start_time":)" +
                    std::to_string(row.start_time) + R"(,"end_time":)" +
                    std::to_string(row.end_time) + R"(,"path":")" + row.path +
                    R"(","json":")" + row.json + R"("})";
                jobs_json.push_back(std::move(job_json));
            }
            return R"({"jobs":)" + build_json_array(jobs_json) + R"(})";
        }
        case RequestType::ExecsQuery: {
            const auto& rows =
                std::get<ExecDataInterfaceRows>(db_interface_data.db_data);
            std::vector<std::string> execs_json;
            execs_json.reserve(rows.size());
            for (const auto& row : rows) {
                execs_json.push_back(build_exec_json(row));
            }
            return R"({"execs":)" + build_json_array(execs_json) + R"(})";
        }
        case RequestType::ProcessesQuery: {
            auto processes_json = build_processes_data_json(
                std::get<ProcessMapDB>(db_interface_data.db_data));
            return R"({"processes":)" + processes_json + R"(})";
        }
        case RequestType::FileQuery: {
            const auto& op_map =
                std::get<DBOperationsRows>(db_interface_data.db_data);
            std::unordered_map<std::string, std::string> as_json;
            as_json.reserve(op_map.size());
            for (const auto& [path, ops] : op_map) {
                std::vector<std::string> op_strings;
                if (ops.read) op_strings.push_back(R"("read")");
                if (ops.write) op_strings.push_back(R"("write")");
                if (ops.deleted) op_strings.push_back(R"("deleted")");
                as_json[path] = build_json_array(op_strings);
            }
            return R"({"files":)" + build_json_object(as_json, false) + R"(})";
        }
    }
}

std::string wrap_retriever_json(std::string payload, bool status) {
    return std::string(R"({"status":)") + (status ? "0" : "1") +
           R"(,"payload":)" + payload + "}";
}

std::string build_exec_operations_json(
    const std::unordered_map<
        uint64_t, std::unordered_map<std::string, Operations>>& exec_map) {
    std::unordered_map<std::string, std::string> exec_json_map;
    for (const auto& [exec_id, file_map] : exec_map) {
        std::unordered_map<std::string, std::string> file_json_map;
        for (const auto& [path, ops] : file_map) {
            std::vector<std::string> op_array;
            if (ops.read) op_array.push_back(R"("read")");
            if (ops.write) op_array.push_back(R"("write")");
            if (ops.deleted) op_array.push_back(R"("deleted")");
            file_json_map[path] =
                R"({"performed_operations":)" + build_json_array(op_array) +
                R"(,"start_checksum":")" + ops.start_checksum +
                R"(","end_checksum":")" + ops.end_checksum + R"("})";
        }
        exec_json_map[std::to_string(exec_id)] =
            build_json_object(file_json_map, false);
    }
    return R"({"exec_operations_map":)" +
           build_json_object(exec_json_map, false) + R"(})";
}

std::string convert_retriever_data_to_json(DBRetrieverData db_retriever_data) {
    switch (db_retriever_data.request_type) {
        case RetrieverRequestType::Job:
        case RetrieverRequestType::ExecId: {
            const auto& exec_map = std::get<std::unordered_map<
                uint64_t, std::unordered_map<std::string, Operations>>>(
                db_retriever_data.operations_by_exec);
            std::string payload = build_exec_operations_json(exec_map);
            return wrap_retriever_json(payload, true);
        }
        case RetrieverRequestType::Checksum: {
            bool exists = std::get<bool>(db_retriever_data.operations_by_exec);
            std::string payload = R"({"content":{"checksum_exists":)" +
                                  std::string(exists ? "1" : "0") + R"(}})";
            return wrap_retriever_json(payload, true);
        }
    }
    return wrap_retriever_json(R"({"error_message":"error"})", false);
}
