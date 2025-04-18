#ifndef VIDEODISPLAYWIDGET_H
#define VIDEODISPLAYWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <opencv2/opencv.hpp>
#include <mutex>

class VideoDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoDisplayWidget(QWidget *parent = nullptr);
    ~VideoDisplayWidget() override = default;

public slots:
    void setFrame(const cv::Mat& frame);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    std::mutex m_pixmapMutex;
};

#endif // VIDEODISPLAYWIDGET_H