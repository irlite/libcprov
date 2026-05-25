#include <cstdint>
#include <iostream>
#include <string>

#include "common/common.hpp"
#include "retriever/retriever_build_json.hpp"
#include "retriever/retriever_model.hpp"
#include "retriever/retriever_parser.hpp"
#include "retriever/retriever_retrieve_data.hpp"

void start_retriever(ParsedRetrieverCliInput parsed_retriever_cli_input,
                     std::string endpoint_url_base) {
    std::string output_string;
    RetrievalType retrieval_type = parsed_retriever_cli_input.retrieval_type;
    switch (retrieval_type) {
        case RetrievalType::Job: {
            ParsedJobRetrievalInput parsed_job_retrieval_input =
                std::get<ParsedJobRetrievalInput>(
                    parsed_retriever_cli_input.retrieval_cli_input);
            output_string =
                build_job_retriever_json(parsed_job_retrieval_input.job_id,
                                         parsed_job_retrieval_input.cluster);
            break;
        }
        case RetrievalType::Exec: {
            output_string = build_exec_retriever_json(std::get<uint64_t>(
                parsed_retriever_cli_input.retrieval_cli_input));
            break;
        }
        case RetrievalType::Checksum: {
            output_string = build_checksum_retriever_json(
                std::get<FileData>(
                    parsed_retriever_cli_input.retrieval_cli_input)
                    .file_path);
            break;
        }
    }
    std::string endpoint_url = endpoint_url_base + "/retriever_api";
    std::string json_response =
        post_json_and_get_response(endpoint_url, output_string);
    std::cout << json_response << "\n";
    ParsedRetrieverBackendResponse parsed_retriever_backend_response =
        parse_retriever_backend_response(std::move(json_response),
                                         retrieval_type);
    if (!parsed_retriever_backend_response.success) {
        std::cout << parsed_retriever_backend_response.error_message.value()
                  << "\n";
        return;
    }
    RetrieverBackendResponseData retrieved_provenance_data =
        parsed_retriever_backend_response.retriever_backend_response_data
            .value();
    switch (retrieval_type) {
        case RetrievalType::Job: {
            ParsedJobRetrievalInput parsed_job_retrieval_input =
                std::get<ParsedJobRetrievalInput>(
                    parsed_retriever_cli_input.retrieval_cli_input);
            int exec_id = parsed_job_retrieval_input.end ? -1 : 0;
            retrieve_exec_state(
                std::get<OrderedOperationsPerExecs>(retrieved_provenance_data),
                exec_id);
            break;
        }
        case RetrievalType::Exec: {
            retrieve_exec_state(
                std::get<OrderedOperationsPerExecs>(retrieved_provenance_data),
                std::get<uint64_t>(
                    parsed_retriever_cli_input.retrieval_cli_input));
            break;
        }
        case RetrievalType::Checksum: {
            if (std::get<bool>(retrieved_provenance_data)) {
                retrieve_single_file(std::move(std::get<FileData>(
                    parsed_retriever_cli_input.retrieval_cli_input)));
            } else {
                std::cout << "Error: No file with the provided Checksum exists."
                          << "\n";
            }
            break;
        }
    }
}
