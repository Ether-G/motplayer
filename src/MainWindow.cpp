#include "MainWindow.h"
#include "VideoProcessor.h"
#include "VideoDisplayWidget.h"

#include <QApplication>
#include <QFileDialog>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_videoProcessor(std::make_unique<VideoProcessor>())
{
    setupUI();
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    connectSignalsSlots();

    updateUIState();

    setWindowTitle("Motion Video Player");
    resize(1024, 600);
}

MainWindow::~MainWindow()
{
    // VideoProcessor unique_ptr handles deletion and thread cleanup via its destructor
    qInfo() << "MainWindow destructor called";
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    this->setCentralWidget(m_centralWidget);

    m_originalDisplayWidget = new VideoDisplayWidget(m_centralWidget);
    m_maskDisplayWidget = new VideoDisplayWidget(m_centralWidget);

    QHBoxLayout* displayLayout = new QHBoxLayout();
    displayLayout->addWidget(m_originalDisplayWidget, 1);
    displayLayout->addWidget(m_maskDisplayWidget, 1);

    m_openButton = new QPushButton("Open Video", m_centralWidget);
    m_playPauseButton = new QPushButton(m_centralWidget);

    // Delta Control
    QLabel* deltaLabel = new QLabel("Frame Delta:", m_centralWidget);
    m_deltaSpinBox = new QSpinBox(m_centralWidget);
    m_deltaSpinBox->setRange(1, 30);
    m_deltaSpinBox->setValue(3);
    m_deltaSpinBox->setSuffix(" frames");
    QHBoxLayout* deltaLayout = new QHBoxLayout();
    deltaLayout->addWidget(deltaLabel);
    deltaLayout->addWidget(m_deltaSpinBox);
    deltaLayout->addStretch();

    // Threshold Control
    QLabel* thresholdLabel = new QLabel("Motion Threshold:", m_centralWidget);
    m_thresholdSlider = new QSlider(Qt::Horizontal, m_centralWidget);
    m_thresholdSlider->setRange(0, 255);
    m_thresholdSlider->setValue(30);
    m_thresholdValueLabel = new QLabel(QString::number(m_thresholdSlider->value()), m_centralWidget);
    m_thresholdValueLabel->setMinimumWidth(30);
    QHBoxLayout* thresholdLayout = new QHBoxLayout();
    thresholdLayout->addWidget(thresholdLabel);
    thresholdLayout->addWidget(m_thresholdSlider);
    thresholdLayout->addWidget(m_thresholdValueLabel);


    // Info Label
    m_videoInfoLabel = new QLabel("No video loaded.", m_centralWidget);
    m_videoInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout* controlLayout = new QVBoxLayout();
    controlLayout->addWidget(m_openButton);
    controlLayout->addWidget(m_playPauseButton);
    controlLayout->addLayout(deltaLayout);
    controlLayout->addLayout(thresholdLayout);
    controlLayout->addWidget(m_videoInfoLabel);
    controlLayout->addStretch();


    QHBoxLayout* mainBodyLayout = new QHBoxLayout();
    mainBodyLayout->addLayout(displayLayout, 4);
    mainBodyLayout->addLayout(controlLayout, 1);


    QVBoxLayout* centralLayout = new QVBoxLayout(m_centralWidget);
    centralLayout->addLayout(mainBodyLayout);

    m_centralWidget->setLayout(centralLayout);
}


void MainWindow::createActions()
{
    m_openAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "&Open Video...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip("Open a video file for processing");

    m_playPauseAction = new QAction(this);
    m_playPauseAction->setStatusTip("Play or pause the video processing");
    connect(m_playPauseAction, &QAction::triggered, this, &MainWindow::onPlayPause);


    m_exitAction = new QAction(style()->standardIcon(QStyle::SP_DialogCloseButton), "E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_openAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu* controlMenu = menuBar()->addMenu("&Control");
    controlMenu->addAction(m_playPauseAction);
}

void MainWindow::createToolBar()
{
    QToolBar* fileToolBar = addToolBar("File");
    fileToolBar->addAction(m_openAction);

    QToolBar* controlToolBar = addToolBar("Control");
    controlToolBar->addAction(m_playPauseAction);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::connectSignalsSlots()
{
    // UI Element -> MainWindow Slots
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(m_playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_deltaSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onDeltaChanged);
    connect(m_thresholdSlider, &QSlider::valueChanged, this, &MainWindow::onThresholdChanged);

    // VideoProcessor -> MainWindow Slots
    connect(m_videoProcessor.get(), &VideoProcessor::newFramesReady, this, &MainWindow::updateFrames);
    connect(m_videoProcessor.get(), &VideoProcessor::processingFinished, this, &MainWindow::handleProcessingFinished);
    connect(m_videoProcessor.get(), &VideoProcessor::errorOccurred, this, &MainWindow::handleVideoLoadError);
    connect(m_videoProcessor.get(), &VideoProcessor::videoInfoReady, this, &MainWindow::handleVideoInfoReady);
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open Video File",
                                                    QDir::homePath(),
                                                    "Video Files (*.mp4 *.avi *.mov *.mkv *.wmv);;All Files (*)");

    if (!fileName.isEmpty()) {
        m_currentFilePath = fileName;
        m_isFileLoaded = false;
        m_isPlaying = false;
        m_videoProcessor->loadVideo(m_currentFilePath);
        m_statusLabel->setText("Loading: " + QFileInfo(m_currentFilePath).fileName());
    }
}

void MainWindow::onPlayPause()
{
    if (!m_isFileLoaded) return;

    if (m_isPlaying) {
        m_videoProcessor->pause();
        m_isPlaying = false;
        m_statusLabel->setText("Paused: " + QFileInfo(m_currentFilePath).fileName());
    } else {
        m_videoProcessor->startProcessing();
        m_isPlaying = true;
        m_statusLabel->setText("Playing: " + QFileInfo(m_currentFilePath).fileName());
    }
    updateUIState();
}

void MainWindow::onDeltaChanged(int value)
{
    m_videoProcessor->setFrameDelta(value);
}

void MainWindow::onThresholdChanged(int value)
{
    m_thresholdValueLabel->setText(QString::number(value));
    m_videoProcessor->setMotionThreshold(value);
}

void MainWindow::updateFrames(const cv::Mat& original, const cv::Mat& mask)
{
    if(m_originalDisplayWidget) m_originalDisplayWidget->setFrame(original);
    if(m_maskDisplayWidget) m_maskDisplayWidget->setFrame(mask);
}

void MainWindow::handleProcessingFinished()
{
    m_isPlaying = false;
    m_statusLabel->setText("Finished: " + QFileInfo(m_currentFilePath).fileName());
    updateUIState();
    // clear displays?
    // m_originalDisplayWidget->clear();
    // m_maskDisplayWidget->clear();
}

void MainWindow::handleVideoLoadError(const QString& message)
{
    QMessageBox::warning(this, "Video Load Error", message);
    m_currentFilePath.clear();
    m_isFileLoaded = false;
    m_isPlaying = false;
    m_statusLabel->setText("Error loading video");
    m_videoInfoLabel->setText("Load Error.");
     if(m_originalDisplayWidget) m_originalDisplayWidget->clear();
     if(m_maskDisplayWidget) m_maskDisplayWidget->clear();
    updateUIState();
}

void MainWindow::handleVideoInfoReady(double fps, int width, int height)
{
     m_isFileLoaded = true;
     m_isPlaying = false;
     m_videoInfoLabel->setText(QString("Loaded: %1x%2 @ %3 FPS")
                              .arg(width)
                              .arg(height)
                              .arg(QString::number(fps, 'f', 2)));
     m_statusLabel->setText("Ready: " + QFileInfo(m_currentFilePath).fileName());
     if(m_originalDisplayWidget) m_originalDisplayWidget->clear();
     if(m_maskDisplayWidget) m_maskDisplayWidget->clear();
     updateUIState();
}


void MainWindow::updateUIState()
{
    m_playPauseButton->setEnabled(m_isFileLoaded);
    m_playPauseAction->setEnabled(m_isFileLoaded);
    m_deltaSpinBox->setEnabled(true);
    m_thresholdSlider->setEnabled(true);

    if (m_isPlaying) {
        m_playPauseButton->setText("Pause");
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        m_playPauseAction->setText("&Pause");
        m_playPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        m_openButton->setEnabled(false);
        m_openAction->setEnabled(false);

    } else {
        m_playPauseButton->setText("Play");
        m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
         m_playPauseAction->setText("&Play");
         m_playPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        m_openButton->setEnabled(true);
        m_openAction->setEnabled(true);
    }
}