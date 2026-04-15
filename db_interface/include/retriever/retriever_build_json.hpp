#include <cstdint>
#include <string>

std::string build_job_retriever_json(uint64_t job_id, std::string cluster);

std::string build_exec_retriever_json(uint64_t exec_id);

std::string build_checksum_retriever_json(std::string checksum);
