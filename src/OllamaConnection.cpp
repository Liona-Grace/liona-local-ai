#include "OllamaConnection.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace
{
size_t writeCallback(char* data, size_t size, size_t count, void* userData)
{
    auto* response = static_cast<std::string*>(userData);
    response->append(data, size * count);
    return size * count;
}
}

OllamaConnection::OllamaConnection()
{
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        throw std::runtime_error("Cannot initialize curl");
    }
}

OllamaConnection::~OllamaConnection()
{
    for (const auto& model : usedModels_) {
        unload(model);
    }
    curl_global_cleanup();
}

std::vector<std::string> OllamaConnection::listModels() const
{
    using json = nlohmann::json;
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Cannot create curl request");
    }

    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/run/ollama-api/ollama.sock");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/api/tags");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    const CURLcode result = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(std::string("Ollama connection failed: ") + curl_easy_strerror(result));
    }
    try {
        const json resultJson = json::parse(response);
        if (statusCode < 200 || statusCode >= 300) {
            throw std::runtime_error(resultJson.value("error", "Ollama returned an error"));
        }
        const auto& entries = resultJson.at("models");
        if (!entries.is_array()) {
            throw std::runtime_error("Invalid model list from Ollama");
        }
        std::vector<std::string> models;
        for (const auto& entry : entries) {
            auto name = entry.at("name").get<std::string>();
            if (!name.empty()) {
                models.push_back(name);
            }
        }
        return models;
    }
    catch (const json::exception& error) {
        throw std::runtime_error(std::string("Invalid response from Ollama: ") + error.what());
    }
}

std::string OllamaConnection::send(const std::string& message, const std::string& model)
{
    using json = nlohmann::json;

    if (model.empty()) {
        throw std::runtime_error("No Ollama model selected");
    }
    usedModels_.insert(model);

    const json request = {
        {"model", model},
        {"messages", {{{"role", "user"}, {"content", message}}}},
        {"stream", false},
        {"keep_alive", "2m"}
    };
    const std::string requestBody = request.dump();
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Cannot create curl request");
    }

    curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/run/ollama-api/ollama.sock");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/api/chat");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestBody.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    const CURLcode result = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(std::string("Ollama connection failed: ") + curl_easy_strerror(result));
    }

    try {
        const json resultJson = json::parse(response);
        if (statusCode < 200 || statusCode >= 300) {
            throw std::runtime_error(resultJson.value("error", "Ollama returned an error"));
        }
        return resultJson.at("message").at("content").get<std::string>();
    }
    catch (const json::exception& error) {
        throw std::runtime_error(std::string("Invalid response from Ollama: ") + error.what());
    }
}

void OllamaConnection::unload(const std::string& model) const noexcept
{
    using json = nlohmann::json;

    const std::string requestBody = json{
        {"model", model},
        {"prompt", ""},
        {"keep_alive", 0}
    }.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        return;
    }

    curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/run/ollama-api/ollama.sock");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/api/generate");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestBody.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
