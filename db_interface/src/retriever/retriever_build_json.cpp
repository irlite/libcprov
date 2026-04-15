#include <cstdint>
#include <string>

#include "retriever/retriever_model.hpp"

std::string wrap_in_retriever_type(std::string retriever_json,
                                   std::string type) {
    return R"({"type":")" + type + R"(","content":)" + retriever_json + "}";
}

std::string build_job_retriever_json(uint64_t job_id, std::string cluster) {
    std::string job_id_string = std::to_string(job_id);
    std::string retriever_json =
        R"({"job_id":)" + job_id_string + R"(,"cluster":")" + cluster + R"("})";
    return wrap_in_retriever_type(retriever_json, "job");
}

std::string build_exec_retriever_json(uint64_t exec_id) {
    std::string retriever_json =
        R"({"exec_id":)" + std::to_string(exec_id) + R"("})";
    return wrap_in_retriever_type(retriever_json, "job");
}

std::string build_checksum_retriever_json(std::string checksum) {
    std::string retriever_json = R"({"checksum":)" + checksum + R"("})";
    return wrap_in_retriever_type(retriever_json, "job");
}
