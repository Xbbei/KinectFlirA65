/*
 * OpenNI2Interface.cpp
 *
 *  Created on: 20 May 2013
 *      Author: thomas
 */

#include "OpenNI2Interface.h"

OpenNI2Interface::OpenNI2Interface(int inWidth, int inHeight, int fps)
 : width(inWidth),
   height(inHeight),
   fps(fps),
   initSuccessful(true)
{
    //Setup
    openni::Status rc = openni::STATUS_OK;

    const char * deviceURI = openni::ANY_DEVICE;

    rc = openni::OpenNI::initialize();

    std::string errorString(openni::OpenNI::getExtendedError());

    if(errorString.length() > 0)
    {
        errorText.append(errorString);
        initSuccessful = false;
    }
    else
    {
        rc = device.open(deviceURI);
        if (rc != openni::STATUS_OK)
        {
            std::cout << "Open Kinect Failed!\n";
            errorText.append(openni::OpenNI::getExtendedError());
            openni::OpenNI::shutdown();
            initSuccessful = false;
        }
        else
        {
            openni::VideoMode depthMode;
            depthMode.setFps(fps);
            depthMode.setPixelFormat(openni::PIXEL_FORMAT_DEPTH_1_MM);
            depthMode.setResolution(width, height);

            openni::VideoMode infraredMode;
            infraredMode.setFps(fps);
            infraredMode.setPixelFormat(openni::PIXEL_FORMAT_GRAY16);
            infraredMode.setResolution(width, height);

            rc = depthStream.create(device, openni::SENSOR_DEPTH);
            if (rc == openni::STATUS_OK)
            {
                depthStream.setVideoMode(depthMode);
                rc = depthStream.start();
                if (rc != openni::STATUS_OK)
                {
                    errorText.append(openni::OpenNI::getExtendedError());
                    depthStream.destroy();
                    initSuccessful = false;
                }
            }
            else
            {
                errorText.append(openni::OpenNI::getExtendedError());
                initSuccessful = false;
            }

            rc = infraredStream.create(device, openni::SENSOR_IR);
            if (rc == openni::STATUS_OK)
            {
                infraredStream.setVideoMode(infraredMode);
                rc = infraredStream.start();
                if (rc != openni::STATUS_OK)
                {
                    errorText.append(openni::OpenNI::getExtendedError());
                    infraredStream.destroy();
                    initSuccessful = false;
                }
            }
            else
            {
                errorText.append(openni::OpenNI::getExtendedError());
                initSuccessful = false;
            }

            if (!depthStream.isValid() || !infraredStream.isValid())
            {
                errorText.append(openni::OpenNI::getExtendedError());
                openni::OpenNI::shutdown();
                initSuccessful = false;
            }

            if(initSuccessful)
            {
                latestDepthIndex.assignValue(-1);
                latestInfraredIndex.assignValue(-1);

                for(int i = 0; i < numBuffers; i++)
                {
                    uint8_t * newDepth = (uint8_t *)calloc(width * height * 2, sizeof(uint8_t));
                    uint8_t * newInfrared = (uint8_t *)calloc(width * height * 2, sizeof(uint8_t));
                    frameBuffers[i] = std::pair<uint8_t *, int64_t>(newDepth, 0);
                    infraredFrameBuffers[i] = std::pair<uint8_t *, int64_t>(newInfrared, 0);
                }

                depthCallback = new DepthCallback(lastDepthTime,
                                                  latestDepthIndex,
                                                  frameBuffers);

                infraredCallback = new InfraredCallback(lastInfraredTime,
                                                        latestInfraredIndex,
                                                        infraredFrameBuffers);

                depthStream.setMirroringEnabled(false);
                infraredStream.setMirroringEnabled(false);

                depthStream.addNewFrameListener(depthCallback);
                infraredStream.addNewFrameListener(infraredCallback);
            }
        }
    }
}

OpenNI2Interface::~OpenNI2Interface()
{
    if(initSuccessful)
    {
        depthStream.removeNewFrameListener(depthCallback);
        infraredStream.removeNewFrameListener(infraredCallback);
        depthStream.stop();
        infraredStream.stop();
        depthStream.destroy();
        infraredStream.destroy();

        device.close();
        openni::OpenNI::shutdown();

        for(int i = 0; i < numBuffers; i++)
        {
            free(frameBuffers[i].first);
            free(infraredFrameBuffers[i].first);
        }

        delete depthCallback;
        delete infraredCallback;
    }
}

bool OpenNI2Interface::findMode(int x, int y, int fps)
{
    const openni::Array<openni::VideoMode> & depthModes = depthStream.getSensorInfo().getSupportedVideoModes();

    bool found = false;

    for(int i = 0; i < depthModes.getSize(); i++)
    {
        if(depthModes[i].getResolutionX() == x &&
           depthModes[i].getResolutionY() == y &&
           depthModes[i].getFps() == fps)
        {
            found = true;
            break;
        }
    }

    return found;
}

void OpenNI2Interface::printModes()
{
    const openni::Array<openni::VideoMode> & depthModes = depthStream.getSensorInfo().getSupportedVideoModes();

    openni::VideoMode currentDepth = depthStream.getVideoMode();

    std::cout << "Depth Modes: (" << currentDepth.getResolutionX() <<
                                     "x" <<
                                     currentDepth.getResolutionY() <<
                                     " @ " <<
                                     currentDepth.getFps() <<
                                     "fps " <<
                                     formatMap[currentDepth.getPixelFormat()] << ")" << std::endl;

    for(int i = 0; i < depthModes.getSize(); i++)
    {
        std::cout << depthModes[i].getResolutionX() <<
                     "x" <<
                     depthModes[i].getResolutionY() <<
                     " @ " <<
                     depthModes[i].getFps() <<
                     "fps " <<
                     formatMap[depthModes[i].getPixelFormat()] << std::endl;
    }
}
