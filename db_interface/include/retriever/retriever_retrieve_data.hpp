#include <cstdint>

#include "retriever/retriever_model.hpp"

void retrieve_exec_state(OrderedOperationsPerExecs ordered_operations_per_execs,
                         uint64_t exec_id);

void retrieve_single_file(FileData file_data);
