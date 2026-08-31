#include "TextRecognizer.h"

#include <QImage>
#include <QStringList>

#include <tesseract/baseapi.h>

#include <algorithm>
#include <array>
#include <cstdint>
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

    api_->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
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

    const QImage grayscaleImage = toGrayscale(image);
    const QImage upscaledImage = upscale(grayscaleImage, 3);
    const QImage contrastedImage = increaseContrast(upscaledImage, 1.8);
    const QImage binaryImage = binarize(contrastedImage);

    api_->SetImage(
        binaryImage.constBits(),
        binaryImage.width(),
        binaryImage.height(),
        1,
        binaryImage.bytesPerLine()
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

QImage TextRecognizer::toGrayscale(const QImage& image)
{
    return image.convertToFormat(QImage::Format_Grayscale8);
}

QImage TextRecognizer::upscale(const QImage& image, int factor)
{
    if (factor <= 1) {
        return image;
    }

    return image.scaled(
        image.width() * factor,
        image.height() * factor,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
    );
}

QImage TextRecognizer::increaseContrast(const QImage& image, double factor)
{
    QImage contrastedImage = image.convertToFormat(QImage::Format_Grayscale8);

    for (int y = 0; y < contrastedImage.height(); ++y) {
        uchar* scanLine = contrastedImage.scanLine(y);
        for (int x = 0; x < contrastedImage.width(); ++x) {
            const double adjusted = (scanLine[x] - 128.0) * factor + 128.0;
            scanLine[x] = static_cast<uchar>(std::clamp(adjusted, 0.0, 255.0));
        }
    }

    return contrastedImage;
}

QImage TextRecognizer::binarize(const QImage& image)
{
    const QImage grayscaleImage = image.convertToFormat(QImage::Format_Grayscale8);
    std::array<std::uint64_t, 256> histogram{};

    for (int y = 0; y < grayscaleImage.height(); ++y) {
        const uchar* scanLine = grayscaleImage.constScanLine(y);
        for (int x = 0; x < grayscaleImage.width(); ++x) {
            ++histogram[scanLine[x]];
        }
    }

    const std::uint64_t pixelCount = static_cast<std::uint64_t>(
        grayscaleImage.width()
    ) * static_cast<std::uint64_t>(grayscaleImage.height());

    double totalIntensity = 0.0;
    for (std::size_t intensity = 0; intensity < histogram.size(); ++intensity) {
        totalIntensity += intensity * histogram[intensity];
    }

    std::uint64_t backgroundCount = 0;
    double backgroundIntensity = 0.0;
    double highestVariance = -1.0;
    int threshold = 127;

    for (int intensity = 0; intensity < 256; ++intensity) {
        backgroundCount += histogram[intensity];
        if (backgroundCount == 0) {
            continue;
        }

        const std::uint64_t foregroundCount = pixelCount - backgroundCount;
        if (foregroundCount == 0) {
            break;
        }

        backgroundIntensity += intensity * histogram[intensity];
        const double backgroundMean = backgroundIntensity / backgroundCount;
        const double foregroundMean =
            (totalIntensity - backgroundIntensity) / foregroundCount;
        const double meanDifference = backgroundMean - foregroundMean;
        const double variance = static_cast<double>(backgroundCount)
            * static_cast<double>(foregroundCount)
            * meanDifference * meanDifference;

        if (variance > highestVariance) {
            highestVariance = variance;
            threshold = intensity;
        }
    }

    QImage binaryImage(
        grayscaleImage.size(),
        QImage::Format_Grayscale8
    );
    std::uint64_t whitePixelCount = 0;

    for (int y = 0; y < grayscaleImage.height(); ++y) {
        const uchar* sourceLine = grayscaleImage.constScanLine(y);
        uchar* targetLine = binaryImage.scanLine(y);
        for (int x = 0; x < grayscaleImage.width(); ++x) {
            targetLine[x] = sourceLine[x] > threshold ? 255 : 0;
            whitePixelCount += targetLine[x] == 255;
        }
    }

    if (whitePixelCount < pixelCount / 2) {
        binaryImage.invertPixels();
    }

    return binaryImage;
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
