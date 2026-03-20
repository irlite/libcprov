#include "parser.hpp"

#include <simdjson.h>

#include <cstdint>
#include <string>

#include "model.hpp"

using namespace simdjson;

std::string get_string(ondemand::object& obj, const char* name) {
    simdjson_result<std::string_view> result =
        obj.find_field_unordered(name).get_string();
    return std::string(result.value());
}

uint64_t get_uint64(ondemand::object& obj, const char* name) {
    simdjson_result<uint64_t> result =
        obj.find_field_unordered(name).get_uint64();
    return result.value();
}

std::string get_array_json(simdjson::ondemand::object& obj,
                           const std::string_view& name) {
    simdjson::simdjson_result<simdjson::ondemand::value> v_res =
        obj.find_field_unordered(name);
    simdjson::ondemand::value v = v_res.value();
    simdjson::simdjson_result<simdjson::ondemand::json_type> t = v.type();
    simdjson::simdjson_result<std::string_view> raw = v.raw_json();
    return std::string(raw.value());
}

std::vector<std::string> parse_json_string_array(
    simdjson::ondemand::object& obj, const std::string_view& name) {
    std::vector<std::string> out;
    auto arr = obj.find_field_unordered(name).get_array().value();
    for (simdjson::ondemand::value el : arr) {
        std::string_view sv = el.get_string().value();
        out.emplace_back(sv);
    }
    return out;
}

//--- PROV ---
InjectorDataType get_injector_data_type(const std::string& type_string) {
    InjectorDataType result;
    if (type_string == "start") {
        result = InjectorDataType::Start;
    } else if (type_string == "end") {
        result = InjectorDataType::End;
    } else if (type_string == "exec") {
        result = InjectorDataType::Exec;
    }
    return result;
}

OperationType get_operation_type(const std::string& operation_string) {
    OperationType result;
    if (operation_string == "READ") {
        result = OperationType::Read;
    } else if (operation_string == "WRITE") {
        result = OperationType::Write;
    } else if (operation_string == "DELETE") {
        result = OperationType::Delete;
    }
    return result;
}

std::unordered_map<std::string, Operations> parse_operation_map(
    ondemand::object& json_operation_map) {
    std::unordered_map<std::string, Operations> operation_map;
    for (ondemand::field json_operation_mapping : json_operation_map) {
        std::string_view path_sv;
        (void)json_operation_mapping.unescaped_key().get(path_sv);
        std::string path(path_sv);
        ondemand::array ops_arr;
        (void)json_operation_mapping.value().get_array().get(ops_arr);
        Operations& ops = operation_map[path];
        for (simdjson::ondemand::value operation : ops_arr) {
            std::string_view operation_sv;
            (void)operation.get_string().get(operation_sv);
            std::string operation_string(operation_sv);
            if (operation_string == "read") {
                ops.read = true;
            } else if (operation_string == "write") {
                ops.write = true;
            } else if (operation_string == "deleted") {
                ops.deleted = true;
            }
        }
    }
    return operation_map;
}

ProcessMap parse_processes(ondemand::array simdjson_processes) {
    ProcessMap process_map;
    for (ondemand::value process_value : simdjson_processes) {
        ondemand::object simdjson_process = process_value.get_object().value();
        Process process;
        process.process_command =
            get_string(simdjson_process, "process_command");
        std::string process_id = get_string(simdjson_process, "process_id");
        process.env_variable_hash =
            get_uint64(simdjson_process, "env_variable_hash");
        ondemand::object simdjson_operations =
            simdjson_process["operations"].get_object().value();
        process.operation_map = parse_operation_map(simdjson_operations);
        process_map[process_id] = process;
    }
    return process_map;
}

ExecuteSetMap parse_execute_maps(ondemand::array simdjson_execute_maps) {
    ExecuteSetMap execute_set_map;
    for (ondemand::value execute_map_value : simdjson_execute_maps) {
        ondemand::object simdjson_execute_map =
            execute_map_value.get_object().value();
        std::string parent_process_id =
            get_string(simdjson_execute_map, "parent_process_id");
        std::vector<std::string> child_process_ids = parse_json_string_array(
            simdjson_execute_map, "child_process_id_array");
        std::unordered_set<std::string> child_process_ids_set(
            child_process_ids.begin(), child_process_ids.end());
        execute_set_map[parent_process_id] = child_process_ids_set;
    }
    return execute_set_map;
}

RenameMap parse_rename_map(ondemand::object& json_rename_map) {
    RenameMap rename_map;
    for (ondemand::field json_rename_mapping : json_rename_map) {
        std::string_view original_path_sv;
        (void)json_rename_mapping.unescaped_key().get(original_path_sv);
        std::string original_path(original_path_sv);
        std::string_view new_path_sv;
        (void)json_rename_mapping.value().get_string().get(new_path_sv);
        std::string new_path(new_path_sv);
        rename_map.emplace(std::move(original_path), std::move(new_path));
    }
    return rename_map;
}

EnvVariableHashPairs parse_env_variable_hash_pairs(
    ondemand::array simdjson_hash_pairs) {
    EnvVariableHashPairs env_variable_hash_pairs;
    uint64_t env_variables_hash;
    std::string env_variables;
    for (ondemand::value hash_pair : simdjson_hash_pairs) {
        ondemand::object simdjson_hash_pair = hash_pair.get_object().value();
        env_variables_hash =
            get_uint64(simdjson_hash_pair, "env_variables_hash");
        env_variables =
            get_array_json(simdjson_hash_pair, "env_variables_array");
        env_variable_hash_pairs[env_variables_hash] = env_variables;
    };
    return env_variable_hash_pairs;
}

ExecData get_exec_data(ondemand::object& payload) {
    ProcessMap process_map = parse_processes(payload["processes"].get_array());
    ExecuteSetMap execute_set_map =
        parse_execute_maps(payload["execute_map"].get_array());
    ondemand::object simdjson_rename_map =
        payload["rename_map"].get_object().value();
    RenameMap rename_map = parse_rename_map(simdjson_rename_map);
    EnvVariableHashPairs env_variable_hash_pairs =
        parse_env_variable_hash_pairs(
            payload["env_variable_hash_pair_array"].get_array());
    std::string json = get_string(payload, "json");
    std::string path = get_string(payload, "path");
    std::string command = get_string(payload, "command");
    return ExecData{.process_map = process_map,
                    .execute_set_map = execute_set_map,
                    .env_variables_hash_to_variables = env_variable_hash_pairs,
                    .json = json,
                    .path = path,
                    .command = command};
}

ParsedInjectorData parse_injector_data(const std::string& request_body) {
    ondemand::parser parser;
    padded_string padded_request_body_string(request_body);
    auto doc_res = parser.iterate(padded_request_body_string);
    auto doc = doc_res.get_object().value();
    ondemand::object header = doc["header"].get_object().value();
    ParsedInjectorData parsed_injector_data;
    std::string type = get_string(header, "type");
    InjectorDataType injector_data_type = get_injector_data_type(type);
    uint64_t job_id = get_uint64(header, "job_id");
    std::string cluster_name = get_string(header, "cluster_name");
    uint64_t timestamp = get_uint64(header, "timestamp");
    parsed_injector_data.injector_data_type = injector_data_type;
    parsed_injector_data.job_id = job_id;
    parsed_injector_data.cluster_name = cluster_name;
    parsed_injector_data.timestamp = timestamp;
    ondemand::object payload = doc["payload"].get_object().value();
    switch (parsed_injector_data.injector_data_type) {
        case InjectorDataType::End: {
            break;
        }
        case InjectorDataType::Start: {
            parsed_injector_data.payload = StartData{
                get_string(payload, "job_name"),
                get_string(payload, "username"), get_string(payload, "json"),
                get_string(payload, "path")};
            break;
        }
        case InjectorDataType::Exec: {
            parsed_injector_data.payload = get_exec_data(payload);
            break;
        }
    }
    return parsed_injector_data;
}

//--- GRAPH ---
ParsedGraphRequestData parse_graph_request_data(std::string request_body) {
    ondemand::parser parser;
    padded_string padded_request_body_string(request_body);
    auto doc_res = parser.iterate(padded_request_body_string);
    auto doc = doc_res.get_object().value();
    uint64_t job_id = get_uint64(doc, "job_id");
    std::string cluster_name = get_string(doc, "cluster_name");
    return ParsedGraphRequestData{job_id, cluster_name};
}

//--- DB_INTERFACE ---
RequestType get_request_type(const std::string& request_type_string) {
    if (request_type_string == "jobs") return RequestType::JobsQuery;
    if (request_type_string == "execs") return RequestType::ExecsQuery;
    if (request_type_string == "processes") return RequestType::ProcessesQuery;
    if (request_type_string == "files") return RequestType::FileQuery;
    return RequestType::JobsQuery;
}

static std::optional<std::string> get_optional_string(
    simdjson::ondemand::object& json_object, const char* json_key) {
    simdjson::ondemand::value json_value;
    if (json_object.find_field_unordered(json_key).get(json_value) !=
        simdjson::SUCCESS)
        return std::nullopt;
    std::string_view json_string_view;
    if (json_value.get_string().get(json_string_view) != simdjson::SUCCESS)
        return std::nullopt;
    return std::string(json_string_view);
}

static std::optional<int> get_optional_int(
    simdjson::ondemand::object& json_object, const char* json_key) {
    simdjson::ondemand::value json_value;
    if (json_object.find_field_unordered(json_key).get(json_value) !=
        simdjson::SUCCESS)
        return std::nullopt;
    int64_t json_int64 = 0;
    if (json_value.get_int64().get(json_int64) != simdjson::SUCCESS)
        return std::nullopt;
    return (int)json_int64;
}

static std::optional<uint64_t> get_optional_uint64(
    simdjson::ondemand::object& json_object, const char* json_key) {
    simdjson::ondemand::value json_value;
    if (json_object.find_field_unordered(json_key).get(json_value) !=
        simdjson::SUCCESS)
        return std::nullopt;
    uint64_t json_uint64 = 0;
    if (json_value.get_uint64().get(json_uint64) != simdjson::SUCCESS)
        return std::nullopt;
    return json_uint64;
}

static bool get_bool_default_false(simdjson::ondemand::object& json_object,
                                   const char* json_key) {
    simdjson::ondemand::value json_value;
    if (json_object.find_field_unordered(json_key).get(json_value) !=
        simdjson::SUCCESS)
        return false;
    bool json_bool = false;
    if (json_value.get_bool().get(json_bool) != simdjson::SUCCESS) return false;
    return json_bool;
}

ParsedDBInterfaceRequestData parse_db_interface_request_data(
    std::string request_body) {
    simdjson::ondemand::parser p;
    simdjson::padded_string s(request_body);
    auto docres = p.iterate(s);
    simdjson::ondemand::object root = docres.get_object().value();
    std::string request_type_string = get_string(root, "query_type");
    RequestType request_type = get_request_type(request_type_string);
    simdjson::ondemand::value payload_val = root["payload"];
    simdjson::ondemand::object payload = payload_val.get_object().value();
    ParsedDBInterfaceRequestData out;
    out.request_type = request_type;
    switch (request_type) {
        case RequestType::JobsQuery: {
            JobsQueryOpts o;
            o.user = get_string(payload, "user");
            o.before = get_optional_string(payload, "before");
            o.after = get_optional_string(payload, "after");
            out.opts = std::move(o);
            return out;
        }
        case RequestType::ExecsQuery: {
            ExecsQueryOpts o;
            o.job_id = get_uint64(payload, "job_id");
            o.cluster = get_string(payload, "cluster");
            o.list_with_processes =
                get_bool_default_false(payload, "processes");
            o.list_with_files = get_bool_default_false(payload, "files");
            out.opts = std::move(o);
            return out;
        }
        case RequestType::ProcessesQuery: {
            ProcessesQueryOpts o;
            o.exec_id = get_uint64(payload, "exec_id");
            o.list_with_files = get_bool_default_false(payload, "files");
            out.opts = std::move(o);
            return out;
        }
        case RequestType::FileQuery: {
            FileQueryOpts o;
            o.process_id = get_uint64(payload, "process_id");
            o.reads = get_bool_default_false(payload, "reads");
            o.writes = get_bool_default_false(payload, "writes");
            o.deletes = get_bool_default_false(payload, "deletes");
            out.opts = std::move(o);
            return out;
        }
    }
    throw std::runtime_error("unknown request_type");
}
