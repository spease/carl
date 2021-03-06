#include "Camera.h"

#include <linux/limits.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char const * const DEVICE_PATH_PRINTF="/dev/video%d";
static uint32_t const DEVICE_FIELD = V4L2_FIELD_NONE;
static uint32_t const DEVICE_BUFFER_COUNT = 2;
static enum v4l2_priority const DEVICE_PRIORITY = V4L2_PRIORITY_RECORD;

/********************----- STRUCT: Buffer -----********************/
struct Buffer_s
{
	void *m_start;
	size_t m_sizeBytes;
};
typedef struct Buffer_s Buffer;
/**************************************************/

/********************----- STRUCT: Camera -----********************/
struct Camera_s
{
	Buffer *m_buffers;
	size_t m_bufferCount;
	size_t m_bufferCountMax;
	int m_deviceHandle;
	struct v4l2_format m_format;
	struct v4l2_captureparm m_parameters;
	enum v4l2_priority m_priority;
};
/**************************************************/

static inline int xioctl(int const i_fileHandle, int const i_request, void * const i_argument)
{
	int ioResult = -1;
	do
	{
		ioResult = ioctl(i_fileHandle, i_request, i_argument);
	} while(-1 == ioResult && EINTR == errno);

	return ioResult;
}

struct camera_capture_data_t
{
	size_t m_outputSizeBytesMax;
	uint8_t * m_outputBuffer;
};

static void camera_capture_data_callback(uint8_t const * const i_frameData, size_t const i_frameSizeBytes, void * const i_callbackData)
{
	struct camera_capture_data_t const * const capData = ((struct camera_capture_data_t*)i_callbackData);

	if(capData->m_outputBuffer != NULL && capData->m_outputSizeBytesMax > 0)
	{
		memcpy(capData->m_outputBuffer, i_frameData, MIN(i_frameSizeBytes, capData->m_outputSizeBytesMax));
	}
}

Result camera_capture_copy(size_t const i_outputSizeBytesMax, uint8_t * const o_outputBuffer, Camera * const io_cameraHandle)
{
	struct camera_capture_data_t capData;
	capData.m_outputSizeBytesMax = i_outputSizeBytesMax;
	capData.m_outputBuffer = o_outputBuffer;

	return camera_capture_callback(camera_capture_data_callback, (void*)(&capData), io_cameraHandle);
}

Result camera_capture_callback(CameraCallback i_callback, void *i_callbackData, Camera * const io_cameraHandle)
{
	fd_set fds;
	struct timeval tv;
	int selectResult = -1;
	Result result = R_FAILURE;
	struct v4l2_buffer buffer;
	int xioResult=-1;

	if(io_cameraHandle == NULL)
	{
		CARL_ERROR("Camera not created.");

		result = R_OBJECTNOTEXTANT;
		goto end;
	}

	/***** Dequeue Buffer *****/
	for(;;)
	{
		/*
		do
		{
			FD_ZERO(&fds);
			FD_SET(io_cameraHandle->m_deviceHandle, &fds);
			tv.tv_usec = 200;

			selectResult = select(io_cameraHandle->m_deviceHandle+1, &fds, NULL, NULL, &tv);
		} while((selectResult == -1 && (errno == EINTR)));
		if(selectResult == -1)
		{
			fprintf(stderr, "party foul");
			goto end;
		}
		*/

		CLEAR(buffer);
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

		errno = 0;
		xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_DQBUF, &buffer);
		if(xioResult != -1)
		{
			break;
		}
		else if(errno == EAGAIN)
		{
			continue;
		}
		else
		{
			CARL_ERROR("Unable to dequeue buffer - \"%s\"", strerror(errno));

			result = R_BUFFERDEQUEUEFAILED;
			goto end;
		}
	}

	/***** Copy the data *****/
	if(i_callback != NULL)
	{
		i_callback(io_cameraHandle->m_buffers[buffer.index].m_start, buffer.bytesused, i_callbackData);
	}

	/***** Requeue buffer *****/
	xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_QBUF, &buffer);
	if(xioResult == -1)
	{
		CARL_ERROR("Unable to requeue buffer - \"%s\"", strerror(errno));

		result = R_BUFFERENQUEUEFAILED;
		goto end;
	}

end:
	return result;
}
	

Result camera_create(int32_t const i_deviceID, PixelFormat const i_pixelFormat, uint32_t const i_sizeX, uint32_t const i_sizeY, Camera ** const o_cameraHandle)
{
	struct v4l2_buffer buffer;
	size_t bufferIndex=0;
	void *bufferMap=NULL;
	struct v4l2_requestbuffers bufferRequest;
	Camera *cameraHandle = NULL;
	struct v4l2_capability cap;
	struct v4l2_control currentControl;
	char devicePathname[PATH_MAX];
	uint32_t devicePixelFormat = 0;
	uint32_t deviceSizeX = 0;
	uint32_t deviceSizeY = 0;
	Result result=R_FAILURE;
	int xioResult=-1;

	/***** Input Validation *****/
	if(i_sizeX == 0 || i_sizeY == 0)
	{
		CARL_ERROR("Frame size must be non-0 for both dimensions.");

		result = R_INPUTBAD;
		goto end;
	}

	/***** Create camera structure *****/
	cameraHandle = (Camera*) malloc(sizeof(Camera));
	if(cameraHandle == NULL)
	{
		CARL_ERROR("Unable to allocate memory.");

		result = R_MEMORYALLOCATIONERROR;
		goto end;
	}

	/***** Clear fields *****/
	cameraHandle->m_buffers = NULL;
	cameraHandle->m_bufferCount = 0;
	cameraHandle->m_bufferCountMax = 0;
	cameraHandle->m_deviceHandle = -1;
	CLEAR(cameraHandle->m_format);

	/***** Generate camera path *****/
	if(i_deviceID < 0)
	{
		CARL_ERROR("Device ID must be > 0.");

		result = R_INPUTBAD;
		goto end;
	}
	snprintf(devicePathname, sizeof(devicePathname), DEVICE_PATH_PRINTF, i_deviceID);

	/***** Open camera device *****/
	cameraHandle->m_deviceHandle = open(devicePathname, O_RDWR | O_NONBLOCK, 0);
	if(cameraHandle->m_deviceHandle < 0)
	{
		CARL_ERROR("Unable to open device - \"%s\"", strerror(errno));

		result = R_DEVICEOPENFAILED;
		goto end;
	}

	/***** Query capabilities *****/
	CLEAR(cap);
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_QUERYCAP, &cap);
	if(xioResult == -1)
	{
		CARL_ERROR("Not a video capture device - \"%s\"", strerror(errno));

		result = R_DEVICEINVALID;
		goto end;
	}
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		CARL_ERROR("Device cannot capture video.");

		result = R_DEVICENOVIDEOCAP;
		goto end;
	}
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		CARL_ERROR("Device cannot stream.");

		result = R_DEVICENOSTREAMCAP;
		goto end;
	}

	/***** Prepare user desires *****/
	deviceSizeX = i_sizeX;
	deviceSizeY = i_sizeY;
	switch(i_pixelFormat)
	{
		case CAMERA_PIXELFORMAT_MJPEG:
			devicePixelFormat = V4L2_PIX_FMT_MJPEG;
			break;
		case CAMERA_PIXELFORMAT_UYVY:
			devicePixelFormat = V4L2_PIX_FMT_UYVY;
			break;
		case CAMERA_PIXELFORMAT_YUYV:
			devicePixelFormat = V4L2_PIX_FMT_YUYV;
			break;
		default:
			CARL_ERROR("Unsupported pixel format %d specified", i_pixelFormat);

			result = R_INPUTBAD;
			goto end;
	}

	/***** Stuff user desires into struct *****/
	CLEAR(cameraHandle->m_format);
	cameraHandle->m_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cameraHandle->m_format.fmt.pix.width = i_sizeX;
	cameraHandle->m_format.fmt.pix.height = i_sizeY;
	cameraHandle->m_format.fmt.pix.pixelformat = devicePixelFormat;
	cameraHandle->m_format.fmt.pix.field = DEVICE_FIELD;

	/***** Apply camera attributes *****/
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_S_FMT, &(cameraHandle->m_format));
	if(xioResult == -1)
	{
		CARL_ERROR("Attribute application failed - \"%s\"", strerror(errno));

		result = R_DEVICEATTRIBUTESETFAILED;
		goto end;
	}

	/***** Check camera attributes *****/
	if(cameraHandle->m_format.fmt.pix.pixelformat != devicePixelFormat)
	{
		CARL_ERROR("Driver set different pixel format (%u)", cameraHandle->m_format.fmt.pix.pixelformat);

		result = R_DEVICEPIXELFORMATFAILED;
		goto end;
	}
	if(cameraHandle->m_format.fmt.pix.width != deviceSizeX || cameraHandle->m_format.fmt.pix.height != deviceSizeY)
	{
		CARL_ERROR("Driver set different resolution (%u x %u)", cameraHandle->m_format.fmt.pix.width, cameraHandle->m_format.fmt.pix.height);

		result = R_DEVICERESOLUTIONFAILED;
		goto end;
	}

	/***** Apply camera stream parameters *****/
	/*
	CLEAR(cameraHandle->m_parameters);
	cameraHandle->m_parameters.capturemode |= V4L2_MODE_HIGHQUALITY;
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_S_PARM, &(cameraHandle->m_parameters));
	if(xioResult == -1)
	{
		fprintf(stderr, "%s: camera_create() - Parameter application failed - %s\n", g_programName, strerror(errno));

		result = R_DEVICEPARAMETERSETFAILED;
		goto end;
	}
	if(!(cameraHandle->m_parameters.capturemode & V4L2_MODE_HIGHQUALITY))
	{
		fprintf(stderr, "%s: camera_create() - Driver did not set high quality mode\n", g_programName);

		result = R_DEVICEHIGHQUALITYFAILED;
		goto end;
	}
	*/

	/***** Apply camera priority *****/
	/*
	cameraHandle->m_priority = DEVICE_PRIORITY;
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_S_PRIORITY, &cameraHandle->m_priority);
	if(xioResult == -1)
	{
		if(errno == EBUSY)
		{
			fprintf(stderr, "%s: camera_create() - Another file handle alreaddy has priority - %s\n", g_programName, strerror(errno));
		}
		else
		{
			fprintf(stderr, "%s: camera_create() - Priority application failed - %s\n", g_programName, strerror(errno));
		}
		result = R_DEVICEPRIORITYSETFAILED;
		goto end;
	}
	if(cameraHandle->m_priority != DEVICE_PRIORITY)
	{
		fprintf(stderr, "%s: camera_create() - Driver set priority %d\n", g_programName, cameraHandle->m_priority);

		result = R_DEVICEPRIORITYFAILED;
		goto end;
	}
	*/

	/***** Apply camera controls *****/
#if 0
	CLEAR(currentControl);
	currentControl.id = V4L2_CID_EXPOSURE_AUTO;
	currentControl.value = V4L2_EXPOSURE_MANUAL;
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_S_CTRL, &currentControl);
	if(xioResult == -1)
	{
		CARL_ERROR("Unable to set shutter priority - \"%s\"", strerror(errno));

		result = R_DEVICECONTROLSETFAILED;
		goto end;
	}
	CLEAR(currentControl);
	currentControl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	currentControl.value = 10000;
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_S_CTRL, &currentControl);
	if(xioResult == -1)
	{
		CARL_ERROR("Unable to set exposure - \"%s\"", strerror(errno));

		result = R_DEVICECONTROLSETFAILED;
		goto end;
	}
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_G_CTRL, &currentControl);
	if(xioResult == -1)
	{
		CARL_ERROR("Unable to get exposure - \"%s\"", strerror(errno));

		result = R_DEVICECONTROLSETFAILED;
		goto end;
	}
	if(currentControl.value != 9999)
	{
		fprintf(stderr, "lolnuts");
		result = R_FAILURE;
		goto end;
	}
#endif

	/***** Setup buffer request *****/
	CLEAR(bufferRequest);
	bufferRequest.count = DEVICE_BUFFER_COUNT;
	bufferRequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufferRequest.memory = V4L2_MEMORY_MMAP;

	/***** Request buffers *****/
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_REQBUFS, &bufferRequest);
	if(xioResult == -1)
	{
		CARL_ERROR("Buffer request failed - \"%s\"", strerror(errno));

		result = R_BUFFERREQUESTFAILED;
		goto end;
	}

	/***** Create storage for buffer info *****/
	cameraHandle->m_buffers = calloc(bufferRequest.count, sizeof(*(cameraHandle->m_buffers)));
	if(cameraHandle->m_buffers == NULL)
	{
		CARL_ERROR("Unable to allocate memory for buffer pointers.");

		result = R_MEMORYALLOCATIONERROR;
		goto end;
	}
	cameraHandle->m_bufferCountMax = bufferRequest.count;

	/***** Setup each buffer *****/
	for(bufferIndex=0; bufferIndex<bufferRequest.count; ++bufferIndex)
	{
		CLEAR(buffer);
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = cameraHandle->m_bufferCount;
		cameraHandle->m_buffers[bufferIndex].m_start = NULL;
		cameraHandle->m_buffers[bufferIndex].m_sizeBytes = 0;

		/***** Save buffer info *****/
		xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_QUERYBUF, &buffer);
		if(xioResult == -1)
		{
			CARL_ERROR("Buffer query failed - \"%s\"", strerror(errno));

			result = R_BUFFERQUERYFAILED;
			goto end;
		}

		/***** Map buffer into application space *****/
		bufferMap = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraHandle->m_deviceHandle, buffer.m.offset);
		if(MAP_FAILED == bufferMap)
		{
			CARL_ERROR("Buffer map failed - \"%s\"", strerror(errno));

			result = R_BUFFERMAPFAILED;
			goto end;
		}

		/***** Set data *****/
		cameraHandle->m_buffers[bufferIndex].m_start = bufferMap;
		cameraHandle->m_buffers[bufferIndex].m_sizeBytes = buffer.length;
		++cameraHandle->m_bufferCount;
	}

	/***** Queue buffers *****/
	for(bufferIndex=0; bufferIndex<cameraHandle->m_bufferCount; ++bufferIndex)
	{
		CLEAR(buffer);
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = bufferIndex;

		xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_QBUF, &buffer);
		if(xioResult == -1)
		{
			CARL_ERROR("Buffer queue failed - \"%s\"", strerror(errno));

			result = R_BUFFERENQUEUEFAILED;
			goto end;
		}
	}

	/***** Set output *****/
	if(o_cameraHandle != NULL)
	{
		(*o_cameraHandle) = cameraHandle;
	}
	else
	{
		camera_destroy(&cameraHandle);
	}

	/***** Return *****/
	return R_SUCCESS;

end:
	CARL_ERROR("camera_create(%d, %d, %u, %u, %p)", i_deviceID, i_pixelFormat, i_sizeX, i_sizeY, o_cameraHandle);
	camera_destroy(&cameraHandle);

	return result;
};

Result camera_destroy(Camera **const io_cameraHandle)
{
	size_t bufferIndex=0;
	Camera *cameraHandle = NULL;

	/***** Input Validation *****/
	if(io_cameraHandle == NULL)
	{
		CARL_ERROR("Received NULL pointer");
		return R_INPUTBAD;
	}

	/***** Is it a valid camera? *****/
	cameraHandle = (*io_cameraHandle);
	if(cameraHandle == NULL)
	{
		CARL_ERROR("Camera handle already destroyed");
		return R_OBJECTNOTEXTANT;
	}

	/***** Free the buffers *****/
	if(cameraHandle->m_buffers != NULL)
	{
		while(cameraHandle->m_bufferCount > 0)
		{
			bufferIndex = cameraHandle->m_bufferCount-1;
			munmap(cameraHandle->m_buffers[bufferIndex].m_start, cameraHandle->m_buffers[bufferIndex].m_sizeBytes);
			--(cameraHandle->m_bufferCount);
		}
		free(cameraHandle->m_buffers);
		cameraHandle->m_buffers = NULL;
	}

	/***** Release device handle *****/
	if(cameraHandle->m_deviceHandle >= 0)
	{
		close(cameraHandle->m_deviceHandle);
		cameraHandle->m_deviceHandle = -1;
	}

	/***** Free camera structure *****/
	free(cameraHandle);
	(*io_cameraHandle) = NULL;

	/***** Return ****/
	return R_SUCCESS;
}

Result camera_start(Camera *const io_cameraHandle)
{
	int xioResult = -1;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/***** Start capturing *****/
	xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_STREAMON, &type);
	if(xioResult == -1)
	{
		CARL_ERROR("xioctl - \"%s\"", strerror(errno));

		return R_DEVICESTARTFAILED;
	}

	return R_SUCCESS;
}

Result camera_stop(Camera * const io_cameraHandle)
{
	int xioResult = -1;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/***** Stop capturing *****/
	xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_STREAMOFF, &type);
	if(xioResult == -1)
	{
		CARL_ERROR("xioctl - \"%s\"", strerror(errno));

		return R_DEVICESTOPFAILED;
	}

	return R_SUCCESS;
}

