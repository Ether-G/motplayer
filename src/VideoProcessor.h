#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QElapsedTimer>
#include <QMetaType>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <deque>
#include <mutex>


class VideoProcessor : public QObject
{
    Q_OBJECT

public:
    explicit VideoProcessor(QObject *parent = nullptr);
    ~VideoProcessor() override;
    VideoProcessor(const VideoProcessor&) = delete;
    VideoProcessor& operator=(const VideoProcessor&) = delete;

public slots:
    void loadVideo(const QString& filePath);
    void startProcessing();
    void pause();
    void resume();
    void stop();
    void setFrameDelta(int delta);
    void setMotionThreshold(int threshold);

signals:
    void newFramesReady(const cv::Mat& original, const cv::Mat& mask);
    void processingFinished();
    void errorOccurred(const QString& message);
    void videoInfoReady(double fps, int width, int height);

private slots:
    void run();

private:
    cv::VideoCapture m_capture;
    QString m_filePath;

    std::atomic<bool> m_stopRequested;
    std::atomic<bool> m_pauseRequested;
    std::atomic<int> m_frameDelta;
    std::atomic<int> m_motionThreshold;

    double m_fps;
    int m_videoWidth;
    int m_videoHeight;

    std::deque<cv::Mat> m_frameBuffer;

    QThread* m_thread;
};

#endif // VIDEOPROCESSOR_H