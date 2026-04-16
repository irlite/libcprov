#include <string>

#include "model.hpp"

std::string convert_job_data_to_json(JobData job_data);
std::string convert_db_interface_data_to_json(
    DBInterfaceData db_interface_data);
std::string convert_retriever_data_to_json(DBRetrieverData db_retriever_data);
