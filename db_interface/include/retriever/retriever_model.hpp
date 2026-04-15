#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

enum class RetrievalType { Job, Exec, Checksum };

struct ParsedJobRetrievalInput {
    uint64_t job_id;
    std::string cluster;
    bool end = false;
};

struct FileData {
    std::string file_path;
    std::string checksum;
};

struct ParsedRetrieverCliInput {
    RetrievalType retrieval_type;
    std::variant<ParsedJobRetrievalInput, uint64_t, FileData>
        retrieval_cli_input;
};

struct Operations {
    bool read = false;
    bool write = false;
    bool deleted = false;
    std::string start_checksum;
    std::string end_checksum;
};

using OperationMap = std::unordered_map<std::string, Operations>;

using OrderedOperationsPerExecs = std::unordered_map<uint64_t, OperationMap>;

using RetrieverBackendResponseData =
    std::variant<OrderedOperationsPerExecs, bool>;

struct ParsedRetrieverBackendResponse {
    bool success = true;
    std::optional<RetrieverBackendResponseData> retriever_backend_response_data;
    std::optional<std::string> error_message;
};
