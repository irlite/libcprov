#pragma once
#include <string>

#include "querier/querier_model.hpp"

std::string build_jobs_query_json(const JobsQueryOpts& jobs_query_opts);
std::string build_execs_query_json(const ExecsQueryOpts& execs_query_opts);
std::string build_processes_query_json(
    const ProcessesQueryOpts& processes_query_opts);
std::string build_files_query_json(const FileQueryOpts& file_query_opts);
