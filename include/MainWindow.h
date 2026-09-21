#pragma once

#include "OllamaConnection.h"
#include "ScreenSelector.h"
#include "TextRecognizer.h"

#include <QMainWindow>
#include <QString>
#include <QStringList>

template<typename T>
class QFutureWatcher;

class QComboBox;
class QPlainTextEdit;
class QPushButton;

struct SendResult
{
    QString response;
    QString error;
};

struct ModelListResult
{
    QStringList models;
    QString error;
};

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void restoreAfterSelection();
    void sendMessage(const QString& targetLanguage = {});

    OllamaConnection ollama_;
    QComboBox* modelSelector_;
    QFutureWatcher<ModelListResult>* modelsWatcher_;
    TextRecognizer textRecognizer_;
    ScreenSelector* screenSelector_;
    QPlainTextEdit* conversationView_;
    QPlainTextEdit* messageInput_;
    QPushButton* screenSelectionButton_;
    QPushButton* sendButton_;
    QPushButton* translateVietnameseButton_;
    QPushButton* translateEnglishButton_;
    QFutureWatcher<SendResult>* sendWatcher_;
};
