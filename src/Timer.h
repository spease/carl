#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

/********************----- STRUCT: Timer -----********************/
struct Timer_s
{
   struct timeval m_timeStart;
   struct timeval m_timeStop;
   double m_timeTotalSeconds;
};
typedef struct Timer_s Timer;
/**************************************************/

void timer_sleep(double const i_seconds);

void timer_start(Timer * const o_timerHandle);
void timer_stop(Timer * const io_timerHandle);
double timer_total_seconds(Timer const * const i_timerHandle);

#ifdef __cplusplus
}
#endif

#endif	/* _TIMER_H_ */
