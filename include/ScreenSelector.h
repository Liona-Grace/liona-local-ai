#pragma once

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

class ScreenSelector final : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenSelector(QWidget* parent = nullptr);

    void startSelection();

signals:
    void selectionFinished(const QImage& image);
    void selectionCanceled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QRect selectedRect() const;
    QImage cropSelection(const QRect& logicalRect) const;
    void cancelSelection();

    QImage screenshot_;
    QPoint selectionStart_;
    QPoint selectionEnd_;
    bool selecting_ = false;
};
