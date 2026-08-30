#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "MainApplication.h"

#include <iostream>
#include <string>

using json = nlohmann::json;

static size_t writeCallback(
    char* ptr,
    size_t size,
    size_t nmemb,
    void* userdata)
{
    auto* response = static_cast<std::string*>(userdata);
    response->append(ptr, size * nmemb);
    return size * nmemb;
}

int main(int argc, char** argv)
{
    MainApplication app(argc, argv);
    return app.exec();
}
