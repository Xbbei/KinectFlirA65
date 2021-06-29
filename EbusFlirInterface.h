/*
 * EbusFlirInterface.h
 *
 *  Created on: 28/12 2017
 *      Author: Baobei Xu
 */
#include <PvDevice.h>
#include <PvDeviceGEV.h>
#include <PvDeviceU3V.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvStreamU3V.h>
#include <PvBuffer.h>
#include <PvDeviceFinderWnd.h>
#include <PvGenBrowserWnd.h>
#include <qstring.h>
#include <limits>
#include <cassert>
#include <iostream>

#include "../ThreadMutexObject.h"

#ifndef EBUSFLIRINTERFACE_H_
#define EBUSFLIRINTERFACE_H_

#include <list>
typedef std::list<PvBuffer *> BufferList;

class EbusFlirInterface
{
private:
    /////////////////////////////////////lDevice, lStream and lBufferList
    const PvDeviceInfo *lDeviceInfo;
    PvDevice *lDevice;
    PvStream *lStream ;
    BufferList lBufferList;

    bool initSuccessful; ////////////////Check the connect device successful
    uint32_t lSize; /////////////////////Receive the data size get the device

    /////////////////////////////////////Select device through GUI
    PvResult SelectDevice( PvDeviceFinderWnd *aDeviceFinderWnd );

    /////////////////////////////////////Connect device
    PvResult Connect2Device();

    /////////////////////////////////////Open the stream
    PvResult OpenStream();

    /////////////////////////////////////Config stream and set the destination
    void ConfigureStream();

    /////////////////////////////////////Create PvBuffer
    void CreateStreamBuffers(int BUFFER_COUNT);

    /////////////////////////////////////Free PvBuffer
    void FreeStreamBuffers();

    /////////////////////////////////////Acquire thread
    void AcquireThread();
public:
    /////////////////////////////////////Construct function that conclude that select and connect device,
    /// open and config stream and create pvbuffer.
    EbusFlirInterface(int BUFFER_COUNT = 16, int width = 640, int height = 512);

    /////////////////////////////////////Close the device and release all memory.
    ~EbusFlirInterface();

    /////////////////////////////////////open the acquire thread / close the acquire thread.
    void StartAcquire();
    void StopAcquire();

    /////////////////////////////////////
    ThreadMutexObject<bool> isAcquisition;
    boost::thread * acquisitionThread;

    /////////////////////////////////////My buffer that save the thermal image and timestamp
    static const int numBuffers = 100;
    ThreadMutexObject<int> latestThermalIndex;
    std::pair<uint8_t *, int64_t> frameBuffers[numBuffers];


    /////////////////////////////////////Check the init successful
    bool IsOK();

    /////////////////////////////////////Get Parameters
    PvGenParameterArray *GetCommunicationParameters()
    {
        if ( !IsOK() || !lDevice )
        {
            return NULL;
        }

        return lDevice->GetCommunicationParameters();
    }

    PvGenParameterArray *GetDeviceParameters()
    {
        if ( !IsOK() || !lDevice )
        {
            return NULL;
        }

        return lDevice->GetParameters();
    }

    PvGenParameterArray *GetStreamParameters()
    {
        if ( !lStream || !lStream->IsOpen() )
        {
            return NULL;
        }

        return lStream->GetParameters();
    }

    //////////////////////////////////////Image width and height
    const int width, height;
};


#endif /* EBUSFLIRINTERFACE_H_ */
