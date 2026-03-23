#include "parser.hpp"

#include <simdjson.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.hpp"

using namespace simdjson;

static std::string get_string(ondemand::object& obj, const char* name) {
    simdjson_result<std::string_view> result =
        obj.find_field_unordered(name).get_string();
    return std::string(result.value());
}

static uint64_t get_uint64(ondemand::object& obj, const char* name) {
    simdjson_result<uint64_t> result =
        obj.find_field_unordered(name).get_uint64();
    return result.value();
}

static std::vector<std::string> parse_json_string_array(
    simdjson::ondemand::object& obj, const std::string_view& name) {
    std::vector<std::string> out;
    auto arr = obj.find_field_unordered(name).get_array().value();
    for (simdjson::ondemand::value el : arr) {
        std::string_view sv = el.get_string().value();
        out.emplace_back(sv);
    }
    return out;
}

static std::unordered_map<std::string, std::string> parse_string_to_string_map(
    ondemand::object& json_string_to_string_map) {
    std::unordered_map<std::string, std::string> out;
    for (ondemand::field json_mapping : json_string_to_string_map) {
        std::string_view key_sv;
        (void)json_mapping.unescaped_key().get(key_sv);
        std::string key(key_sv);
        std::string_view value_sv;
        (void)json_mapping.value().get_string().get(value_sv);
        std::string value(value_sv);
        out.emplace(std::move(key), std::move(value));
    }
    return out;
}

static std::unordered_map<std::string, FileOperations> parse_file_operations(
    ondemand::object& json_files_object) {
    std::unordered_map<std::string, FileOperations> files_to_file_operations;
    for (ondemand::field json_file_mapping : json_files_object) {
        std::string_view file_path_sv;
        (void)json_file_mapping.unescaped_key().get(file_path_sv);
        std::string file_path(file_path_sv);
        ondemand::array operations_array;
        (void)json_file_mapping.value().get_array().get(operations_array);
        FileOperations& file_operations = files_to_file_operations[file_path];
        for (ondemand::value operation_value : operations_array) {
            std::string_view operation_sv;
            (void)operation_value.get_string().get(operation_sv);
            std::string operation_string(operation_sv);
            if (operation_string == "read") {
                file_operations.reads = true;
            } else if (operation_string == "write") {
                file_operations.writes = true;
            } else if (operation_string == "deleted") {
                file_operations.deletes = true;
            }
        }
    }
    return files_to_file_operations;
}

static ParsedJobsQuery parse_jobs_query(ondemand::object& root_object) {
    ParsedJobsQuery parsed_jobs_query;
    ondemand::array jobs_array = root_object["jobs"].get_array().value();
    for (ondemand::value job_value : jobs_array) {
        ondemand::object job_object = job_value.get_object().value();
        ParsedJobsQueryData parsed_jobs_query_data;
        parsed_jobs_query_data.job_id = get_uint64(job_object, "job_id");
        parsed_jobs_query_data.cluster_name =
            get_string(job_object, "cluster_name");
        parsed_jobs_query_data.job_name = get_string(job_object, "job_name");
        parsed_jobs_query_data.username = get_string(job_object, "username");
        parsed_jobs_query_data.start_time =
            get_uint64(job_object, "start_time");
        parsed_jobs_query_data.end_time = get_uint64(job_object, "end_time");
        parsed_jobs_query_data.path = get_string(job_object, "path");
        parsed_jobs_query_data.json = get_string(job_object, "json");
        parsed_jobs_query.emplace_back(std::move(parsed_jobs_query_data));
    }
    return parsed_jobs_query;
}

static ParsedProcessesQuery parse_processes_query(
    ondemand::object& root_object) {
    ParsedProcessesQuery parsed_processes_query;
    ondemand::array processes_array =
        root_object["processes"].get_array().value();
    for (ondemand::value process_value : processes_array) {
        ondemand::object process_object = process_value.get_object().value();
        ParsedProcessesQueryData parsed_processes_query_data;
        parsed_processes_query_data.process_command =
            get_string(process_object, "process_command");
        parsed_processes_query_data.process_id =
            get_uint64(process_object, "process_id");
        ondemand::object operations_object =
            process_object["operations"].get_object().value();
        ParsedFilesQueryData parsed_files_query_data;
        parsed_files_query_data.file_operations =
            parse_file_operations(operations_object);
        parsed_processes_query_data.operations.emplace_back(
            std::move(parsed_files_query_data));
        parsed_processes_query.emplace_back(
            std::move(parsed_processes_query_data));
    }
    return parsed_processes_query;
}

static std::unordered_map<uint64_t, std::vector<uint64_t>> parse_execute_map(
    ondemand::array execute_map_array) {
    std::unordered_map<uint64_t, std::vector<uint64_t>> executes;
    for (ondemand::value execute_map_value : execute_map_array) {
        ondemand::object execute_map_object =
            execute_map_value.get_object().value();
        uint64_t parent_process_id =
            get_uint64(execute_map_object, "parent_process_id");
        ondemand::array child_process_id_array =
            execute_map_object["child_process_id_array"].get_array().value();
        std::vector<uint64_t>& child_process_ids = executes[parent_process_id];
        for (ondemand::value child_process_id_value : child_process_id_array) {
            uint64_t child_process_id =
                child_process_id_value.get_uint64().value();
            child_process_ids.emplace_back(child_process_id);
        }
    }
    return executes;
}

static ParsedExecsQuery parse_execs_query(ondemand::object& root_object) {
    ParsedExecsQuery parsed_execs_query;
    ondemand::array execs_array = root_object["execs"].get_array().value();
    for (ondemand::value exec_value : execs_array) {
        ondemand::object exec_object = exec_value.get_object().value();
        ParsedExecsQueryData parsed_execs_query_data;
        parsed_execs_query_data.start_time =
            get_uint64(exec_object, "start_time");
        parsed_execs_query_data.exec_id = get_uint64(exec_object, "exec_id");
        parsed_execs_query_data.processes = parse_processes_query(exec_object);
        parsed_execs_query_data.executes =
            parse_execute_map(exec_object["execute_map"].get_array().value());
        ondemand::object rename_map_object =
            exec_object["rename_map"].get_object().value();
        parsed_execs_query_data.rename_map =
            parse_string_to_string_map(rename_map_object);
        parsed_execs_query_data.json = get_string(exec_object, "json");
        parsed_execs_query_data.path = get_string(exec_object, "path");
        parsed_execs_query_data.command = get_string(exec_object, "command");
        parsed_execs_query.emplace_back(std::move(parsed_execs_query_data));
    }
    return parsed_execs_query;
}

static ParsedFilesQuery parse_files_query(ondemand::object& root_object) {
    ParsedFilesQuery parsed_files_query;
    ondemand::object json_files_object =
        root_object["files"].get_object().value();
    ParsedFilesQueryData parsed_files_query_data;
    parsed_files_query_data.file_operations =
        parse_file_operations(json_files_object);
    parsed_files_query.emplace_back(std::move(parsed_files_query_data));
    return parsed_files_query;
}

ParsedQuery parse_db_interface_query_response(const std::string& response_body,
                                              RequestType request_type) {
    ondemand::parser response_parser;
    padded_string padded_response_body_string(response_body);
    auto response_document_result =
        response_parser.iterate(padded_response_body_string);
    ondemand::object root_object =
        response_document_result.get_object().value();
    switch (request_type) {
        case RequestType::JobsQuery: {
            return ParsedQuery{parse_jobs_query(root_object)};
        }
        case RequestType::ExecsQuery: {
            return ParsedQuery{parse_execs_query(root_object)};
        }
        case RequestType::ProcessesQuery: {
            return ParsedQuery{parse_processes_query(root_object)};
        }
        case RequestType::FileQuery: {
            return ParsedQuery{parse_files_query(root_object)};
        }
    }
    return ParsedQuery{ParsedJobsQuery{}};
}
