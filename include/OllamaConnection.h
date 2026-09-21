#pragma once

#include <string>
#include <set>
#include <vector>

class OllamaConnection final
{
public:
    OllamaConnection();
    ~OllamaConnection();

    OllamaConnection(const OllamaConnection&) = delete;
    OllamaConnection& operator=(const OllamaConnection&) = delete;

    std::vector<std::string> listModels() const;
    std::string send(const std::string& message, const std::string& model);

private:
    void unload(const std::string& model) const noexcept;
    std::set<std::string> usedModels_;
};
