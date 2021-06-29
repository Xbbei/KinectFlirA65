/*
 * OpenNI2Interface.h
 *
 *  Created on: 20 May 2013
 *      Author: thomas
 */

#include <OpenNI.h>
#include <PS1080.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "../ThreadMutexObject.h"

#ifndef OPENNI2INTERFACE_H_
#define OPENNI2INTERFACE_H_


class OpenNI2Interface
{
    public:
        OpenNI2Interface(int inWidth = 512, int inHeight = 424, int fps = 30);
        virtual ~OpenNI2Interface();

        const int width, height, fps;

        void printModes();
        bool findMode(int x, int y, int fps);

        bool ok()
        {
            return initSuccessful;
        }

        std::string error()
        {
            errorText.erase(std::remove_if(errorText.begin(), errorText.end(), &OpenNI2Interface::isTab), errorText.end());
            return errorText;
        }

        static const int numBuffers = 100;
        ThreadMutexObject<int> latestDepthIndex;
        std::pair<uint8_t *, int64_t> frameBuffers[numBuffers];
        ThreadMutexObject<int> latestInfraredIndex;
        std::pair<uint8_t *, int64_t> infraredFrameBuffers[numBuffers];

        class DepthCallback : public openni::VideoStream::NewFrameListener
        {
            public:
                DepthCallback(int64_t & lastDepthTime,
                              ThreadMutexObject<int> & latestDepthIndex,
                              std::pair<uint8_t *, int64_t> * frameBuffers)
                 : lastDepthTime(lastDepthTime),
                   latestDepthIndex(latestDepthIndex),
                   frameBuffers(frameBuffers)
                {}

                void onNewFrame(openni::VideoStream& stream)
                {
                    stream.readFrame(&frame);

                    boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
                    boost::posix_time::time_duration duration(time.time_of_day());
                    lastDepthTime = duration.total_microseconds();

                    int bufferIndex = (latestDepthIndex.getValue() + 1) % numBuffers;

                    memcpy(frameBuffers[bufferIndex].first, frame.getData(), frame.getWidth() * frame.getHeight() * 2);

                    frameBuffers[bufferIndex].second = lastDepthTime;

                    latestDepthIndex++;
                }

            private:
                openni::VideoFrameRef frame;
                int64_t & lastDepthTime;
                ThreadMutexObject<int> & latestDepthIndex;

                std::pair<uint8_t *, int64_t> * frameBuffers;
        };

        class InfraredCallback : public openni::VideoStream::NewFrameListener
        {
            public:
                InfraredCallback(int64_t & lastInfraredTime,
                                 ThreadMutexObject<int> & latestInfraredIndex,
                                 std::pair<uint8_t *, int64_t> * infraredFrameBuffers)
                 : lastInfraredTime(lastInfraredTime),
                   latestInfraredIndex(latestInfraredIndex),
                   infraredFrameBuffers(infraredFrameBuffers)
                {}

                void onNewFrame(openni::VideoStream& stream)
                {
                    stream.readFrame(&frame);

                    boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
                    boost::posix_time::time_duration duration(time.time_of_day());
                    lastInfraredTime = duration.total_microseconds();

                    int bufferIndex = (latestInfraredIndex.getValue() + 1) % numBuffers;

                    memcpy(infraredFrameBuffers[bufferIndex].first, frame.getData(), frame.getWidth() * frame.getHeight());

                    infraredFrameBuffers[bufferIndex].second = lastInfraredTime;

                    latestInfraredIndex++;
                }

            private:
                openni::VideoFrameRef frame;
                int64_t & lastInfraredTime;
                ThreadMutexObject<int> & latestInfraredIndex;

                std::pair<uint8_t *, int64_t> * infraredFrameBuffers;
        };

    private:
        openni::Device device;

        openni::VideoStream depthStream;
        openni::VideoStream infraredStream;

        //Map for formats from OpenNI2
        std::map<int, std::string> formatMap;

        int64_t lastDepthTime;
        int64_t lastInfraredTime;

        DepthCallback * depthCallback;
        InfraredCallback * infraredCallback;

        bool initSuccessful;
        std::string errorText;

        //For removing tabs from OpenNI's error messages
        static bool isTab(char c)
        {
            switch(c)
            {
                case '\t':
                    return true;
                default:
                    return false;
            }
        }
};

#endif /* OPENNI2INTERFACE_H_ */
