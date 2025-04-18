#include "VideoDisplayWidget.h"
#include <QDebug>

VideoDisplayWidget::VideoDisplayWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(320, 240);
}

void VideoDisplayWidget::setFrame(const cv::Mat& frame)
{
    std::lock_guard<std::mutex> lock(m_pixmapMutex);

    if (frame.empty())
    {
        // Clear the pixmap if the frame is empty
        m_pixmap = QPixmap();
    }
    else
    {
        QImage::Format format;
        if (frame.type() == CV_8UC3)
        {
            format = QImage::Format_BGR888;
        }
        else if (frame.type() == CV_8UC1)
        {
            format = QImage::Format_Grayscale8;
        }
        else
        {
            qWarning() << "VideoDisplayWidget::setFrame: Unsupported cv::Mat type:" << frame.type();
            m_pixmap = QPixmap(); // unsupported format
            return;
        }

        // cv::Mat might be temporary or change.
        QImage qimg(frame.data, frame.cols, frame.rows, static_cast<int>(frame.step), format);
        m_pixmap = QPixmap::fromImage(qimg.copy());
    }

    // Sched a repaint
    this->update();
}

void VideoDisplayWidget::clear() {
    std::lock_guard<std::mutex> lock(m_pixmapMutex);
    m_pixmap = QPixmap();
    this->update();
}

void VideoDisplayWidget::paintEvent(QPaintEvent* event)
{
    std::lock_guard<std::mutex> lock(m_pixmapMutex);
    QPainter painter(this);

    if (m_pixmap.isNull())
    {
        // placeholder
        painter.fillRect(this->rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(this->rect(), Qt::AlignCenter, "No Video");
    }
    else
    {
        // Draw
        painter.drawPixmap(this->rect(), m_pixmap, m_pixmap.rect());
    }
    QWidget::paintEvent(event);
}