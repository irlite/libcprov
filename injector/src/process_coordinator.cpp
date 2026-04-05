#define _GNU_SOURCE

#include <curl/curl.h>
#include <fcntl.h>
#include <simdjson.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <string>

void set_env_variables(const std::string& path_exec,
                       const std::string& path_access) {
    setenv("PROV_PATH_WRITE", path_access.c_str(), 1);
}

void start_preload_process(const std::string& so_path, const std::string& cmd,
                           const std::string& path_access) {
    std::filesystem::create_directory(path_access);
    pid_t pid = fork();
    if (pid == 0) {
        if (!so_path.empty()) setenv("LD_PRELOAD", so_path.c_str(), 1);
        std::cout << so_path << std::endl;
        std::cout << cmd << std::endl;
        std::cout << path_access << std::endl;
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
        _exit(127);
    } else if (pid > 0) {
        (void)waitpid(pid, nullptr, 0);
    }
}

void send_json(const std::string& url, const std::string& header,
               const std::string& json) {
    CURL* curl = curl_easy_init();
    if (!curl) return;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string payload = header + json;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[](void*, size_t s, size_t n, void*) { return s * n; });
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
