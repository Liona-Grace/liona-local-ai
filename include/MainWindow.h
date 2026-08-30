#pragma once

#include "OllamaConnection.h"
#include "ScreenSelector.h"
#include "TextRecognizer.h"

#include <QMainWindow>
#include <QString>

template<typename T>
class QFutureWatcher;

class QPlainTextEdit;
class QPushButton;

struct SendResult
{
    QString response;
    QString error;
};

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void sendMessage();

    OllamaConnection ollama_;
    TextRecognizer textRecognizer_;
    ScreenSelector* screenSelector_;
    QPlainTextEdit* conversationView_;
    QPlainTextEdit* messageInput_;
    QPushButton* screenSelectionButton_;
    QPushButton* sendButton_;
    QFutureWatcher<SendResult>* sendWatcher_;
};
