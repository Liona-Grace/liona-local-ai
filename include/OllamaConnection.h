#pragma once

#include <string>

class OllamaConnection final
{
public:
    OllamaConnection();
    ~OllamaConnection();

    OllamaConnection(const OllamaConnection&) = delete;
    OllamaConnection& operator=(const OllamaConnection&) = delete;

    std::string send(const std::string& message) const;

private:
    void unload() const noexcept;
};
