#include <curl/curl.h>

#include <string>

static size_t write_to_string(void* ptr, size_t size, size_t nmemb,
                              void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

std::string post_json_and_get_response(const std::string& url,
                                       const std::string& json) {
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)json.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK) return "";
    return response;
}
