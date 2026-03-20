#include <string>

#include "model.hpp"

std::string build_jobs_query_json(const JobsQueryOpts& jobs_query_opts) {
    const std::string user = jobs_query_opts.user.value();
    const std::string before = jobs_query_opts.before.value_or("");
    const std::string after = jobs_query_opts.after.value_or("");
    return std::string(R"({"query_type":"jobs","payload":{"user":")") + user +
           R"(","before":")" + before + R"(","after":")" + after + R"("}})";
}

std::string build_execs_query_json(const ExecsQueryOpts& execs_query_opts) {
    return std::string(R"({"query_type":"execs","payload":{"job_id":)") +
           execs_query_opts.job_id + R"(,"cluster":")" +
           execs_query_opts.cluster + R"(","files":)" +
           (execs_query_opts.list_with_files ? "true" : "false") + R"(}})";
}

std::string build_processes_query_json(
    const ProcessesQueryOpts& processes_query_opts) {
    return std::string(R"({"query_type":"processes","payload":{"exec_id":)") +
           processes_query_opts.exec_id + R"(,"files":)" +
           (processes_query_opts.list_with_files ? "true" : "false") + R"(}})";
}

std::string build_files_query_json(const FileQueryOpts& file_query_opts) {
    const std::string process_id =
        file_query_opts.process_id ? std::to_string(file_query_opts.process_id)
                                   : "";
    return std::string(R"({"query_type":"files","payload":{"process_id":)") +
           process_id + R"(,"reads":)" +
           (file_query_opts.reads ? "true" : "false") + R"(,"writes":)" +
           (file_query_opts.writes ? "true" : "false") + R"(,"deletes":)" +
           (file_query_opts.deletes ? "true" : "false") + R"(}})";
}
