#include <curl/curl.h>

#include <iostream>
#include <optional>
#include <string>
#include <variant>

#include "build_json.hpp"
#include "build_output_strings.hpp"
#include "cli.h"
#include "config_parser.hpp"
#include "model.hpp"
#include "parser.hpp"

Parsed parse_cli(CLI::App& app, int argc, char** argv) {
    JobsQueryOpts jobs_query_opts;
    ExecsQueryOpts execs_query_opts;
    ProcessesQueryOpts processes_query_opts;
    FileQueryOpts files_query_opts;
    auto jobs = app.add_subcommand("jobs", "Query jobs");
    jobs->add_option("-u,--user", jobs_query_opts.user, "Filter by user");
    auto before = jobs->add_option("-b,--before", jobs_query_opts.before,
                                   "Jobs before date (dd.mm.yyyy)");
    auto after = jobs->add_option("-a,--after", jobs_query_opts.after,
                                  "Jobs after date (dd.mm.yyyy)");
    std::vector<std::string> range_vals;
    auto range = jobs->add_option("-r,--range", range_vals,
                                  "Date range (dd.mm.yyyy dd.mm.yyyy)")
                     ->expected(2);
    jobs->add_flag("-f,--files", execs_query_opts.list_with_files,
                   "Include referenced files");
    before->excludes(after)->excludes(range);
    after->excludes(range);
    auto execs = app.add_subcommand("execs", "Query execs");
    execs->add_option("job_id", execs_query_opts.job_id, "Job ID")->required();
    execs->add_option("cluster", execs_query_opts.cluster, "Cluster")
        ->required();
    execs->add_flag("-f,--files", execs_query_opts.list_with_files,
                    "Include referenced files");
    auto processes = app.add_subcommand("processes", "Query processes");
    processes->add_option("exec_id", processes_query_opts.exec_id, "Exec ID")
        ->required();
    processes->add_flag("-f,--files", processes_query_opts.list_with_files,
                        "Include referenced files");
    auto files = app.add_subcommand("files", "Query files");
    files->add_option("process_id", files_query_opts.process_id, "Process ID")
        ->required();
    bool reads_flag = false, writes_flag = false, deletes_flag = false;
    files->add_flag("-r,--reads", reads_flag, "Only read operations");
    files->add_flag("-w,--writes", writes_flag, "Only write operations");
    files->add_flag("-d,--deletes", deletes_flag, "Only delete operations");
    bool print_json = false;
    jobs->add_flag("-j,--json", print_json, "Print output as JSON");
    execs->add_flag("-j,--json", print_json, "Print output as JSON");
    processes->add_flag("-j,--json", print_json, "Print output as JSON");
    files->add_flag("-j,--json", print_json, "Print output as JSON");
    app.require_subcommand(1);
    app.parse(argc, argv);
    if (*jobs && !jobs_query_opts.user) {
        if (const char* u = std::getenv("USER"); u && *u)
            jobs_query_opts.user = std::string(u);
    }
    if (*range) {
        jobs_query_opts.after = range_vals[0];
        jobs_query_opts.before = range_vals[1];
    }
    const bool any_rwd = reads_flag || writes_flag || deletes_flag;
    if (!any_rwd) {
        files_query_opts.reads = true;
        files_query_opts.writes = true;
        files_query_opts.deletes = true;
    } else {
        files_query_opts.reads = reads_flag;
        files_query_opts.writes = writes_flag;
        files_query_opts.deletes = deletes_flag;
    }
    Parsed parsed{};
    parsed.print_json = print_json;
    if (*jobs) {
        parsed.request_type = RequestType::JobsQuery;
        parsed.opts = std::move(jobs_query_opts);
        return parsed;
    }
    if (*execs) {
        parsed.request_type = RequestType::ExecsQuery;
        parsed.opts = std::move(execs_query_opts);
        return parsed;
    }
    if (*processes) {
        parsed.request_type = RequestType::ProcessesQuery;
        parsed.opts = std::move(processes_query_opts);
        return parsed;
    }
    if (*files) {
        parsed.request_type = RequestType::FileQuery;
        parsed.opts = std::move(files_query_opts);
        return parsed;
    }
    throw CLI::CallForHelp();
}

static size_t write_to_string(void* ptr, size_t size, size_t nmemb,
                              void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

std::string post_json_and_get_response(const std::string& url,
                                       const std::string& json) {
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)json.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK) return "";
    return response;
}

int main(int argc, char** argv) {
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    const std::string endpoint_url = "http://" + config.post_request_ip + ":" +
                                     std::to_string(config.post_request_port) +
                                     "/db_interface_api";
    CLI::App app{"Database interface"};
    try {
        Parsed parsed = parse_cli(app, argc, argv);
        std::string output_string;
        RequestType request_type = parsed.request_type;
        switch (request_type) {
            case RequestType::JobsQuery: {
                output_string =
                    build_jobs_query_json(std::get<JobsQueryOpts>(parsed.opts));
                break;
            }
            case RequestType::ExecsQuery: {
                output_string = build_execs_query_json(
                    std::get<ExecsQueryOpts>(parsed.opts));
                break;
            }
            case RequestType::ProcessesQuery: {
                output_string = build_processes_query_json(
                    std::get<ProcessesQueryOpts>(parsed.opts));
                break;
            }
            case RequestType::FileQuery: {
                output_string = build_files_query_json(
                    std::get<FileQueryOpts>(parsed.opts));
                break;
            }
        }
        std::string json_response =
            post_json_and_get_response(endpoint_url, output_string);
        ParsedQuery parsed_query =
            parse_db_interface_query_response(json_response, request_type);
        if (parsed.print_json) {
            std::cout << json_response << "\n";
        } else {
            std::cout << get_output_string(parsed_query, request_type);
        }
        return 0;
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }
}
