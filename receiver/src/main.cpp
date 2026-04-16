#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "config_parser.hpp"
#include "db.hpp"
#include "db_access.hpp"
#include "json_string_builders.hpp"
#include "logserver.hpp"
#include "model.hpp"
#include "parser.hpp"

void loop_request_processing(std::queue<std::string>& request_queue, DB& db,
                             std::mutex& request_mutex,
                             std::condition_variable& queue_cv) {
    std::queue<std::string> local_queue;
    while (true) {
        {
            std::unique_lock lock{request_mutex};
            queue_cv.wait(lock, [&] { return !request_queue.empty(); });
            local_queue = std::move(request_queue);
        }
        while (!local_queue.empty()) {
            // parse_injector_data(local_queue.front());
            save_db_data(db, parse_injector_data(local_queue.front()));
            local_queue.pop();
        }
    }
}

int main() {
    ConfigUtil::Config config = ConfigUtil::ConfigParser::parse_config_file();
    LogServer server(config.post_request_ip, config.post_request_port);
    DB db;
    db.build_tables();
    std::mutex request_mutex;
    std::queue<std::string> request_queue;
    std::condition_variable queue_cv;
    std::thread{[&] {
        loop_request_processing(request_queue, db, request_mutex, queue_cv);
    }}.detach();
    server.set_log_handlers(
        [&](const httplib::Request& req, httplib::Response& res) {
            {
                std::lock_guard lock{request_mutex};
                request_queue.push(req.body);
            }
            queue_cv.notify_one();
            std::cerr << "[http] POST /prov_api size=" << req.body.size()
                      << "\n";
            std::cerr << req.body << "\n";
            res.status = 200;
        },
        [&](const httplib::Request& req, httplib::Response& res) {
            std::cerr << "[http] POST /graph_api size=" << req.body.size()
                      << "\n";
            std::cerr << req.body << "\n";
            JobIdentifier parsed_graph_request_data =
                parse_graph_request_data(std::move(req.body));
            JobData job_data =
                fetch_graph_db_data(std::move(parsed_graph_request_data));
            std::string json_response_data =
                convert_job_data_to_json(std::move(job_data));
            res.set_content(json_response_data, "application/json");
        },
        [&](const httplib::Request& req, httplib::Response& res) {
            std::cerr << "[http] POST /db_interface_api size="
                      << req.body.size() << "\n";
            std::cerr << req.body << "\n";
            ParsedDBInterfaceRequestData parsed_db_interface_request_data =
                parse_db_interface_request_data(std::move(req.body));
            DBInterfaceData db_interface_data = fetch_db_interface_db_data(
                std::move(parsed_db_interface_request_data));
            std::string json_response_data =
                convert_db_interface_data_to_json(std::move(db_interface_data));
            res.set_content(json_response_data, "application/json");
        },
        [&](const httplib::Request& req, httplib::Response& res) {
            std::cerr << "[http] POST /retriever_api size=" << req.body.size()
                      << "\n";
            ParsedRetrieverData parsed_retriever_data =
                parse_retriever_data(std::move(req.body));
            DBRetrieverData db_retriever_data =
                fetch_retriever_db_data(std::move(parsed_retriever_data));
            std::string json_response_data =
                convert_retriever_data_to_json(std::move(db_retriever_data));
            res.set_content(json_response_data, "application/json");
        });
    server.run(4);
    return 0;
}
