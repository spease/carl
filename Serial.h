#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "carl.h"

#include <stdlib.h>
#include <stdint.h>

/********************----- ENUM: BaudRate -----********************/
enum BaudRate_e
{
	SERIAL_BAUDRATE_4800,
	SERIAL_BAUDRATE_9600,
	SERIAL_BAUDRATE_14400,
	SERIAL_BAUDRATE_19200,
	SERIAL_BAUDRATE_28800,
	SERIAL_BAUDRATE_38400,
	SERIAL_BAUDRATE_57600,
	SERIAL_BAUDRATE_115200
};
typedef enum BaudRate_e BaudRate;
/**************************************************/

/********************----- ENUM: SerialMode -----********************/
enum SerialMode_e
{
	SERIAL_MODE_DEFAULT,
	SERIAL_MODE_ARDUINO
};
typedef enum SerialMode_e SerialMode;
/**************************************************/

/********************----- STRUCT: Serial -----********************/
struct Serial_t;
typedef struct Serial_t Serial;
/**************************************************/

Result serial_create(int const i_deviceID,
							BaudRate const i_baudRate,
							SerialMode const i_serialMode,
							Serial ** const o_serialHandle);
Result serial_read(	size_t const i_bytesToRead,
							uint8_t * const o_outputBuffer,
							size_t * const o_bytesRead,
							Serial const * const i_serialHandle);
Result serial_write(	size_t const i_bytesToWrite,
							uint8_t const * const i_data,
							size_t * const o_bytesWritten,
							Serial const * const i_serialHandle);
Result serial_destroy(Serial ** const io_serialHandle);

#endif /* _SERIAL_H_ */
