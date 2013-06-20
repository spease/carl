#include "Serial.h"

#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>

/********************----- STRUCT: Serial -----********************/
struct Serial_t
{
	int m_deviceHandle;
};
typedef struct Serial_t Serial;
/**************************************************/

Result serial_create(int const i_deviceID,
							BaudRate const i_baudRate,
							SerialMode const i_serialMode,
							Serial ** const o_serialHandle)
{
	int cfResult = 0;
	int deviceBaudRate = 0;
   Result result = R_FAILURE;
	struct termios serialAttributes;
	Serial *serialHandle = NULL;
	char serialPathname[PATH_MAX];
	int tcResult = 0;

	/***** Get baud rate *****/
   switch(i_baudRate)
   {
      case SERIAL_BAUDRATE_4800:
         deviceBaudRate = B4800;
         break;
		case SERIAL_BAUDRATE_9600:
			deviceBaudRate = B9600;
			break;
#ifdef B14400
		case SERIAL_BAUDRATE_14400:
			deviceBaudRate = B14400;
			break;
#endif
		case SERIAL_BAUDRATE_19200:
			deviceBaudRate = B19200;
			break;
#ifdef B28800
		case SERIAL_BAUDRATE_28800:
			deviceBaudRate = B28800;
			break;
#endif
		case SERIAL_BAUDRATE_38400:
			deviceBaudRate = B38400;
			break;
		case SERIAL_BAUDRATE_57600:
			deviceBaudRate = B57600;
			break;
		case SERIAL_BAUDRATE_115200:
			deviceBaudRate = B115200;
			break;
		default:
			fprintf(stderr, "%s: serial_create() - Invalid baudrate (%d).\n", g_programName, i_baudRate);

			result = R_INPUTBAD;
			goto end;
   }

	/***** Serial Handle *****/
	serialHandle = (Serial *)malloc(sizeof(Serial));
	if(serialHandle == NULL)
	{
		fprintf(stderr, "%s: serial_create() - Insufficient memory for serial handle.\n", g_programName);

		result = R_MEMORYALLOCATIONERROR;
		goto end;
	}
	serialHandle->m_deviceHandle = -1;

	/***** Setup serial pathname *****/
	snprintf(serialPathname, sizeof(serialPathname), "/dev/ttyUSB%d", i_deviceID);

	/***** Open serial port *****/
	serialHandle->m_deviceHandle = open(serialPathname, O_RDWR | O_NOCTTY | O_NDELAY);
	if(serialHandle->m_deviceHandle < 0)
	{
		fprintf(stderr, "%s: serial_create() - Unable to open serial device - %s\n", g_programName, strerror(errno));

		result = R_DEVICEOPENFAILED;
		goto end;
	}

	tcResult = tcgetattr(serialHandle->m_deviceHandle, &serialAttributes);
	if(tcResult < 0)
	{
		fprintf(stderr, "%s: serial_create() - Unable to get serial attributes - %s\n", g_programName, strerror(errno));

		result = R_DEVICEATTRIBUTEGETFAILED;
		goto end;
	}

	/***** Setup parameters *****/
	cfResult = cfsetispeed(&serialAttributes, deviceBaudRate);
	if(cfResult < 0)
	{
		fprintf(stderr, "%s: serial_create() - Unable to set serial input speed - %s\n", g_programName, strerror(errno));

		result = R_DEVICEINPUTSPEEDFAILED;
		goto end;
	}

	cfResult = cfsetospeed(&serialAttributes, deviceBaudRate);
	if(cfResult < 0)
	{
		fprintf(stderr, "%s: serial_create() - Unable to set serial output speed - %s\n", g_programName, strerror(errno));

		result = R_DEVICEOUTPUTSPEEDFAILED;
		goto end;
	}

	switch(i_serialMode)
	{
		case SERIAL_MODE_ARDUINO:
			//Based on:
			//http://todbot.com/arduino/host/arduino-serial/arduino-serial.c

			serialAttributes.c_cflag &= ~(PARENB);		//No parity
			serialAttributes.c_cflag &= ~(CSTOPB);		//One stop bit
			serialAttributes.c_cflag &= ~(CSIZE);		//Clear all CS* bits
			serialAttributes.c_cflag |= CS8;				//8 bits
			serialAttributes.c_cflag &= ~(CRTSCTS);	//No hardware flow control
			serialAttributes.c_cflag |= CREAD;			//Enable receiver
			serialAttributes.c_cflag |= CLOCAL;			//Ignore control lines

			serialAttributes.c_iflag &= ~(IXON | IXOFF | IXANY);	//No software flow control;

			serialAttributes.c_lflag &= ~(ICANON | ECHO | ECHOE);	//Disable echo
			serialAttributes.c_lflag &= ~(ISIG);						//Disable signals

			serialAttributes.c_oflag &= ~(OPOST);						//Disble output processing

			//See:
			//http://unixwiz.net/techtips/termios-vmin-vtime.html
			serialAttributes.c_cc[VMIN] = 0;				//Wait for any data
			serialAttributes.c_cc[VTIME] = 20;			//Wait VTIME tenths of a second for more data
			break;
		case SERIAL_MODE_DEFAULT:
		default:
			break;
	}

	/***** Set attributes *****/
	tcResult = tcsetattr(serialHandle->m_deviceHandle, TCSANOW, &serialAttributes);
	if(tcResult < 0)
	{
		fprintf(stderr, "%s: serial_create() - Unable to set serial attributes - %s\n", g_programName, strerror(errno));

		result = R_DEVICEATTRIBUTESETFAILED;
		goto end;
	}

	if(o_serialHandle != NULL)
	{
		(*o_serialHandle) = serialHandle;
	}
	else
	{
		serial_destroy(&serialHandle);
	}

	return R_SUCCESS;

end:
	fprintf(stderr, "%s: serial_create(%d, %d, %p)\n", g_programName, i_deviceID, i_baudRate, o_serialHandle);
	serial_destroy(&serialHandle);

   return result;
}

Result serial_read(  size_t const i_bytesToRead,
                     uint8_t * const o_outputBuffer,
                     size_t * const o_bytesRead,
                     Serial const * const i_serialHandle)
{
	Result result = R_FAILURE;
	ssize_t readResult = 0;

	if(i_serialHandle == NULL)
	{
		fprintf(stderr, "%s: serial_read() - Handle not created.\n", g_programName);

		result = R_OBJECTNOTEXTANT;
		goto end;
	}

	readResult = read(i_serialHandle->m_deviceHandle, o_outputBuffer, i_bytesToRead);
	if(readResult < 0)
	{
		fprintf(stderr, "%s: serial_read() - IO Error - %s\n", g_programName, strerror(errno));

		result = R_DEVICEREADFAILED;
		goto end;
	}

	if(o_bytesRead != NULL)
	{
		(*o_bytesRead) = readResult;
	}

	return R_SUCCESS;

end:
	fprintf(stderr, "%s: serial_read(%zu, %p, %p, %p)\n", g_programName, i_bytesToRead, o_outputBuffer, o_bytesRead, i_serialHandle);
	return result;
}

Result serial_write( size_t const i_bytesToWrite,
                     uint8_t const * const i_data,
                     size_t * const o_bytesWritten,
                     Serial const * const i_serialHandle)
{
	Result result = R_FAILURE;
	ssize_t writeResult = 0;

	if(i_serialHandle == NULL)
	{
		fprintf(stderr, "%s: serial_write() - Handle not created.\n", g_programName);

		result = R_OBJECTNOTEXTANT;
		goto end;
	}

	writeResult = write(i_serialHandle->m_deviceHandle, i_data, i_bytesToWrite);
	if(writeResult < 0)
	{
		fprintf(stderr, "%s: serial_write() - IO Error - %s\n", g_programName, strerror(errno));

		result = R_DEVICEWRITEFAILED;
		goto end;
	}

	if(o_bytesWritten != NULL)
	{
		(*o_bytesWritten) = writeResult;
	}

	return R_SUCCESS;

end:
	fprintf(stderr, "%s: serial_write(%zu, %p, %p, %p)\n", g_programName, i_bytesToWrite, i_data, o_bytesWritten, i_serialHandle);
	return result;
}

Result serial_destroy(Serial ** const io_serialHandle)
{
	int closeResult = 0;
	Serial * serialHandle = NULL;

	/***** Input Validation *****/
	if(io_serialHandle == NULL)
	{
		return R_INPUTBAD;
	}

	/***** Check if already destroyed *****/
	serialHandle = (*io_serialHandle);
	if(serialHandle == NULL)
	{
		return R_SUCCESS;
	}

	/***** Close handle *****/
	closeResult = close(serialHandle->m_deviceHandle);
	if(closeResult < 0)
	{
		fprintf(stderr, "%s: serial_destroy() - Error closing port - %s\n", g_programName, strerror(errno));
	
		return R_DEVICECLOSEFAILED;
	}

	/***** Return *****/
	return R_SUCCESS;
}

