
#ifndef _JSON_FETCHER_H_
#define _JSON_FETCHER_H_

#include <mutex>

class json_fetcher {
private:
    void* json_obj;

    static int curl_init_count;
    static std::mutex curl_mtx;

public:
    json_fetcher(char* url);
    ~json_fetcher();

    bool query(char* q_str, void* res, int* res_len);
};

#endif
