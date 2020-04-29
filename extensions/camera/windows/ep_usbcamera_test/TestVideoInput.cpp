// TestvideoInput.cpp
//

#include <Windows.h>
#include <Tchar.h>
#include "videoInput.h"
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/highgui.hpp"
 
void StopEvent(int deviceID, void *userData)
{
    videoInput *VI = &videoInput::getInstance();
 
    VI->closeDevice(deviceID);
}
 
int main(int argc, _TCHAR* argv[])
{
    videoInput *VI = &videoInput::getInstance();
 
    int i = VI->listDevices();
 
    if(i > 0)
    {
        if(VI->setupDevice(i-1, 640, 480, 60))
        {
            VI->setEmergencyStopEvent(i - 1, NULL, StopEvent);
 
            if(VI->isFrameNew(i-1))
            {
                int countLeftFrames = 0;
 
                cvNamedWindow("VideoTest", CV_WINDOW_AUTOSIZE);
                CvSize size = cvSize(VI->getWidth(i-1), VI->getHeight(i-1));
 
                IplImage* frame;
 
                frame = cvCreateImage(size, 8,3);
 
                while(1)
                {
                    if(VI->isFrameNew(i-1))
                    {
                        VI->getPixels(i - 1, (unsigned char *)frame->imageData);                        
 
                        cvShowImage("VideoTest", frame);
 
                        countLeftFrames = 0;
                    }
                    else
                        countLeftFrames++;
 
                    char c = cvWaitKey(33);
 
                    if(c == 27) 
                        break;
                    
                    if(c == 49) 
                    {
                        CamParametrs CP = VI->getParametrs(i-1);                        
                        CP.Brightness.CurrentValue = 128; 
                        CP.Brightness.Flag = 1; 
                        VI->setParametrs(i - 1, CP);
                    }
 
                    if(!VI->isDeviceSetup(i - 1))
                    {
                        break;
                    }
 
                    if(countLeftFrames > 60)
                        break;
                }
 
                VI->closeDevice(i - 1);
                
                cvDestroyWindow("VideoTest");
            }
        }
    }
 
    if(VI->setupDevice(i-1, 1920, 1080, 60))
    {
        if(VI->isFrameNew(i-1))
        {
            int countLeftFrames = 0;
 
            cvNamedWindow("VideoTest1", CV_WINDOW_AUTOSIZE);
            CvSize size = cvSize(VI->getWidth(i-1), VI->getHeight(i-1));
 
            IplImage* frame;
 
            frame = cvCreateImage(size, 8,3);
 
            while(1)
            {
                if(VI->isFrameNew(i-1))
                {
                    VI->getPixels(i - 1, (unsigned char *)frame->imageData,false); 
                    cvShowImage("VideoTest1", frame); 
                    countLeftFrames = 0;
                }
                else
                    countLeftFrames++;
                    
                char c = cvWaitKey(33);
 
                if(c == 27) 
                    break;
                    
                if(!VI->isDeviceSetup(i - 1))
                {
                    break;
                }
 
                if(countLeftFrames > 60)
                    break;
            }
 
            VI->closeDevice(i - 1);
                
            cvDestroyWindow("VideoTest1");
        }
 
    }
    return 0;
}