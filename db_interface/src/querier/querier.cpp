#include <iostream>
#include <string>

#include "common/common.hpp"
#include "querier/querier_build_json.hpp"
#include "querier/querier_build_output_strings.hpp"
#include "querier/querier_model.hpp"
#include "querier/querier_parser.hpp"

void start_querier(Parsed parsed, std::string endpoint_url_base) {
    std::string output_string;
    RequestType request_type = parsed.request_type;
    switch (request_type) {
        case RequestType::JobsQuery: {
            output_string =
                build_jobs_query_json(std::get<JobsQueryOpts>(parsed.opts));
            break;
        }
        case RequestType::ExecsQuery: {
            output_string =
                build_execs_query_json(std::get<ExecsQueryOpts>(parsed.opts));
            break;
        }
        case RequestType::ProcessesQuery: {
            output_string = build_processes_query_json(
                std::get<ProcessesQueryOpts>(parsed.opts));
            break;
        }
        case RequestType::FileQuery: {
            output_string =
                build_files_query_json(std::get<FileQueryOpts>(parsed.opts));
            break;
        }
    }
    std::string endpoint_url = endpoint_url_base + "/db_interface_api";
    std::string json_response =
        post_json_and_get_response(endpoint_url, output_string);
    if (parsed.print_json) {
        std::cout << json_response << "\n";
    } else {
        ParsedQuery parsed_query =
            parse_db_interface_query_response(json_response, request_type);
        std::cout << get_output_string(parsed_query, request_type);
    }
}
