#include "Timer.h"

#include <math.h>
#include <unistd.h>

void timer_sleep(double const i_seconds)
{
	int const seconds = (int)floor(i_seconds);
	if(seconds > 0)
	{
		sleep((int)i_seconds);
	}

	useconds_t const microseconds = ((i_seconds-((double)seconds))*1000000.0);
	usleep(microseconds);
}

