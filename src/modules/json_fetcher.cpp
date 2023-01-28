
#include "json_fetcher.hpp"
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <memory>

int json_fetcher::curl_init_count = 0;
std::mutex json_fetcher::curl_mtx;

void curl_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
}

json_fetcher::~json_fetcher() {
    Json::Value* root = (Json::Value*)json_obj;
    root->~Value();

    curl_mtx.lock();
    curl_init_count--;
    if (curl_init_count == 0)
        curl_global_cleanup();
    curl_mtx.unlock();
}

json_fetcher::json_fetcher(char* url) {
    curl_mtx.lock();
    if (curl_init_count == 0)
        curl_global_init(CURL_GLOBAL_ALL);
    curl_init_count++;
    curl_mtx.unlock();

    std::string rawJson = "";

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(&rawJson));

        /* Perform the request, res will get the return code */
        curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    printf("%s\n", rawJson.c_str());

    const auto rawJsonLength = static_cast<int>(rawJson.length());
    JSONCPP_STRING err;
    Json::Value* root = new Json::Value();

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, root, &err)) {
        root = NULL;
    }
    json_obj = root;
}

bool json_fetcher::query(char* q_str, void* res, int* res_len) {
    return false;
}
