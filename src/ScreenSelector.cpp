#include "ScreenSelector.h"

#include <QCursor>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

#include <algorithm>

ScreenSelector::ScreenSelector(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(
        Qt::Tool
        | Qt::FramelessWindowHint
        | Qt::WindowStaysOnTopHint
    );
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
}

void ScreenSelector::startSelection()
{
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        emit selectionCanceled();
        return;
    }

    screenshot_ = screen->grabWindow(0).toImage();
    if (screenshot_.isNull()) {
        emit selectionCanceled();
        return;
    }

    selecting_ = false;
    selectionStart_ = {};
    selectionEnd_ = {};
    setGeometry(screen->geometry());
    show();
    raise();
    activateWindow();
    setFocus();
}

void ScreenSelector::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        cancelSelection();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ScreenSelector::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    selecting_ = true;
    selectionStart_ = event->position().toPoint();
    selectionEnd_ = selectionStart_;
    update();
}

void ScreenSelector::mouseMoveEvent(QMouseEvent* event)
{
    if (!selecting_) {
        return;
    }

    selectionEnd_ = event->position().toPoint();
    update();
}

void ScreenSelector::mouseReleaseEvent(QMouseEvent* event)
{
    if (!selecting_ || event->button() != Qt::LeftButton) {
        return;
    }

    selectionEnd_ = event->position().toPoint();
    selecting_ = false;

    const QRect selection = selectedRect();
    if (selection.width() < 2 || selection.height() < 2) {
        cancelSelection();
        return;
    }

    const QImage selectedImage = cropSelection(selection);
    hide();
    emit selectionFinished(selectedImage);
}

void ScreenSelector::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawImage(rect(), screenshot_);
    painter.fillRect(rect(), QColor(0, 0, 0, 110));

    const QRect selection = selectedRect();
    if (!selection.isEmpty()) {
        painter.drawImage(selection, cropSelection(selection));
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(selection.adjusted(0, 0, -1, -1));
    }
}

QRect ScreenSelector::selectedRect() const
{
    if (!selecting_ && selectionStart_ == selectionEnd_) {
        return {};
    }
    return QRect(selectionStart_, selectionEnd_).normalized().intersected(rect());
}

QImage ScreenSelector::cropSelection(const QRect& logicalRect) const
{
    const double scaleX = static_cast<double>(screenshot_.width()) / width();
    const double scaleY = static_cast<double>(screenshot_.height()) / height();
    const QRect imageRect(
        static_cast<int>(logicalRect.x() * scaleX),
        static_cast<int>(logicalRect.y() * scaleY),
        std::max(1, static_cast<int>(logicalRect.width() * scaleX)),
        std::max(1, static_cast<int>(logicalRect.height() * scaleY))
    );
    return screenshot_.copy(imageRect.intersected(screenshot_.rect()));
}

void ScreenSelector::cancelSelection()
{
    selecting_ = false;
    hide();
    emit selectionCanceled();
}
