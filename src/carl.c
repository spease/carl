#include "carl.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/********************----- Global Variables -----********************/
uint64_t const g_nanU64 = 0x7ff0000000000000;

char * g_programName = "carl";
int g_programNameAllocated = 0;
/**************************************************/

/********************----- Internal Functions -----********************/
static void carl_free_program_name(void)
{
	if(!g_programNameAllocated || g_programName == NULL)
	{
		return;
	}

	free(g_programName);
	g_programName = NULL;
	g_programNameAllocated = 0;
}


static Result carl_print_time(FILE * const i_fileHandle)
{
	static char const * const sc_strEmpty="[\?\?\?\?-\?\?-\?\? \?\?:\?\?:\?\?] ";
	size_t strResult=0;
	char timeBuf[32];
	time_t timeUnix;
	struct tm *timeLocal;

	timeUnix = time(NULL);
	timeLocal=localtime(&timeUnix);
	if(timeLocal == NULL)
	{
		fputs(sc_strEmpty, i_fileHandle);
		return R_LOCALTIMEFAILED;
	}

	strResult=strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", timeLocal);
	if(strResult == 0)
	{
		fputs(sc_strEmpty, i_fileHandle);
		return R_STRINGFORMATFAILED;
	}

	fputs(timeBuf, i_fileHandle);
	fputc(' ', i_fileHandle);

	return R_SUCCESS;
}
/**************************************************/

/********************----- Error Handling -----********************/
void carl_error(char const * const i_functionName, ...)
{
	if(g_programName != NULL)
	{
		fputs(g_programName, stderr);
		fputs(": ", stderr);
	}
	carl_print_time(stderr);
	if(i_functionName != NULL)
	{
		fputs(i_functionName, stderr);
		fputs(" - ", stderr);
	}

	va_list argumentList;
	va_start(argumentList, i_functionName);
	char const *format = va_arg(argumentList, char const *);
	vfprintf(stderr, format, argumentList);
	va_end(argumentList);

	fprintf(stderr, "\n");
}

void carl_errorno(char const * const i_functionName, ...)
{
	int const savedErrorNo = errno;

	if(g_programName != NULL)
	{
		fputs(g_programName, stderr);
		fputs(": ", stderr);
	}
	carl_print_time(stderr);
	if(i_functionName != NULL)
	{
		fputs(i_functionName, stderr);
		fputs(" - ", stderr);
	}

	va_list argumentList;
	va_start(argumentList, i_functionName);
	char const *format = va_arg(argumentList, char const *);
	vfprintf(stderr, format, argumentList);
	va_end(argumentList);

	fprintf(stderr, " - \"%s\"\n", strerror(savedErrorNo));
}

void carl_info(char const * const i_functionName, ...)
{
	if(g_programName != NULL)
	{
		fputs(g_programName, stdout);
		fputs(": ", stdout);
	}
	carl_print_time(stdout);
	if(i_functionName != NULL)
	{
		fputs(i_functionName, stdout);
		fputs(" - ", stdout);
	}

	va_list argumentList;
	va_start(argumentList, i_functionName);
	char const *format = va_arg(argumentList, char const *);
	vfprintf(stderr, format, argumentList);
	va_end(argumentList);

	fprintf(stdout, "\n");
}

Result carl_set_program_name(char const * const i_programName)
{
	char *nextProgramName = NULL;
	if(i_programName == NULL)
	{
		/***** Set to blank *****/
		nextProgramName = NULL;
	}
	else
	{
		/***** Set to input name *****/
		size_t const nextProgramNameLength = strlen(i_programName);
		size_t const nextProgramNameSizeBytes = (nextProgramNameLength+1)*sizeof(char);
		nextProgramName = (char *)malloc(nextProgramNameSizeBytes);
		if(nextProgramName == NULL)
		{
			CARL_ERROR("Not enough memory to copy program name.");
			return R_MEMORYALLOCATIONERROR;
		}

		memcpy(nextProgramName, i_programName, nextProgramNameSizeBytes);
	}

	if(g_programNameAllocated)
	{
		char * const oldProgramName = g_programName;
		g_programName = nextProgramName;
		free(oldProgramName);
	}
	else
	{
		g_programName = nextProgramName;
		if(!g_programNameAllocated)
		{
			g_programNameAllocated = 1;
			atexit(carl_free_program_name);
		}
	}

	return R_SUCCESS;
}
/**************************************************/

