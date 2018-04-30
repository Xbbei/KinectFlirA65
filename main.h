/*
 * EbusFlirInterface.h
 *
 *  Created on: 8/1 2018
 *      Author: Baobei Xu
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <vector>
#include <string>
#include <QImage>
#include <QApplication>
#include <QString>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <qmessagebox.h>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QTextEdit>
#include <QComboBox>
#include <QMouseEvent>
#include <qlineedit.h>
#include <QPushButton>
#include <QFileDialog>
#include <QPainter>

#include "Logger.h"

class MainWindow : public QWidget
{
    Q_OBJECT;

public:
    MainWindow(int width, int height);
    virtual ~MainWindow();

private slots:
    void TimerCallback();
    void SelectCamera();
    void DisconnectCamera();
    void StartCapture();
    void StopCapture();
    void OnShowCommParameters();
    void OnShowDeviceParameters();
    void OnShowStreamParameters();
    void StartRecording();
    void StopRecording();
    void SingleRecording();

private:
    Logger * logger;

    QImage depthImage;
    QLabel * depthLabel;
    QImage infraredImage;
    QLabel * infraredLabel;
    QImage thermalImage;
    QLabel * thermalLabel;

    QTimer * timer;

    QPushButton * selectButton;
    QPushButton * disconnectButton;

    QPushButton * startCButton;
    QPushButton * stopCButton;

    QPushButton * startRButton;
    QPushButton * stopRButton;
    QPushButton * singleRButton;

    QPushButton *communicationButton;
    QPushButton *deviceButton;
    QPushButton *streamButton;
    PvGenBrowserWnd *deviceWnd;
    PvGenBrowserWnd *communicationWnd;
    PvGenBrowserWnd *streamWnd;

    unsigned char thermalBuffer[640 * 512];
    unsigned char depthBuffer[512 * 424 * 2];
    unsigned char infraredBuffer[512 * 424 * 2];
    int lastDepthDrawn;
    int lastInfraredDrawn;
    int lastThermalDrawn;

    void Normalize(const cv::Mat& src, cv::Mat& dst, float scale);
};

#endif //////End of MAIN_H_
