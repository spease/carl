#ifndef _GLOBAL_H_
#define _GLOBAL_H_

/********************----- ENUM: Result -----********************/
enum Result_e
{
   R_TRUE=1,
   R_FALSE=0,
   R_SUCCESS=0,
   R_FAILURE=-1,
   R_INPUTBAD=-2,
   R_MEMORYALLOCATIONERROR=-3,
   R_DEVICEOPENFAILED=-4,
   R_DEVICEPIXELFORMATFAILED=-5,
   R_DEVICERESOLUTIONFAILED=-6,
	R_DEVICEINVALID=-7,
   R_BUFFERREQUESTFAILED=-8,
   R_BUFFERMAPFAILED=-9,
   R_BUFFERENQUEUEFAILED=-10,
   R_BUFFERDEQUEUEFAILED=-11,
   R_BUFFERQUERYFAILED=-12,
   R_OBJECTNOTEXTANT=-13,
	R_DEVICENOVIDEOCAP=-14,
	R_DEVICENOSTREAMCAP=-15,
	R_DEVICESTARTFAILED=-16,
	R_DEVICESTOPFAILED=-17,
	R_DEVICEINPUTSPEEDFAILED=-18,
	R_DEVICEOUTPUTSPEEDFAILED=-19,
	R_DEVICEATTRIBUTEGETFAILED=-20,
	R_DEVICEATTRIBUTESETFAILED=-21,
	R_DEVICEREADFAILED=-22,
	R_DEVICEWRITEFAILED=-23,
	R_DEVICECLOSEFAILED=-24
};

typedef enum Result_e Result;
/**************************************************/

/********************----- Utility Macros -----********************/
#ifndef CLEAR
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#endif

#ifndef MIN
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#endif
/**************************************************/

extern char const * g_programName;

#endif	/* _GLOBAL_H_ */
