#pragma once

#include <QString>

#include <memory>
#include <string>

class QImage;

namespace tesseract
{
class TessBaseAPI;
}

class TextRecognizer final
{
public:
    explicit TextRecognizer(const std::string& languages = "eng+vie");
    ~TextRecognizer();

    TextRecognizer(const TextRecognizer&) = delete;
    TextRecognizer& operator=(const TextRecognizer&) = delete;

    QString recognize(const QImage& image);

private:
    std::unique_ptr<tesseract::TessBaseAPI> api_;
};
