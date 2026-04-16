#include "logserver.hpp"

#include <string>

LogServer::LogServer(std::string url, int port) : url_(url), port_(port) {}

void LogServer::set_log_handlers(Handler prov_api_handler,
                                 Handler graph_api_handler,
                                 Handler db_interface_api_handler,
                                 Handler retriever_api_handler) {
    prov_api_handler_ = prov_api_handler;
    graph_api_handler_ = graph_api_handler;
    db_interface_api_handler_ = db_interface_api_handler;
    retriever_api_handler_ = retriever_api_handler;
    svr_.Post("/prov_api",
              [this](const httplib::Request& req, httplib::Response& res) {
                  prov_api_handler_(req, res);
              });
    svr_.Post("/graph_api",
              [this](const httplib::Request& req, httplib::Response& res) {
                  graph_api_handler_(req, res);
              });
    svr_.Post("/db_interface_api",
              [this](const httplib::Request& req, httplib::Response& res) {
                  db_interface_api_handler_(req, res);
              });

    svr_.Post("/retriever_api",
              [this](const httplib::Request& req, httplib::Response& res) {
                  retriever_api_handler_(req, res);
              });
}

void LogServer::run(int num_threads) { svr_.listen(url_, port_, num_threads); }
