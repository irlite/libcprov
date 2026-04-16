#pragma once

#include <string>

#include "model.hpp"

ParsedInjectorData parse_injector_data(const std::string&);
JobIdentifier parse_graph_request_data(std::string request_body);
ParsedDBInterfaceRequestData parse_db_interface_request_data(
    std::string request_body);
ParsedRetrieverData parse_retriever_data(std::string request_body);
