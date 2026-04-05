#pragma once

#include <simdjson.h>

#include <string>

namespace ConfigUtil {
struct Config {
    std::string injector_path;
    std::string prov_artifacts_path;
    std::string post_request_ip;
    uint16_t post_request_port;
};
class ConfigParser {
   public:
    static Config parse_config_file();

   private:
    static std::string get_config_path_relative_to_source(
        const std::string& filename);
    static std::string get_string_field(simdjson::ondemand::object& obj,
                                        const char* name);
    static uint16_t get_uint16_field(simdjson::ondemand::object& obj,
                                     const char* name);
};
}  // namespace ConfigUtil
