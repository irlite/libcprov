#include <string>

#include "retriever_model.hpp"

ParsedRetrieverBackendResponse parse_retriever_backend_response(
    const std::string& response_body, RetrievalType retrieval_type);
