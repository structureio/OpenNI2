#ifndef _KINECT2_STREAM_IMPL_H_
#define _KINECT2_STREAM_IMPL_H_

#include "BaseKinect2Stream.h"
#include "XnList.h"

struct IKinectSensor;
struct ICoordinateMapper;
struct IFrameDescription;

namespace kinect2_device
{
  class Kinect2StreamImpl
  {
    public:
	    Kinect2StreamImpl(IKinectSensor * pKinectSensor, OniSensorType sensorType);
	    virtual ~Kinect2StreamImpl();

	    void addStream(BaseKinect2Stream* stream);
	    void removeStream(BaseKinect2Stream* stream);	
	    unsigned int getStreamCount();

	    void setVideoMode(OniVideoMode* videoMode);

	    OniStatus virtual start();
	    void virtual stop();
	    bool	isRunning() { return m_running; }

	    OniSensorType getSensorType () { return m_sensorType; }
	    void setSensorType(OniSensorType sensorType);

	    void mainLoop();

      XnDouble getHorizontalFov();
      XnDouble getVerticalFov();

	    OniImageRegistrationMode getImageRegistrationMode() const { return m_imageRegistrationMode; }
	    void setImageRegistrationMode(OniImageRegistrationMode mode) { m_imageRegistrationMode = mode; }

	    IKinectSensor* getKinectSensor() { return m_pKinectSensor; } // Need review: not sure if it is a good idea to expose this.
	    ICoordinateMapper* getCoordinateMapper() { return m_pCoordinateMapper; }

    private:
	    void setDefaultVideoMode();
      IFrameDescription* getFrameDescription(OniSensorType sensorType);

	    static XN_THREAD_PROC threadFunc(XN_THREAD_PARAM pThreadParam);

    private:
	    IKinectSensor* m_pKinectSensor;
      ICoordinateMapper* m_pCoordinateMapper;
	    OniSensorType m_sensorType;
	    OniImageRegistrationMode m_imageRegistrationMode;
	    OniVideoMode m_videoMode;
	    xnl::List<BaseKinect2Stream*> m_streamList;

	    // Thread
	    bool m_running;
	    HANDLE m_hStreamHandle;
	    HANDLE m_hNextFrameEvent;
      unsigned long m_timestamp;
	    XN_THREAD_HANDLE m_threadHandle;	
  };
} // namespace kinect2_device

#endif //_KINECT2_STREAM_IMPL_H_
