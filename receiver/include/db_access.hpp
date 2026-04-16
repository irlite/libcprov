#include <optional>
#include <string>

#include "db.hpp"
#include "model.hpp"

void save_db_data(DB& db, const ParsedInjectorData& parsed_injector_data);
// std::string fetch_db_data(const std::string& request_body);
JobData fetch_graph_db_data(JobIdentifier parsed_graph_request_data);
DBInterfaceData fetch_db_interface_db_data(
    ParsedDBInterfaceRequestData parsed_db_interface_request_data);
DBRetrieverData fetch_retriever_db_data(
    ParsedRetrieverData parsed_retriever_data);
