#ifndef LOGSERVER_H
#define LOGSERVER_H

#include <functional>
#include <string>

#include "httplib.hpp"

class LogServer {
   public:
    using Handler =
        std::function<void(const httplib::Request&, httplib::Response&)>;
    LogServer(std::string url, int port);
    void set_log_handlers(Handler prov_api_handler, Handler graph_api_handler,
                          Handler db_interface_api_handler,
                          Handler retriever_api_handler);
    void run(int num_threads);

   private:
    httplib::Server svr_;
    std::string url_;
    Handler prov_api_handler_;
    Handler graph_api_handler_;
    Handler db_interface_api_handler_;
    Handler retriever_api_handler_;
    int port_;
};

#endif
