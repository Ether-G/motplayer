#include "MainWindow.h"
#include <QApplication>
#include <QMetaType>
#include <opencv2/opencv.hpp>
#include <QDebug>

Q_DECLARE_METATYPE(cv::Mat);

int main(int argc, char *argv[])
{
    // Register cv::Mat type for cross-thread signal/slot connections
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qInfo() << "cv::Mat registered with QMetaTypeSystem.";

    QApplication a(argc, argv);

    QApplication::setApplicationName("MotionVideoPlayer");
    QApplication::setOrganizationName("Ether-G");
    QApplication::setApplicationVersion("1.0");

    MainWindow w;
    w.show();

    return a.exec();
}