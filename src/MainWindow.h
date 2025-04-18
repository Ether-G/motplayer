#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <memory>
#include <opencv2/opencv.hpp>

class VideoProcessor;
class VideoDisplayWidget;
class QPushButton;
class QLabel;
class QSlider;
class QAction;
class QSpinBox; // SpinBox for better control over Delta..?

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // UI
    void onOpenFile();
    void onPlayPause();
    void onDeltaChanged(int value);
    void onThresholdChanged(int value);

    // VideoProcessor
    void updateFrames(const cv::Mat& original, const cv::Mat& mask);
    void handleProcessingFinished();
    void handleVideoLoadError(const QString& message);
    void handleVideoInfoReady(double fps, int width, int height);

    // Internal UI Update
    void updateUIState();


private:
    void setupUI();
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void connectSignalsSlots();


    // UI
    QWidget* m_centralWidget = nullptr;
    VideoDisplayWidget* m_originalDisplayWidget = nullptr;
    VideoDisplayWidget* m_maskDisplayWidget = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    QPushButton* m_openButton = nullptr;
    QSlider* m_thresholdSlider = nullptr;
    QLabel* m_thresholdValueLabel = nullptr;
    QSpinBox* m_deltaSpinBox = nullptr;
    QLabel* m_deltaValueLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_videoInfoLabel = nullptr;

    // Actions
    QAction* m_openAction = nullptr;
    QAction* m_playPauseAction = nullptr;
    QAction* m_exitAction = nullptr;

    // Backend
    std::unique_ptr<VideoProcessor> m_videoProcessor;

    // State
    QString m_currentFilePath;
    bool m_isFileLoaded = false;
    bool m_isPlaying = false;
};

#endif // MAINWINDOW_H