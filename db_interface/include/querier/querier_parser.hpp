#pragma once

#include <querier/querier_model.hpp>
#include <string>

ParsedQuery parse_db_interface_query_response(const std::string& response_body,
                                              RequestType request_type);
