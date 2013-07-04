#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "carl.h"

#include <stdint.h>
#include <stdlib.h>

/********************----- STRUCT: Camera -----********************/
struct Camera_s;
typedef struct Camera_s Camera;
/**************************************************/

/********************----- ENUM: PixelFormat -----********************/
enum PixelFormat_e
{
	CAMERA_PIXELFORMAT_MJPEG,
	CAMERA_PIXELFORMAT_UYVY,
	CAMERA_PIXELFORMAT_YUYV
};
typedef enum PixelFormat_e PixelFormat;
/**************************************************/

typedef void (*CameraCallback)(uint8_t const * const i_frameData, size_t const i_frameSizeBytes, void * const i_callbackData);

Result camera_capture_callback(CameraCallback i_callback, void * const i_callbackData, Camera * const io_cameraHandle);
Result camera_capture_copy(size_t const i_outputSizeBytesMax, uint8_t * const o_outputBuffer, Camera * const io_cameraHandle);
Result camera_create(int32_t i_deviceID,
							PixelFormat const i_pixelFormat,
							uint32_t const i_sizeX,
							uint32_t const i_sizeY,
							Camera ** const o_cameraHandle);
Result camera_destroy(Camera **const io_cameraHandle);
Result camera_start(Camera *const io_cameraHandle);
Result camera_stop(Camera * const io_cameraHandle);

#ifdef __cplusplus
}
#endif

#endif	/* _CAMERA_H_ */
