#include <chrono>
#include <cstdint>
#include <filesystem>
#include <model.hpp>
#include <string>
#include <unordered_map>

std::string now_ns() {
    using namespace std::chrono;
    uint64_t ts =
        duration_cast<nanoseconds>(system_clock::now().time_since_epoch())
            .count();
    std::string ts_string = std::to_string(ts);
    return ts_string;
}

std::string build_header(const std::string& type,
                         const std::string& path_access,
                         const std::string& slurm_job_id,
                         const std::string& slurm_cluster_name) {
    return R"({"header":{"type":")" + type + R"(","job_id":)" + slurm_job_id +
           R"(,"cluster_name":")" + slurm_cluster_name + R"(","timestamp":)" +
           now_ns() + R"(})";
}

std::string build_start_json_output(const std::string& job_name,
                                    const std::string& username,
                                    const std::string& path_start,
                                    const std::string& json_start_extra) {
    std::string absolute_path_start = std::filesystem::canonical(path_start);
    return R"(,"payload":{"job_name":")" + job_name + R"(","username":")" +
           username + R"(","json":")" + json_start_extra + R"(","path":")" +
           absolute_path_start + R"("}})";
}

std::string build_end_json_output(const std::string& json_end_extra) {
    return R"(,"payload":{"json":")" + json_end_extra + R"("}})";
}

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

std::string build_execute_map_json(const ExecuteSetMap& execute_set_map) {
    std::vector<std::string> execute_map_json_strings;
    for (auto& [parent_process_id, child_process_id_set] : execute_set_map) {
        std::vector<std::string> child_process_id_vector;
        child_process_id_vector.reserve(child_process_id_set.size());
        for (std::string child_process_id : child_process_id_set) {
            child_process_id_vector.push_back(R"(")" + child_process_id +
                                              R"(")");
        }
        std::string execute_map_json =
            (R"({"parent_process_id":")" + parent_process_id +
             R"(","child_process_id_array":)" +
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

struct OperationsJsonFormat {
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    std::vector<std::string> deletes;
};

std::string build_operations_json(
    std::unordered_map<std::string, Operations> operation_map) {
    std::unordered_map<std::string, std::string> operation_map_json_format;
    for (auto& [path, operations] : operation_map) {
        std::vector<std::string> operation_strings;
        bool first = true;
        if (operations.read) {
            operation_strings.push_back(R"("read")");
        }
        if (operations.write) {
            operation_strings.push_back(R"("write")");
        }
        if (operations.deleted) {
            operation_strings.push_back(R"("deleted")");
        }
        std::string start_checksum =
            operations.start_checksum.empty()
                ? "null"
                : R"(")" + operations.start_checksum + R"(")";
        std::string end_checksum =
            operations.end_checksum.empty()
                ? "null"
                : R"(")" + operations.end_checksum + R"(")";
        operation_map_json_format[path] =
            R"({"performed_operations":)" +
            build_json_array(operation_strings) + R"(,"start_checksum":)" +
            start_checksum + R"(,"end_checksum":)" + end_checksum + "}";
    }
    return build_json_object(operation_map_json_format, false);
}

std::string build_processes_data_json(ProcessMap process_map) {
    bool first = true;
    std::vector<std::string> process_json_vector;
    for (auto& [process_id, process] : process_map) {
        std::string process_json =
            R"({"process_command":")" + process.process_name +
            R"(","process_id":")" + process_id + R"(","env_variable_hash":)" +
            std::to_string(process.env_variable_hash) + R"(,"operations":)" +
            build_operations_json(process.operation_map) + R"(})";
        process_json_vector.push_back(process_json);
    }
    return build_json_array(process_json_vector);
}

std::string build_exec_json_output(const std::string& path_exec,
                                   const std::string& json_exec,
                                   const std::string& cmd,
                                   ProcessedExecData processed_exec_data) {
    std::string type = "exec";
    std::string absolute_path_exec = std::filesystem::canonical(path_exec);
    std::string env_variable_hash_pair_array_string =
        build_env_variables_map_json(
            processed_exec_data.env_variables_hash_to_variables);
    std::string execute_map_json =
        build_execute_map_json(processed_exec_data.execute_set_map);
    std::string rename_map_json =
        build_json_object(processed_exec_data.rename_map, true);
    std::string json_string =
        R"(,"payload":{"processes":)" +
        build_processes_data_json(std::move(processed_exec_data.process_map)) +
        R"(,"execute_map":)" + execute_map_json + R"(,"rename_map":)" +
        rename_map_json + R"(,"env_variable_hash_pair_array":)" +
        env_variable_hash_pair_array_string + R"(,"json":")" + json_exec +
        R"(","path":")" + absolute_path_exec + R"(","command":")" + cmd +
        R"("}})";
    return json_string;
}
