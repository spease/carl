#ifndef _TIMER_H_
#define _TIMER_H_

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

void timer_start(Timer * const o_timerHandle);
void timer_stop(Timer * const io_timerHandle);
double timer_total_seconds(Timer const * const i_timerHandle);

#endif	/* _TIMER_H_ */
