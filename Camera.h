#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "carl.h"

#include <stdint.h>
#include <stdlib.h>

/********************----- STRUCT: CameraHandle -----********************/
struct CameraHandle_s;
typedef struct CameraHandle_s CameraHandle;
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

Result camera_capture(CameraHandle * const io_cameraHandle, size_t const i_outputSizeBytesMax, uint8_t * const o_outputBuffer);
Result camera_create(int32_t i_deviceID,
							PixelFormat const i_pixelFormat,
							uint32_t const i_sizeX,
							uint32_t const i_sizeY,
							CameraHandle ** const o_cameraHandle);
Result camera_destroy(CameraHandle **const io_cameraHandle);
Result camera_start(CameraHandle *const io_cameraHandle);
Result camera_stop(CameraHandle * const io_cameraHandle);

#endif	/* _CAMERA_H_ */
