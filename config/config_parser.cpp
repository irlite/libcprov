#include "config_parser.hpp"

#include <simdjson.h>

#include <filesystem>

namespace ConfigUtil {

std::string ConfigParser::get_config_path_relative_to_source(
    const std::string& filename) {
    return std::filesystem::path(__FILE__).parent_path() / filename;
}

std::string ConfigParser::get_string_field(simdjson::ondemand::object& obj,
                                           const char* name) {
    return std::string(obj.find_field_unordered(name).get_string().value());
}

uint16_t ConfigParser::get_uint16_field(simdjson::ondemand::object& obj,
                                        const char* name) {
    return static_cast<uint16_t>(
        obj.find_field_unordered(name).get_uint64().value());
}

Config ConfigParser::parse_config_file() {
    std::string path = get_config_path_relative_to_source("config.json");
    simdjson::ondemand::parser parser;
    simdjson::padded_string padded_json =
        simdjson::padded_string::load(path).value();
    simdjson::ondemand::document document = parser.iterate(padded_json).value();
    simdjson::ondemand::object root_object = document.get_object().value();
    Config parsed_config;
    parsed_config.injector_path =
        get_string_field(root_object, "injector_path");
    parsed_config.prov_artifacts_path =
        get_string_field(root_object, "prov_artifacts_path");
    parsed_config.post_request_ip =
        get_string_field(root_object, "post_request_ip");
    parsed_config.post_request_port =
        get_uint16_field(root_object, "post_request_port");
    return parsed_config;
}
}  // namespace ConfigUtil
