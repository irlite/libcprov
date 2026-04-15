#include <curl/curl.h>

#include <string>

#include "common/cli.h"
#include "common/model.hpp"
#include "config_parser.hpp"
#include "querier/querier.hpp"
#include "querier/querier_model.hpp"
#include "retriever/retriever.hpp"
#include "retriever/retriever_model.hpp"

void add_query_subcommand(CLI::App& app, ParsedCliInput& parsed_cli_input,
                          QueryCliState& s) {
    auto query = app.add_subcommand("query", "Query data");
    auto jobs = query->add_subcommand("jobs", "Query jobs");
    jobs->add_option("-u,--user", s.jobs_query_opts.user, "Filter by user");
    auto before = jobs->add_option("-b,--before", s.jobs_query_opts.before,
                                   "Jobs before date (dd.mm.yyyy)");
    auto after = jobs->add_option("-a,--after", s.jobs_query_opts.after,
                                  "Jobs after date (dd.mm.yyyy)");
    auto range = jobs->add_option("-r,--range", s.range_vals,
                                  "Date range (dd.mm.yyyy dd.mm.yyyy)")
                     ->expected(2);
    jobs->add_flag("-f,--files", s.jobs_query_opts.list_with_files,
                   "Include referenced files");
    before->excludes(after)->excludes(range);
    after->excludes(range);
    jobs->add_flag("-j,--json", s.print_json, "Print output as JSON");
    auto execs = query->add_subcommand("execs", "Query execs");
    execs->add_option("job_id", s.execs_query_opts.job_id)->required();
    execs->add_option("cluster", s.execs_query_opts.cluster)->required();
    execs->add_flag("-f,--files", s.execs_query_opts.list_with_files);
    execs->add_flag("-j,--json", s.print_json);
    auto processes = query->add_subcommand("processes", "Query processes");
    processes->add_option("exec_id", s.processes_query_opts.exec_id)
        ->required();
    processes->add_flag("-f,--files", s.processes_query_opts.list_with_files);
    processes->add_flag("-j,--json", s.print_json);
    auto files = query->add_subcommand("files", "Query files");
    files->add_option("process_id", s.files_query_opts.process_id)->required();
    files->add_flag("-r,--reads", s.reads_flag);
    files->add_flag("-w,--writes", s.writes_flag);
    files->add_flag("-d,--deletes", s.deletes_flag);
    files->add_flag("-j,--json", s.print_json);
    query->require_subcommand(1);
    query->callback([&s, &parsed_cli_input, jobs, execs, processes, files,
                     range]() {
        if (*jobs && !s.jobs_query_opts.user) {
            if (const char* u = std::getenv("USER"); u && *u)
                s.jobs_query_opts.user = std::string(u);
        }
        if (*range) {
            s.jobs_query_opts.after = s.range_vals[0];
            s.jobs_query_opts.before = s.range_vals[1];
        }
        const bool any_rwd = s.reads_flag || s.writes_flag || s.deletes_flag;
        if (!any_rwd) {
            s.files_query_opts.reads = true;
            s.files_query_opts.writes = true;
            s.files_query_opts.deletes = true;
        } else {
            s.files_query_opts.reads = s.reads_flag;
            s.files_query_opts.writes = s.writes_flag;
            s.files_query_opts.deletes = s.deletes_flag;
        }
        Parsed parsed;
        parsed.print_json = s.print_json;
        if (*jobs) {
            parsed.request_type = RequestType::JobsQuery;
            parsed.opts = s.jobs_query_opts;
        } else if (*execs) {
            parsed.request_type = RequestType::ExecsQuery;
            parsed.opts = s.execs_query_opts;
        } else if (*processes) {
            parsed.request_type = RequestType::ProcessesQuery;
            parsed.opts = s.processes_query_opts;
        } else if (*files) {
            parsed.request_type = RequestType::FileQuery;
            parsed.opts = s.files_query_opts;
        }
        parsed_cli_input.program_type = ProgramType::Querier;
        parsed_cli_input.parsed_program_input = std::move(parsed);
    });
}

void add_retrieval_subcommand(CLI::App& app, ParsedCliInput& parsed_cli_input,
                              RetrievalCliState& s) {
    auto retrieval = app.add_subcommand("retrieval", "Retrieve data");
    auto job_cmd = retrieval->add_subcommand("job", "Retrieve job");
    job_cmd->add_option("job_id", s.job_input.job_id, "Job ID")->required();
    job_cmd->add_option("cluster", s.job_input.cluster, "Cluster")->required();
    job_cmd->add_flag("-e,--end", s.job_input.end, "End flag");
    auto exec_cmd = retrieval->add_subcommand("exec", "Retrieve exec");
    exec_cmd->add_option("exec_id", s.exec_id, "Exec ID")->required();
    auto checksum_cmd =
        retrieval->add_subcommand("checksum", "Retrieve by checksum");
    checksum_cmd->add_option("checksum", s.checksum, "Checksum")->required();
    checksum_cmd->add_option("path", s.file_path, "File Path")->required();
    retrieval->require_subcommand(1);
    retrieval->callback([&s, &parsed_cli_input, job_cmd, exec_cmd,
                         checksum_cmd]() {
        ParsedRetrieverCliInput parsed_retriever_cli_input;
        if (*job_cmd) {
            parsed_retriever_cli_input.retrieval_type = RetrievalType::Job;
            parsed_retriever_cli_input.retrieval_cli_input = s.job_input;
        } else if (*exec_cmd) {
            parsed_retriever_cli_input.retrieval_type = RetrievalType::Exec;
            parsed_retriever_cli_input.retrieval_cli_input = s.exec_id;
        } else if (*checksum_cmd) {
            parsed_retriever_cli_input.retrieval_type = RetrievalType::Checksum;
            parsed_retriever_cli_input.retrieval_cli_input =
                FileData{s.file_path, s.checksum};
        }
        parsed_cli_input.program_type = ProgramType::Retriever;
        parsed_cli_input.parsed_program_input =
            std::move(parsed_retriever_cli_input);
    });
}

int main(int argc, char** argv) {
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    const std::string endpoint_url_base =
        "http://" + config.post_request_ip + ":" +
        std::to_string(config.post_request_port);
    CLI::App app{"Prov Access"};
    app.require_subcommand(1);
    ParsedCliInput parsed_cli_input;
    QueryCliState query_state;
    RetrievalCliState retrieval_state;
    add_query_subcommand(app, parsed_cli_input, query_state);
    add_retrieval_subcommand(app, parsed_cli_input, retrieval_state);
    CLI11_PARSE(app, argc, argv);
    switch (parsed_cli_input.program_type) {
        case ProgramType::Visualizer:
            break;
        case ProgramType::Querier:
            start_querier(std::move(std::get<Parsed>(
                              parsed_cli_input.parsed_program_input)),
                          endpoint_url_base);
            break;
        case ProgramType::Retriever:
            start_retriever(std::move(std::get<ParsedRetrieverCliInput>(
                                parsed_cli_input.parsed_program_input)),
                            endpoint_url_base);
            break;
    }
    return 0;
}
