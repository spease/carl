#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include <stdint.h>
#include <string.h>

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
	R_DEVICECLOSEFAILED=-24,
	R_DEVICEPARAMETERSETFAILED=-25,
	R_DEVICEHIGHQUALITYFAILED=-26,
	R_DEVICEPRIORITYSETFAILED=-27,
	R_DEVICEPRIORITYFAILED=-28,
	R_DEVICECONTROLSETFAILED=-29,
	R_LOCALTIMEFAILED=-30,
	R_STRINGFORMATFAILED=-31
};

typedef enum Result_e Result;
/**************************************************/

/********************----- Internal Constants -----********************/
extern uint64_t const g_nanU64;
/**************************************************/

/********************----- Utility Macros -----********************/
#ifndef CLEAR
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#endif

#ifndef MAX
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#endif

#ifndef NAN
#define NAN() (*((double*)&g_nanU64))
#endif
/**************************************************/

/********************----- Internal Error Handling -----********************/
void carl_error(char const * const i_functionName, ...);
void carl_errorno(char const * const i_functionName, ...);
void carl_info(char const * const i_functionName, ...);
/**************************************************/

/********************----- Error Handling -----********************/
#define CARL_ERROR(...) carl_error(__func__, __VA_ARGS__)
#define CARL_ERRORNO(...) carl_errorno(__func__, __VA_ARGS__)
#define CARL_INFO(...) carl_info(__func__, __VA_ARGS__)
Result carl_set_program_name(char const * const i_programName);
/**************************************************/

#endif	/* _GLOBAL_H_ */
