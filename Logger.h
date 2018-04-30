/*
 * EbusFlirInterface.h
 *
 *  Created on: 8/1 2018
 *      Author: Baobei Xu
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <opencv2/opencv.hpp>
#include <QWidget>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "FlirKinect/OpenNI2Interface.h"
#include "FlirKinect/EbusFlirInterface.h"

class Logger
{
public:
    Logger();
    virtual ~Logger();

    bool ConnectCamera();

    bool ConnectKinect();

    void StartWriting();
    void StopWriting();
    void SingleWriting();

    void ShowGenWindow( PvGenBrowserWnd *aWnd, PvGenParameterArray *aArray, const QString &aTitle, QWidget * gui );

    OpenNI2Interface * getKinect();
    EbusFlirInterface * getFlir();

private:
    OpenNI2Interface * kinect;
    EbusFlirInterface * flir;

    int lastDepthWritten;
    boost::thread * writeDepthThread;
    int lastThermalWritten;
    boost::thread * writeThermalThread;
    ThreadMutexObject<bool> writing;

    int singleWrite;

    std::string tfolderName;
    std::string dfolderName;
    std::string ifolderName;

    void ThermalWritingThread();
    void DepthWritingThread();

    bool MkDir(std::string dirFolder);
};

#endif /* LOGGER_H_ */
