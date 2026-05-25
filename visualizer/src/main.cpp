#include <curl/curl.h>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "config_parser.hpp"
#include "model.hpp"
#include "parser.hpp"
#include "svg_graph.hpp"

bool is_number(const std::string& s) {
    for (unsigned char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

static size_t append_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string http_post(const std::string& url, const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string out;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcprov_visualizer/1.0");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return out;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <job id> <cluster name>\n";
        return EXIT_FAILURE;
    }
    std::string job_id = argv[1];
    std::string cluster_name = argv[2];
    if (!is_number(job_id)) {
        std::cerr << "Error: job_id must be a positive number, got: " << argv[1]
                  << "\n";
        return 1;
    }
    std::string payload = R"({"job_id":)" + job_id + R"(,"cluster_name":")" +
                          cluster_name + R"("})";
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    std::string url = "http://" + config.post_request_ip + ":" +
                      std::to_string(config.post_request_port) + "/graph_api";
    std::string body = http_post(url, payload);
    ParsedLibcprovData parsed_libcprov_data = parse_injector_data(body);
    if (parsed_libcprov_data.response_type == ResponseType::ProvData) {
        build_graph(parsed_libcprov_data.job_data.value(), job_id,
                    cluster_name);
        std::cout << "Parsed data: " << body << std::endl;
    } else if (parsed_libcprov_data.response_type == ResponseType::Error) {
        std::cout << "Error" << std::endl;
    }
    return 0;
}
