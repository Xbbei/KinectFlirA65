#include "Logger.h"

Logger::Logger()
    : kinect(0),
      flir(0),
      lastDepthWritten(-1),
      writeDepthThread(0),
      lastThermalWritten(-1),
      writeThermalThread(0),
      singleWrite(0)
{
    writing.assignValue(false);
    tfolderName = "thermal";
    dfolderName = "depth";
    ifolderName = "infrared";
}

Logger::~Logger()
{
    if(writing.getValue())
    {
        assert(writeDepthThread && writeThermalThread);
        StopWriting();
    }
    if(flir)
    {
        delete flir;
        flir = 0;
    }
    if(kinect)
    {
        delete kinect;
        kinect = 0;
    }
}

bool Logger::ConnectCamera()
{
    flir = new EbusFlirInterface();
    if(!flir->IsOK())
    {
        delete flir;
        flir = 0;
        return false;
    }
    return true;
}

bool Logger::ConnectKinect()
{
    kinect = new OpenNI2Interface();
    if(!kinect->ok())
    {
        delete kinect;
        kinect = 0;
        return false;
    }
    return true;
}

void Logger::StartWriting()
{
    if(!(flir->IsOK() && flir->isAcquisition.getValue()) || !kinect->ok())
        return;

    if(writeDepthThread || writing.getValue() || writeThermalThread)
        return;

    if(!MkDir(tfolderName) || !MkDir(dfolderName)){
        std::cout << "make folder failed! Fuck!\n";
        return;
    }

    writing.assignValue(true);
    writeDepthThread = new boost::thread(boost::bind(&Logger::DepthWritingThread,
                                                            this));
    writeThermalThread = new boost::thread(boost::bind(&Logger::ThermalWritingThread,
                                                            this));
}

void Logger::StopWriting()
{
    if(!writing.getValue())
        return;

    writing.assignValue(false);
    writeDepthThread->join();
    writeThermalThread->join();
    delete writeDepthThread;
    writeDepthThread = 0;
    delete writeThermalThread;
    writeThermalThread = 0;
}

void Logger::SingleWriting()
{
    if(!kinect->ok() || !flir->IsOK())
        return;

    int latestDepth = kinect->latestDepthIndex.getValue();
    if(latestDepth == -1)
    {
        return;
    }
    int depthBufferIndex = latestDepth % OpenNI2Interface::numBuffers;

    int latestInfrared = kinect->latestInfraredIndex.getValue();
    if(latestInfrared == -1)
    {
        return;
    }
    int infraredBufferIndex = latestInfrared % OpenNI2Interface::numBuffers;

    int latestThermal = flir->latestThermalIndex.getValue();
    if(latestThermal == -1)
    {
        return;
    }
    int thermalBufferIndex = latestThermal % OpenNI2Interface::numBuffers;

    cv::Mat depth(kinect->height, kinect->width, CV_16UC1);
    memcpy(depth.data, kinect->frameBuffers[depthBufferIndex].first, kinect->height * kinect->width * 2);
    cv::Mat infrared(kinect->height, kinect->width, CV_16UC1);
    memcpy(infrared.data, kinect->infraredFrameBuffers[infraredBufferIndex].first, kinect->height * kinect->width * 2);
    cv::Mat thermal(flir->height, flir->width, CV_8UC1);
    memcpy(thermal.data, flir->frameBuffers[thermalBufferIndex].first, flir->height * flir->width);
    cv::Mat fthermal;
    cv::flip(thermal, fthermal, 0);

    std::string num;
    num = boost::lexical_cast<std::string>(singleWrite);
    std::string depthfile = dfolderName + "/depth" + num + ".png";
    std::string infraredfile = ifolderName + "/infrared" + num + ".png";
    std::string thermalfile = tfolderName + "/thermal" + num + ".png";
    cv::imwrite(depthfile, depth);
    cv::imwrite(infraredfile, infrared);
    cv::imwrite(thermalfile, fthermal);

    singleWrite++;
}

void Logger::ShowGenWindow( PvGenBrowserWnd *aWnd, PvGenParameterArray *aArray, const QString &aTitle, QWidget * gui )
{
    if ( aWnd->GetQWidget()->isVisible() )
    {
        aWnd->SetGenParameterArray( NULL );
        aWnd->Close();
        return;
    }

    aWnd->SetTitle( aTitle.toAscii().data() );

    aWnd->ShowModeless(gui);
    aWnd->SetGenParameterArray( aArray );
}

OpenNI2Interface * Logger::getKinect()
{
    return kinect;
}

EbusFlirInterface * Logger::getFlir()
{
    return flir;
}

void Logger::ThermalWritingThread()
{
    while(writing.getValue())
    {
        int lastThermal = flir->latestThermalIndex.getValue();
        if(lastThermal == -1)
        {
            continue;
        }

        int bufferIndex = lastThermal % EbusFlirInterface::numBuffers;
        if(lastThermalWritten == bufferIndex)
        {
            continue;
        }

        cv::Mat1b thermal(flir->height, flir->width, flir->frameBuffers[bufferIndex].first);
        cv::Mat fthermal;
        cv::flip(thermal, fthermal, 0);
        std::string imagename = "";
        imagename = boost::lexical_cast<std::string>(flir->frameBuffers[bufferIndex].second);
        imagename = tfolderName + "/" + imagename + ".png";
        cv::imwrite(imagename, fthermal);

        lastThermalWritten = bufferIndex;
    }
}

void Logger::DepthWritingThread()
{
    while(writing.getValue())
    {
        int lastDepth = kinect->latestDepthIndex.getValue();
        if(lastDepth == -1)
        {
            continue;
        }

        int bufferIndex = lastDepth % OpenNI2Interface::numBuffers;
        if(lastDepthWritten == bufferIndex)
        {
            continue;
        }

        cv::Mat1w depth(kinect->height, kinect->width, (unsigned short *)kinect->frameBuffers[bufferIndex].first);
        std::string imagename = "";
        imagename = boost::lexical_cast<std::string>(kinect->frameBuffers[bufferIndex].second);
        imagename = dfolderName + "/" + imagename + ".png";
        cv::imwrite(imagename, depth);

        lastDepthWritten = bufferIndex;
    }
}

bool Logger::MkDir(std::string dirFolder)
{
    if(access(dirFolder.c_str(), 0) == 0)
        return true;
    else{
        int flag = mkdir(dirFolder.c_str(), S_IRWXU);
        return flag == 0;
    }
}
