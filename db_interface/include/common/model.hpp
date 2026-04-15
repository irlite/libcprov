#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "querier/querier_model.hpp"
#include "retriever/retriever_model.hpp"

enum class ProgramType { Visualizer, Querier, Retriever };

struct ParsedVisualizerCliInput {};

struct ParsedCliInput {
    ProgramType program_type{};
    std::variant<ParsedVisualizerCliInput, Parsed, ParsedRetrieverCliInput>
        parsed_program_input;
};

struct QueryCliState {
    JobsQueryOpts jobs_query_opts;
    ExecsQueryOpts execs_query_opts;
    ProcessesQueryOpts processes_query_opts;
    FileQueryOpts files_query_opts{};
    bool print_json = false;
    std::vector<std::string> range_vals;
    bool reads_flag = false;
    bool writes_flag = false;
    bool deletes_flag = false;
};

struct RetrievalCliState {
    ParsedJobRetrievalInput job_input{};
    uint64_t exec_id = 0;
    std::string checksum;
    std::string file_path;
};
