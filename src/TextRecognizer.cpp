#include "TextRecognizer.h"

#include <QImage>
#include <QStringList>

#include <tesseract/baseapi.h>

#include <memory>
#include <stdexcept>

TextRecognizer::TextRecognizer(const std::string& languages)
    : api_(std::make_unique<tesseract::TessBaseAPI>())
{
    if (api_->Init(nullptr, languages.c_str()) != 0) {
        throw std::runtime_error(
            "Cannot initialize Tesseract for languages: " + languages
        );
    }
}

TextRecognizer::~TextRecognizer()
{
    api_->End();
}

QString TextRecognizer::recognize(const QImage& image)
{
    if (image.isNull()) {
        throw std::invalid_argument("Cannot recognize text from an empty image");
    }

    const QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    api_->SetImage(
        rgbImage.constBits(),
        rgbImage.width(),
        rgbImage.height(),
        3,
        rgbImage.bytesPerLine()
    );

    if (api_->Recognize(nullptr) != 0) {
        throw std::runtime_error("Tesseract failed to recognize the image");
    }

    std::unique_ptr<char[]> recognizedText(api_->GetUTF8Text());
    if (!recognizedText) {
        throw std::runtime_error("Tesseract returned no recognition result");
    }

    return normalizeText(QString::fromUtf8(recognizedText.get()));
}

QString TextRecognizer::normalizeText(QString text)
{
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');

    QStringList paragraphs;
    QStringList currentParagraph;

    const QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        const QString normalizedLine = line.simplified();
        if (!normalizedLine.isEmpty()) {
            currentParagraph.append(normalizedLine);
            continue;
        }

        if (!currentParagraph.isEmpty()) {
            paragraphs.append(currentParagraph.join(' '));
            currentParagraph.clear();
        }
    }

    if (!currentParagraph.isEmpty()) {
        paragraphs.append(currentParagraph.join(' '));
    }

    return paragraphs.join("\n\n");
}
