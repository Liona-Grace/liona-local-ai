#include "MainWindow.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QFutureWatcher>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <exception>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , screenSelector_(new ScreenSelector())
    , conversationView_(new QPlainTextEdit(this))
    , messageInput_(new QPlainTextEdit(this))
    , screenSelectionButton_(new QPushButton("Screen Selection", this))
    , sendButton_(new QPushButton("Send", this))
    , sendWatcher_(new QFutureWatcher<SendResult>(this))
{
    setWindowTitle("Liona Local AI");
    resize(800, 600);

    conversationView_->setReadOnly(true);
    conversationView_->setPlaceholderText("Conversation will appear here...");
    messageInput_->setPlaceholderText("Type a message...");
    messageInput_->installEventFilter(this);
    screenSelectionButton_->setMinimumWidth(130);
    sendButton_->setMinimumWidth(100);

    auto* inputArea = new QWidget(this);
    auto* inputLayout = new QHBoxLayout(inputArea);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(8);
    inputLayout->addWidget(messageInput_, 1);
    inputLayout->addWidget(screenSelectionButton_, 0, Qt::AlignBottom);
    inputLayout->addWidget(sendButton_, 0, Qt::AlignBottom);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(conversationView_, 7);
    mainLayout->addWidget(inputArea, 3);

    setCentralWidget(centralWidget);

    connect(sendButton_, &QPushButton::clicked, this, [this] {
        sendMessage();
    });

    connect(screenSelectionButton_, &QPushButton::clicked, this, [this] {
        showMinimized();
        QTimer::singleShot(250, this, [this] {
            screenSelector_->startSelection();
        });
    });

    connect(
        screenSelector_,
        &ScreenSelector::selectionFinished,
        this,
        [this](const QImage& image) {
            restoreAfterSelection();

            QTimer::singleShot(0, this, [this, image] {
                try {
                    messageInput_->setPlainText(textRecognizer_.recognize(image));
                    messageInput_->setFocus();
                }
                catch (const std::exception& error) {
                    QMessageBox::warning(
                        this,
                        "Text recognition failed",
                        QString::fromUtf8(error.what())
                    );
                }
            });
        }
    );

    connect(
        screenSelector_,
        &ScreenSelector::selectionCanceled,
        this,
        [this] {
            restoreAfterSelection();
        }
    );

    connect(sendWatcher_, &QFutureWatcher<SendResult>::finished, this, [this] {
        const SendResult result = sendWatcher_->result();
        if (result.error.isEmpty()) {
            conversationView_->appendPlainText("Ollama: " + result.response + "\n");
        }
        else {
            conversationView_->appendPlainText("Error: " + result.error + "\n");
        }

        sendButton_->setEnabled(true);
        messageInput_->setFocus();
    });
}

MainWindow::~MainWindow()
{
    if (sendWatcher_->isRunning()) {
        sendWatcher_->waitForFinished();
    }
    delete screenSelector_;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == messageInput_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const bool isEnter = keyEvent->key() == Qt::Key_Return
            || keyEvent->key() == Qt::Key_Enter;

        if (isEnter && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (!sendWatcher_->isRunning()) {
                sendButton_->click();
            }
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::restoreAfterSelection()
{
    setWindowState(
        (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive
    );
    show();
    raise();
    activateWindow();
}

void MainWindow::sendMessage()
{
    const QString message = messageInput_->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    conversationView_->appendPlainText("You: " + message);
    messageInput_->clear();
    sendButton_->setEnabled(false);

    const QByteArray encodedMessage = message.toUtf8();
    sendWatcher_->setFuture(QtConcurrent::run(
        [this, encodedMessage] {
            try {
                return SendResult{
                    QString::fromStdString(ollama_.send(encodedMessage.toStdString())),
                    {}
                };
            }
            catch (const std::exception& error) {
                return SendResult{{}, QString::fromUtf8(error.what())};
            }
        }
    ));
}
