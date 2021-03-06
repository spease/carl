#include "Timer.h"
#include "carl.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void timer_sleep(double const i_seconds)
{
	int const seconds = (int)floor(i_seconds);
	if(seconds > 0)
	{
		sleep((int)i_seconds);
	}

	__useconds_t const microseconds = ((i_seconds-((double)seconds))*1000000.0);
	usleep(microseconds);
}

void timer_start(Timer * const o_timerHandle)
{
	if(o_timerHandle == NULL)
	{
		return;
	}

	o_timerHandle->m_timeTotalSeconds = NAN;
	gettimeofday(&o_timerHandle->m_timeStart, NULL);
}

void timer_stop(Timer * const io_timerHandle)
{
	double timeTotalMicroseconds = 0.0;

	if(io_timerHandle == NULL)
	{
		return;
	}

	gettimeofday(&io_timerHandle->m_timeStop, NULL);
	timeTotalMicroseconds = ((double)(io_timerHandle->m_timeStop.tv_sec - io_timerHandle->m_timeStart.tv_sec))*1000000.0;
	timeTotalMicroseconds +=((double)(io_timerHandle->m_timeStop.tv_usec - io_timerHandle->m_timeStart.tv_usec));
	io_timerHandle->m_timeTotalSeconds = timeTotalMicroseconds / 1000000.0;
}

double timer_total_seconds(Timer const * const i_timerHandle)
{
	if(i_timerHandle == NULL)
	{
		return NAN;
	}

	if(i_timerHandle->m_timeTotalSeconds == NAN)
   {
		struct timeval timeCurrent;
		double timeTotalMicroseconds = 0.0;

		gettimeofday(&timeCurrent, NULL);
		timeTotalMicroseconds = ((double)(timeCurrent.tv_sec - i_timerHandle->m_timeStart.tv_sec))*1000000.0;
		timeTotalMicroseconds +=((double)(timeCurrent.tv_usec - i_timerHandle->m_timeStart.tv_usec));

		return (timeTotalMicroseconds / 1000000.0);
   }

	return i_timerHandle->m_timeTotalSeconds;
}

