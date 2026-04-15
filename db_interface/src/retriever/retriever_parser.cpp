#include <simdjson.h>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "retriever/retriever_model.hpp"

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

OperationMap parse_exec_operations(ondemand::object& json_operation_map) {
    OperationMap operation_map;
    for (auto json_operation_mapping : json_operation_map) {
        std::string_view path_sv;
        json_operation_mapping.unescaped_key().get(path_sv);
        std::string path(path_sv);
        ondemand::object json_operation_mapping_object =
            json_operation_mapping.value().get_object().value();
        Operations& ops = operation_map[path];
        auto performed_ops_val =
            json_operation_mapping_object
                .find_field_unordered("performed_operations")
                .get_array()
                .value();
        for (auto op_val : performed_ops_val) {
            std::string_view op_sv;
            op_val.get_string().get(op_sv);
            std::string op_str(op_sv);
            if (op_str == "read") {
                ops.read = true;
            } else if (op_str == "write") {
                ops.write = true;
            } else if (op_str == "deleted") {
                ops.deleted = true;
            }
        }
        ops.start_checksum =
            get_string(json_operation_mapping_object, "start_checksum");
        ops.end_checksum =
            get_string(json_operation_mapping_object, "end_checksum");
    }
    return operation_map;
}

OrderedOperationsPerExecs parse_job_or_exec_retrieval_data(
    ondemand::object content_object) {
    ondemand::object exec_operations_map =
        content_object["exec_operations_map"].get_object().value();
    std::unordered_map<uint64_t, OperationMap> exec_operations_by_id;
    for (auto exec_operations_mapping : exec_operations_map) {
        std::string_view path_sv;
        exec_operations_mapping.unescaped_key().get(path_sv);
        uint64_t exec_id = std::stoull(std::string(path_sv));
        ondemand::object json_operation_mapping_object =
            exec_operations_mapping.value().get_object().value();
        exec_operations_by_id[exec_id] =
            parse_exec_operations(json_operation_mapping_object);
    }
    return exec_operations_by_id;
}

bool parse_checksum_retrieval_data(ondemand::object content_object) {
    return get_uint64(content_object, "checksum_exists") == 1;
}

ParsedRetrieverBackendResponse parse_retriever_backend_response(
    const std::string& response_body, RetrievalType retrieval_type) {
    ParsedRetrieverBackendResponse parsed_retriever_backend_response;
    ondemand::parser response_parser;
    padded_string padded_response_body_string(response_body);
    auto response_document_result =
        response_parser.iterate(padded_response_body_string);
    ondemand::object root_object =
        response_document_result.get_object().value();
    ondemand::object content_object =
        root_object["content"].get_object().value();
    uint64_t status = get_uint64(root_object, "status");
    if (status != 0) {
        parsed_retriever_backend_response.success = false;
        parsed_retriever_backend_response.error_message =
            get_string(content_object, "error_message");
        return parsed_retriever_backend_response;
    }
    RetrieverBackendResponseData retriever_backend_response_data;
    switch (retrieval_type) {
        case RetrievalType::Job: {
            retriever_backend_response_data =
                parse_job_or_exec_retrieval_data(content_object);
            break;
        }
        case RetrievalType::Exec: {
            retriever_backend_response_data =
                parse_job_or_exec_retrieval_data(content_object);
            break;
        }
        case RetrievalType::Checksum: {
            retriever_backend_response_data =
                parse_checksum_retrieval_data(content_object);
            break;
        }
    }
    parsed_retriever_backend_response.retriever_backend_response_data =
        retriever_backend_response_data;
    return parsed_retriever_backend_response;
}
