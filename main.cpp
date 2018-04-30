#include "main.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <fstream>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    int width = 640;
    int height = 512;

    MainWindow * window = new MainWindow(width, height);
    window->show();

    return app.exec();
}

MainWindow::MainWindow(int width, int height)
    : logger(0),
      thermalImage(640, 512, QImage::Format_RGB888),
      depthImage(512, 424, QImage::Format_RGB888),
      infraredImage(512, 424, QImage::Format_RGB888),
      lastDepthDrawn(-1),
      lastInfraredDrawn(-1),
      lastThermalDrawn(-1)
{
    this->setMaximumSize(width + 2 * 512, height + 80);
    this->setMinimumSize(width + 2 * 512, height + 80);

    deviceWnd = new PvGenBrowserWnd;
    communicationWnd = new PvGenBrowserWnd;
    streamWnd = new PvGenBrowserWnd;

    QVBoxLayout * wrapperLayout = new QVBoxLayout;

    QHBoxLayout * mainLayout = new QHBoxLayout;
    QHBoxLayout * buttonLayout = new QHBoxLayout;
    QHBoxLayout * recordLayout = new QHBoxLayout;
    QHBoxLayout * parameterLayout = new QHBoxLayout;

    wrapperLayout->addLayout(mainLayout);
    thermalLabel = new QLabel(this);
    thermalLabel->setPixmap(QPixmap::fromImage(thermalImage));
    mainLayout->addWidget(thermalLabel);

    depthLabel = new QLabel(this);
    depthLabel->setPixmap(QPixmap::fromImage(depthImage));
    mainLayout->addWidget(depthLabel);

    infraredLabel = new QLabel(this);
    infraredLabel->setPixmap(QPixmap::fromImage(infraredImage));
    mainLayout->addWidget(infraredLabel);

    wrapperLayout->addLayout(buttonLayout);
    selectButton = new QPushButton("Select Camera", this);
    connect(selectButton, SIGNAL(clicked()), this, SLOT(SelectCamera()));
    buttonLayout->addWidget(selectButton);

    disconnectButton = new QPushButton("Disconnect Camera / Close", this);
    connect(disconnectButton, SIGNAL(clicked()), this, SLOT(DisconnectCamera()));
    buttonLayout->addWidget(disconnectButton);

    startCButton = new QPushButton("Start Capture", this);
    connect(startCButton, SIGNAL(clicked()), this, SLOT(StartCapture()));
    buttonLayout->addWidget(startCButton);

    stopCButton = new QPushButton("Stop Capture", this);
    connect(stopCButton, SIGNAL(clicked()), this, SLOT(StopCapture()));
    buttonLayout->addWidget(stopCButton);

    wrapperLayout->addLayout(recordLayout);
    startRButton = new QPushButton("Start Record", this);
    connect(startRButton, SIGNAL(clicked()), this, SLOT(StartRecording()));
    recordLayout->addWidget(startRButton);

    stopRButton = new QPushButton("Stop Record", this);
    connect(stopRButton, SIGNAL(clicked()), this, SLOT(StopRecording()));
    recordLayout->addWidget(stopRButton);

    singleRButton = new QPushButton("Single Image Record", this);
    connect(singleRButton, SIGNAL(clicked()), this, SLOT(SingleRecording()));
    recordLayout->addWidget(singleRButton);

    wrapperLayout->addLayout(parameterLayout);
    communicationButton = new QPushButton("Communication control", this);
    connect(communicationButton, SIGNAL(clicked()), this, SLOT(OnShowCommParameters()));
    parameterLayout->addWidget(communicationButton);

    deviceButton = new QPushButton("Device control", this);
    connect(deviceButton, SIGNAL(clicked()), this, SLOT(OnShowDeviceParameters()));
    parameterLayout->addWidget(deviceButton);

    streamButton = new QPushButton("Image stream control", this);
    connect(streamButton, SIGNAL(clicked()), this, SLOT(OnShowStreamParameters()));
    parameterLayout->addWidget(streamButton);

    setLayout(wrapperLayout);
}

MainWindow::~MainWindow()
{
//    if(logger){
//        delete logger;
//        logger = 0;
//    }
}

void MainWindow::TimerCallback()
{
    int lastDepth = logger->getKinect()->latestDepthIndex.getValue();
    int lastThermal = logger->getFlir()->latestThermalIndex.getValue();
    int lastInfrared = logger->getKinect()->latestInfraredIndex.getValue();
    if(lastDepth == -1 || lastThermal == -1 || lastThermal == -1)
        return;

    int bufferDIndex = lastDepth % OpenNI2Interface::numBuffers;
    int bufferIIndex = lastInfrared % OpenNI2Interface::numBuffers;
    int bufferTIndex = lastThermal % EbusFlirInterface::numBuffers;
    if(bufferDIndex != lastDepthDrawn)
    {
        memcpy(&depthBuffer[0], logger->getKinect()->frameBuffers[bufferDIndex].first, logger->getKinect()->width * logger->getKinect()->height * 2);
        cv::Mat1w depth(logger->getKinect()->height, logger->getKinect()->width, (unsigned short *)&depthBuffer[0]);
        cv::Mat tmp(logger->getKinect()->height, logger->getKinect()->width, CV_8UC1);
        Normalize(depth, tmp, 8.0);
        cv::Mat3b depthImg(logger->getKinect()->height, logger->getKinect()->width, (cv::Vec<unsigned char, 3> *)depthImage.bits());
        cv::cvtColor(tmp, depthImg, CV_GRAY2RGB);

        lastDepthDrawn = bufferDIndex;
        depthLabel->setPixmap(QPixmap::fromImage(depthImage));
    }

    if(bufferIIndex != lastInfraredDrawn)
    {
        memcpy(&infraredBuffer[0], logger->getKinect()->infraredFrameBuffers[bufferIIndex].first, logger->getKinect()->width * logger->getKinect()->height * 2);
        cv::Mat1w infrared(logger->getKinect()->height, logger->getKinect()->width, (unsigned short *)&infraredBuffer[0]);
        cv::Mat tmp(logger->getKinect()->height, logger->getKinect()->width, CV_8UC1);
        Normalize(infrared, tmp, 1.0);
        cv::Mat3b infraredImg(logger->getKinect()->height, logger->getKinect()->width, (cv::Vec<unsigned char, 3> *)infraredImage.bits());
        cv::cvtColor(tmp, infraredImg, CV_GRAY2RGB);

        lastInfraredDrawn = bufferIIndex;
        infraredLabel->setPixmap(QPixmap::fromImage(infraredImage));
    }

    if(bufferTIndex != lastThermalDrawn)
    {
        memcpy(&thermalBuffer[0], logger->getFlir()->frameBuffers[bufferTIndex].first, logger->getFlir()->width * logger->getFlir()->height);
        cv::Mat1b thermal(logger->getFlir()->height, logger->getFlir()->width, (unsigned char *)&thermalBuffer[0]);
        cv::Mat fthermal;
        cv::flip(thermal, fthermal, 0);

        cv::Mat3b thermalImg(logger->getFlir()->height, logger->getFlir()->width, (cv::Vec<unsigned char, 3> *)thermalImage.bits());
        cv::cvtColor(fthermal, thermalImg, CV_GRAY2RGB);
//        cv::applyColorMap(fthermal, thermalImg, cv::COLORMAP_JET);

        lastThermalDrawn = bufferTIndex;
        thermalLabel->setPixmap(QPixmap::fromImage(thermalImage));
    }
}

void MainWindow::SelectCamera()
{
    if(logger)
        return;
    logger = new Logger();
    if(!logger->ConnectCamera())
    {
        delete logger;
        logger = 0;
    }
}

void MainWindow::DisconnectCamera()
{
    if(logger){
        delete logger;
        logger = 0;
    }
    exit(0);
}

void MainWindow::StartCapture()
{
    if(!logger)
        return;
    if(!logger->ConnectKinect()){
        std::cout << "Connect Kinect Failed\n";
        return;
    }
    logger->getFlir()->StartAcquire();
    if(!timer)
    {
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(TimerCallback()));
        timer->start(15);
    }
}

void MainWindow::StopCapture()
{
    if(!logger)
        return;
    logger->getFlir()->StopAcquire();
    if(timer){
        timer->stop();
        delete timer;
        timer = 0;
    }
}

void MainWindow::OnShowCommParameters()
{
    if(!logger)
        return;
    logger->ShowGenWindow(
                communicationWnd,
                logger->getFlir()->GetCommunicationParameters(),
                "Communication Control",
                this);
}

void MainWindow::OnShowDeviceParameters()
{
    if(!logger)
        return;
    logger->ShowGenWindow(
                deviceWnd,
                logger->getFlir()->GetDeviceParameters(),
                "Device Control",
                this);
}

void MainWindow::OnShowStreamParameters()
{
    if(!logger)
        return;
    logger->ShowGenWindow(
                streamWnd,
                logger->getFlir()->GetStreamParameters(),
                "Image Stream Control",
                this);
}

void MainWindow::StartRecording()
{
    if(!logger)
        return;
    logger->StartWriting();
    std::cout << "Start Recording" << std::endl;
}

void MainWindow::StopRecording()
{
    if(!logger)
        return;
    logger->StopWriting();
    std::cout << "Stop Recording" << std::endl;
}

void MainWindow::SingleRecording()
{
    if(!logger)
        return;
    logger->SingleWriting();
    std::cout << "Save Single Image Success" << std::endl;
}

void MainWindow::Normalize(const cv::Mat& src, cv::Mat& dst, float scale)
{
    int width = src.rows;
    int height = src.cols;
    for(int i = 0; i < width; ++i)
    {
        const unsigned short * s = src.ptr<unsigned short>(i);
        unsigned char * d = dst.ptr<unsigned char>(i);
        for(int j = 0; j < height; ++j)
        {
            d[j] = float(s[j]) * scale / 65535.0 * 255.0;
        }
    }
}
