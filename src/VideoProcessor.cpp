#include "VideoProcessor.h"
#include <QDebug>

VideoProcessor::VideoProcessor(QObject *parent)
    : QObject(parent),
      m_stopRequested(false),
      m_pauseRequested(false),
      m_frameDelta(3),
      m_motionThreshold(30),
      m_fps(0.0),
      m_videoWidth(0),
      m_videoHeight(0),
      m_thread(new QThread(this))
{
    this->moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &VideoProcessor::run);

    // Connect finished signal for cleanup
    // connect(m_thread, &QThread::finished, this, &VideoProcessor::someCleanupSlot);

    qInfo() << "VideoProcessor constructed";
}

VideoProcessor::~VideoProcessor()
{
    qInfo() << "VideoProcessor destructor called";
    stop(); // STOP FOR CLEAN!
}

void VideoProcessor::loadVideo(const QString& filePath)
{
    qInfo() << "Loading video:" << filePath;
    if (m_thread->isRunning()) {
       m_stopRequested = true;
       m_thread->quit();
       m_thread->wait(1000);
       if(m_thread->isRunning()) {
           qWarning() << "Video processing thread did not finish gracefully, terminating.";
           m_thread->terminate();
           m_thread->wait();
       }
       m_stopRequested = false;
    }

    m_filePath = filePath;

    cv::VideoCapture temp_capture;
    if (!temp_capture.open(m_filePath.toStdString())) {
        emit errorOccurred(QString("Failed to open video file: %1").arg(m_filePath));
        m_filePath.clear();
    } else {
        m_fps = temp_capture.get(cv::CAP_PROP_FPS);
        m_videoWidth = static_cast<int>(temp_capture.get(cv::CAP_PROP_FRAME_WIDTH));
        m_videoHeight = static_cast<int>(temp_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
        emit videoInfoReady(m_fps, m_videoWidth, m_videoHeight);
        qInfo() << "Video info ready - FPS:" << m_fps << " W:" << m_videoWidth << " H:" << m_videoHeight;
    }
    temp_capture.release();
}

void VideoProcessor::startProcessing()
{
    if (m_filePath.isEmpty()) {
        emit errorOccurred("No video file loaded.");
        return;
    }
    if (m_thread->isRunning()) {
        qWarning() << "Processing thread already running.";
        resume();
        return;
    }

    qInfo() << "Starting processing thread...";
    m_stopRequested = false;
    m_pauseRequested = false;
    m_thread->start();
}

void VideoProcessor::pause()
{
    qInfo() << "Pause requested";
    m_pauseRequested = true;
}

void VideoProcessor::resume()
{
    qInfo() << "Resume requested";
    m_pauseRequested = false;
}

void VideoProcessor::stop()
{
    qInfo() << "Stop requested";
    m_stopRequested = true;
    if (m_thread->isRunning()) {
        m_thread->quit();
        if (!m_thread->wait(2000)) {
             qWarning() << "Video processing thread did not finish gracefully after 2s, terminating.";
             m_thread->terminate();
             m_thread->wait();
        }
        qInfo() << "Video processing thread finished.";
    } else {
         qInfo() << "Video processing thread was not running.";
    }
     m_frameBuffer.clear();
}

void VideoProcessor::setFrameDelta(int delta)
{
    if (delta > 0) {
        qInfo() << "Setting frame delta to" << delta;
        m_frameDelta = delta;
    } else {
         qWarning() << "Frame delta must be positive.";
    }
}

void VideoProcessor::setMotionThreshold(int threshold)
{
    if (threshold >= 0 && threshold <= 255) {
        qInfo() << "Setting motion threshold to" << threshold;
        m_motionThreshold = threshold;
    } else {
        qWarning() << "Motion threshold must be between 0 and 255.";
    }
}

void VideoProcessor::run()
{
    qInfo() << "VideoProcessor::run() started in thread" << QThread::currentThreadId();

    if (!m_capture.open(m_filePath.toStdString())) {
        emit errorOccurred(QString("Failed to open video file in worker thread: %1").arg(m_filePath));
        return;
    }

    m_fps = m_capture.get(cv::CAP_PROP_FPS);
    m_videoWidth = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    m_videoHeight = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (m_fps <= 0) m_fps = 30.0; // Default FPS if reading fails

    m_frameBuffer.clear();

    int currentDelta = m_frameDelta.load();
    int delayMs = static_cast<int>((1000.0 / m_fps) * currentDelta);
    if (delayMs <= 0) delayMs = 33 * currentDelta;

    cv::Mat currentFrame;
    cv::Mat grayN, grayNminusDelta, diff, mask;
    QElapsedTimer frameTimer;

    while (!m_stopRequested.load())
    {
        while (m_pauseRequested.load() && !m_stopRequested.load()) {
            QThread::msleep(50);
        }
        if (m_stopRequested.load()) break;

        frameTimer.start();

        if (!m_capture.read(currentFrame) || currentFrame.empty()) {
            qInfo() << "End of video or read error.";
            break;
        }

        currentDelta = m_frameDelta.load();
        delayMs = static_cast<int>((1000.0 / m_fps) * currentDelta);
        if (delayMs <= 0) delayMs = 33 * currentDelta;

        m_frameBuffer.push_back(currentFrame.clone());

        while (m_frameBuffer.size() > static_cast<size_t>(currentDelta + 1) && !m_frameBuffer.empty()) {
             m_frameBuffer.pop_front();
        }

        cv::Mat motionMaskToSend;
        if (m_frameBuffer.size() >= static_cast<size_t>(currentDelta + 1))
        {
            const cv::Mat& frameN = m_frameBuffer.back();
            const cv::Mat& frameNminusDelta = m_frameBuffer.front();

            cv::cvtColor(frameN, grayN, cv::COLOR_BGR2GRAY);
            cv::cvtColor(frameNminusDelta, grayNminusDelta, cv::COLOR_BGR2GRAY);
            cv::absdiff(grayN, grayNminusDelta, diff);
            cv::threshold(diff, motionMaskToSend, m_motionThreshold.load(), 255, cv::THRESH_BINARY);
        }

        emit newFramesReady(currentFrame, motionMaskToSend);

        int elapsed = static_cast<int>(frameTimer.elapsed());
        int waitTime = delayMs - elapsed;
        if (waitTime > 0 && !m_stopRequested.load()) {
            QThread::msleep(waitTime);
        }
    }

    m_capture.release();
    m_frameBuffer.clear();
    qInfo() << "VideoProcessor::run() finished.";
    emit processingFinished();
}