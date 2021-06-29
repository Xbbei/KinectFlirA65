#include "EbusFlirInterface.h"
#include <libusb.h>

using namespace std;

EbusFlirInterface::EbusFlirInterface(int BUFFER_COUNT, int width, int height)
    :lDeviceInfo(NULL),
     lDevice(NULL),
     lStream(NULL),
     initSuccessful(false),
     lSize(0),
     acquisitionThread(0),
     width(width),
     height(height)
{
    cout << libusb_get_version()->nano << endl;
    isAcquisition.assignValue(false);

    PvDeviceFinderWnd *lDeviceFinderWnd = new PvDeviceFinderWnd();
    PvResult lResult = SelectDevice(lDeviceFinderWnd);
    if( NULL != lDeviceFinderWnd )
    {
        delete lDeviceFinderWnd;
        lDeviceFinderWnd = NULL;
    }

    if(lResult.IsOK())
    {
        lResult = Connect2Device();
        if(lResult.IsOK())
        {
            lResult = OpenStream();
            if(lResult.IsOK())
            {
                ConfigureStream();
                CreateStreamBuffers(BUFFER_COUNT);
                initSuccessful = true;
                latestThermalIndex.assignValue(-1);
            }
        }
    }

    if(initSuccessful)
    {
        for(int i = 0; i < numBuffers; i++)
        {
            uint8_t * newThermal = (uint8_t *)malloc(lSize);
            frameBuffers[i] = std::pair<uint8_t *, int64_t>(newThermal, 0);
        }
    }
}

void EbusFlirInterface::StartAcquire()
{
    if(!initSuccessful)
        return;
    if(acquisitionThread || isAcquisition.getValue())
        return;
    PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
    PvGenCommand *lTimestampRest = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "GevTimestampControlReset" ) );
    lTimestampRest->Execute();
    isAcquisition.assignValue(true);
    BufferList::iterator lIt = lBufferList.begin();
    while ( lIt != lBufferList.end() )
    {
        lStream->QueueBuffer( *lIt );
        lIt++;
    }
    acquisitionThread = new boost::thread(boost::bind(&EbusFlirInterface::AcquireThread,
                                                        this));
}

void EbusFlirInterface::StopAcquire()
{
    if(!acquisitionThread || !isAcquisition.getValue())
        return;
    isAcquisition.assignValue(false);
    acquisitionThread->join();
    delete acquisitionThread;
    acquisitionThread = 0;
}

EbusFlirInterface::~EbusFlirInterface()
{
    if(!initSuccessful)
        return;
    StopAcquire();
    FreeStreamBuffers();
    // Close the stream
    cout << "Closing stream" << endl;
    lStream->Close();
    PvStream::Free( lStream );
    // Disconnect the device
    cout << "Disconnecting device" << endl;
    lDevice->Disconnect();
    PvDevice::Free( lDevice );
    if(initSuccessful)
    {
        for(int i = 0; i < numBuffers; i++)
        {
            free(frameBuffers[i].first);
        }
    }
}

PvResult EbusFlirInterface::SelectDevice( PvDeviceFinderWnd *aDeviceFinderWnd )
{
    PvResult lResult;
    if (NULL != aDeviceFinderWnd)
    {
        // Display list of GigE Vision and USB3 Vision devices
        lResult = aDeviceFinderWnd->ShowModal();
        if ( !lResult.IsOK() )
        {
            // User hit cancel
            cout << "No device selected." << endl;
            return lResult;
        }

        // Get the selected device information.
        lDeviceInfo = aDeviceFinderWnd->GetSelected();
    }
    return lResult;
}


PvResult EbusFlirInterface::Connect2Device()
{
    PvResult lResult;

    // Connect to the GigE Vision or USB3 Vision device
    cout << "Connecting to " << lDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    lDevice = PvDevice::CreateAndConnect( lDeviceInfo, &lResult );
    if ( lDevice == NULL )
    {
        cout << "Unable to connect to " << lDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    }

    return lResult;
}


PvResult EbusFlirInterface::OpenStream()
{
    PvResult lResult;

    // Open stream to the GigE Vision or USB3 Vision device
    cout << "Opening stream to device." << endl;
    lStream = PvStream::CreateAndOpen( lDeviceInfo->GetConnectionID(), &lResult );
    if ( lStream == NULL )
    {
        cout << "Unable to stream from " << lDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    }

    return lResult;
}


void EbusFlirInterface::ConfigureStream()
{
    // If this is a GigE Vision device, configure GigE Vision specific streaming parameters
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( lDevice );
    if ( lDeviceGEV != NULL )
    {
        PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( lStream );

        // Negotiate packet size
        lDeviceGEV->NegotiatePacketSize();

        // Configure device streaming destination
        lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
    }
}

void EbusFlirInterface::CreateStreamBuffers(int BUFFER_COUNT)
{
    // Reading payload size from device
    lSize = lDevice->GetPayloadSize();

    // Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
    uint32_t lBufferCount = ( lStream->GetQueuedBufferMaximum() < BUFFER_COUNT ) ?
        lStream->GetQueuedBufferMaximum() :
        BUFFER_COUNT;

    // Allocate buffers
    for ( uint32_t i = 0; i < lBufferCount; i++ )
    {
        // Create new buffer object
        PvBuffer *lBuffer = new PvBuffer;

        // Have the new buffer object allocate payload memory
        lBuffer->Alloc( static_cast<uint32_t>( lSize ) );

        // Add to external list - used to eventually release the buffers
        lBufferList.push_back( lBuffer );
    }
}

void EbusFlirInterface::FreeStreamBuffers()
{
    // Go through the buffer list
    BufferList::iterator lIt = lBufferList.begin();
    while ( lIt != lBufferList.end() )
    {
        delete *lIt;
        lIt++;
    }

    // Clear the buffer list
    lBufferList.clear();
}

void EbusFlirInterface::AcquireThread()
{
    // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = lDevice->GetParameters();

    // Map the GenICam AcquisitionStart and AcquisitionStop commands
    PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
    PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );

    // Enable streaming and send the AcquisitionStart command
    cout << "Enabling streaming and sending AcquisitionStart command." << endl;
    lDevice->StreamEnable();
    lStart->Execute();

    // Acquire images until the user instructs us to stop.
    while ( isAcquisition.getValue() )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        // Retrieve next buffer
        PvResult lResult = lStream->RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
        if ( lResult.IsOK() )
        {
            if ( lOperationResult.IsOK() )
            {
                PvPayloadType lType;

                lType = lBuffer->GetPayloadType();

                if ( lType == PvPayloadTypeImage )
                {
                    // Get buffer pointer interface.
                    int bufferIndex = (latestThermalIndex.getValue() + 1) % numBuffers;
                    memcpy(frameBuffers[bufferIndex].first, lBuffer->GetDataPointer(), lSize);
                    boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
                    boost::posix_time::time_duration duration(time.time_of_day());
//                    frameBuffers[bufferIndex].second = float(lBuffer->GetTimestamp()) / 69513.0 * 33365.0;
                    frameBuffers[bufferIndex].second = duration.total_microseconds();
                    latestThermalIndex++;
                }
                else
                {
                    cout << " (buffer does not contain image)";
                }

            }
            else
            {
                // Non OK operational result
                cout << lOperationResult.GetCodeString().GetAscii() << "\n";
            }

            // Re-queue the buffer in the stream object
            lStream->QueueBuffer( lBuffer );
        }
        else
        {
            // Retrieve buffer failure
            cout << lResult.GetCodeString().GetAscii() << "\n";
        }
    }

    // Tell the device to stop sending images.
    cout << "Sending AcquisitionStop command to the device" << endl;
    lStop->Execute();

    // Disable streaming on the device
    cout << "Disable streaming on the controller." << endl;
    lDevice->StreamDisable();

    // Abort all buffers from the stream and dequeue
    cout << "Aborting buffers still in stream" << endl;
    lStream->AbortQueuedBuffers();
    while ( lStream->GetQueuedBufferCount() > 0 )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        lStream->RetrieveBuffer( &lBuffer, &lOperationResult );
    }
}

bool EbusFlirInterface::IsOK()
{
    return initSuccessful;
}
