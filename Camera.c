#include "Camera.h"

#include <linux/limits.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char const * const DEVICE_PATH_PRINTF="/dev/video%d";
static uint32_t const DEVICE_FIELD = V4L2_FIELD_NONE;

/********************----- STRUCT: Buffer -----********************/
struct Buffer_s
{
	void *m_start;
	size_t m_sizeBytes;
};
typedef struct Buffer_s Buffer;
/**************************************************/

/********************----- STRUCT: CameraHandle -----********************/
struct CameraHandle_s
{
	Buffer *m_buffers;
	size_t m_bufferCount;
	size_t m_bufferCountMax;
	int m_deviceHandle;
	struct v4l2_format m_format;
};
typedef struct CameraHandle_s CameraHandle;
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

Result camera_capture_copy(size_t const i_outputSizeBytesMax, uint8_t * const o_outputBuffer, CameraHandle * const io_cameraHandle)
{
	struct camera_capture_data_t capData;
	capData.m_outputSizeBytesMax = i_outputSizeBytesMax;
	capData.m_outputBuffer = o_outputBuffer;

	return camera_capture_callback(camera_capture_data_callback, (void*)(&capData), io_cameraHandle);
}

Result camera_capture_callback(CameraCallback i_callback, void *i_callbackData, CameraHandle * const io_cameraHandle)
{
	fd_set fds;
	struct timeval tv;
	int selectResult = -1;
	Result result = R_FAILURE;
	struct v4l2_buffer buffer;
	int xioResult=-1;

	if(io_cameraHandle == NULL)
	{
		fprintf(stderr, "%s: camera_capture() - Camera not created.\n", g_programName);

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
			fprintf(stderr, "%s: camera_capture() - Unable to dequeue buffer - %s\n", g_programName, io_cameraHandle, strerror(errno)); 

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
		fprintf(stderr, "%s: camera_capture() - Unable to dequeue buffer - %s\n", g_programName, io_cameraHandle, strerror(errno));

		result = R_BUFFERENQUEUEFAILED;
		goto end;
	}

end:
	return result;
}
	

Result camera_create(int32_t const i_deviceID, PixelFormat const i_pixelFormat, uint32_t const i_sizeX, uint32_t const i_sizeY, CameraHandle ** const o_cameraHandle)
{
	struct v4l2_buffer buffer;
	size_t bufferIndex=0;
	void *bufferMap=NULL;
	struct v4l2_requestbuffers bufferRequest;
	CameraHandle *cameraHandle = NULL;
	struct v4l2_capability cap;
	char devicePathname[PATH_MAX];
	uint32_t devicePixelFormat = 0;
	uint32_t deviceSizeX = 0;
	uint32_t deviceSizeY = 0;
	Result result=R_FAILURE;
	int xioResult=-1;

	/***** Input Validation *****/
	if(i_sizeX == 0 || i_sizeY == 0)
	{
		fprintf(stderr, "%s: camera_create() - Frame size must be non-0 for both dimensions.\n", g_programName);

		result = R_INPUTBAD;
		goto end;
	}

	/***** Create camera structure *****/
	cameraHandle = (CameraHandle*) malloc(sizeof(CameraHandle));
	if(cameraHandle == NULL)
	{
		fprintf(stderr, "%s: camera_create() - Unable to allocate memory.\n", g_programName);
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
		fprintf(stderr, "%s: camera_create() - Device ID must be > 0.\n", g_programName);

		result = R_INPUTBAD;
		goto end;
	}
	snprintf(devicePathname, sizeof(devicePathname), DEVICE_PATH_PRINTF, i_deviceID);

	/***** Open camera device *****/
	cameraHandle->m_deviceHandle = open(devicePathname, O_RDWR | O_NONBLOCK, 0);
	if(cameraHandle->m_deviceHandle < 0)
	{
		fprintf(stderr, "%s: camera_create() - Unable to open device - %s\n", g_programName, strerror(errno));

		result = R_DEVICEOPENFAILED;
		goto end;
	}

	/***** Query capabilities *****/
	CLEAR(cap);
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_QUERYCAP, &cap);
	if(xioResult == -1)
	{
		fprintf(stderr, "%s: camera_create(%p) - Not a video capture device - %s", g_programName, o_cameraHandle, strerror(errno));

		result = R_DEVICEINVALID;
		goto end;
	}
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s: camera_create(%p) - Device cannot capture video", g_programName, o_cameraHandle, strerror(errno));

		result = R_DEVICENOVIDEOCAP;
		goto end;
	}
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		fprintf(stderr, "%s: camera_create(%p) - Device cannot stream", g_programName, o_cameraHandle, strerror(errno));

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
			fprintf(stderr, "%s: camera_create() - Unsupported pixel format %d specified", i_pixelFormat);

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
		fprintf(stderr, "%s: camera_create() - Attribute application failed - %s\n", g_programName, strerror(errno));

		result = R_DEVICEATTRIBUTESETFAILED;
		goto end;
	}

	/***** Check camera attributes *****/
	if(cameraHandle->m_format.fmt.pix.pixelformat != devicePixelFormat)
	{
		fprintf(stderr, "%s: camera_create() - Driver set different pixel format (%u)\n", g_programName, cameraHandle->m_format.fmt.pix.pixelformat);

		result = R_DEVICEPIXELFORMATFAILED;
		goto end;
	}
	if(cameraHandle->m_format.fmt.pix.width != deviceSizeX || cameraHandle->m_format.fmt.pix.height != deviceSizeY)
	{
		fprintf(stderr, "%s: camera_create() - Driver set different resolution (%u x %u)\n", g_programName, cameraHandle->m_format.fmt.pix.width, cameraHandle->m_format.fmt.pix.height);

		result = R_DEVICERESOLUTIONFAILED;
		goto end;
	}

	/***** Setup buffer request *****/
	CLEAR(bufferRequest);
	bufferRequest.count = 4;
	bufferRequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufferRequest.memory = V4L2_MEMORY_MMAP;

	/***** Request buffers *****/
	xioResult = xioctl(cameraHandle->m_deviceHandle, VIDIOC_REQBUFS, &bufferRequest);
	if(xioResult == -1)
	{
		fprintf(stderr, "%s: camera_create() - Buffer request failed - %s\n", g_programName, strerror(errno));

		result = R_BUFFERREQUESTFAILED;
		goto end;
	}

	/***** Create storage for buffer info *****/
	cameraHandle->m_buffers = calloc(bufferRequest.count, sizeof(*(cameraHandle->m_buffers)));
	if(cameraHandle->m_buffers == NULL)
	{
		fprintf(stderr, "%s: camera_create() - Unable to allocate memory for buffer pointers\n", g_programName);

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
			fprintf(stderr, "%s: camera_create() - Buffer query failed - %s\n", g_programName, strerror(errno));

			result = R_BUFFERQUERYFAILED;
			goto end;
		}

		/***** Map buffer into application space *****/
		bufferMap = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraHandle->m_deviceHandle, buffer.m.offset);
		if(MAP_FAILED == bufferMap)
		{
			fprintf(stderr, "%s: camera_create() - Buffer map failed - %s\n", g_programName, strerror(errno));

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
			fprintf(stderr, "%s: camera_create() - Buffer queue failed - %s\n", g_programName, strerror(errno));

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
	fprintf(stderr, "%s: camera_create(%d, %d, %u, %u, %p)\n", g_programName, i_deviceID, i_pixelFormat, i_sizeX, i_sizeY, o_cameraHandle);
	camera_destroy(&cameraHandle);

	return result;
};

Result camera_destroy(CameraHandle **const io_cameraHandle)
{
	size_t bufferIndex=0;
	CameraHandle *cameraHandle = NULL;

	/***** Input Validation *****/
	if(io_cameraHandle == NULL)
	{
		fprintf(stderr, "%s: camera_destroy() - Received NULL pointer\n", g_programName);
		return R_INPUTBAD;
	}

	/***** Is it a valid camera? *****/
	cameraHandle = (*io_cameraHandle);
	if(cameraHandle == NULL)
	{
		fprintf(stderr, "%s: camera_destroy() - Camera handle already destroyed\n", g_programName);
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

Result camera_start(CameraHandle *const io_cameraHandle)
{
	int xioResult = -1;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/***** Start capturing *****/
	xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_STREAMON, &type);
	if(xioResult == -1)
	{
		fprintf(stderr, "%s: camera_start(%p) - xioctl - %s", g_programName, io_cameraHandle, strerror(errno));

		return R_DEVICESTARTFAILED;
	}

	return R_SUCCESS;
}

Result camera_stop(CameraHandle * const io_cameraHandle)
{
	int xioResult = -1;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/***** Stop capturing *****/
	xioResult = xioctl(io_cameraHandle->m_deviceHandle, VIDIOC_STREAMOFF, &type);
	if(xioResult == -1)
	{
		fprintf(stderr, "%s: camera_stop(%p) - xioctl - %s", g_programName, io_cameraHandle, strerror(errno));

		return R_DEVICESTOPFAILED;
	}

	return R_SUCCESS;
}

