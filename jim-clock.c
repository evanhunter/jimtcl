/*
 * jim-clock.c
 *
 * Implements the clock command
 */

/* For strptime() */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

/* for localtime_r & gmtime_r */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "jimautoconf.h"
#include <jim-subcmd.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

static int clock_cmd_format(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    /* How big is big enough? */
    char buf[100];
    time_t t;
	int i;
	jim_wide seconds;
	enum {
		FORMAT_OPT_FORMAT, FORMAT_OPT_GMT, FORMAT_OPT_LOCALE, FORMAT_OPT_BASE, FORMAT_OPT_TIMEZONE
	};
	static const char * const options[] = {
		"-format", "-gmt", "-locale", "-base", "-timezone", NULL
	};
    const char *format = "%a %b %d %H:%M:%S %Z %Y";
	const char *locale = "";
	const char *timzone = "";
	jim_wide base = 0;

	int gmt = 0;

	if (argc < 1) {
		goto wrongNumArgs;
	}

	for (i = 1; i < argc; i++) {
		const char *opt = Jim_String(argv[i]);
		int option;

		if (*opt != '-') {
			Jim_SetResultFormatted(interp, "Unknown trailing data \"%#s\"", argv[i]);
			return JIM_ERR;
		}
		if (Jim_GetEnum(interp, argv[i], options, &option, "format", JIM_ERRMSG) != JIM_OK) {
			return JIM_ERR;
		}
		switch (option) {
		case FORMAT_OPT_FORMAT:
			if (++i == argc) {
				goto wrongNumArgs;
			}
			format = Jim_String(argv[i]);
			break;

		case FORMAT_OPT_GMT: {
				if (++i == argc) {
					goto wrongNumArgs;
				}
				if (Jim_GetBoolean(interp, argv[i], &gmt) != JIM_OK) {
					Jim_SetResultFormatted(interp, "-gmt argument \"%#s\" is not a boolean value", argv[i]);
					return JIM_ERR;
				}
			}
			break;

		case FORMAT_OPT_LOCALE:
			if (++i == argc) {
				goto wrongNumArgs;
			}
			locale = Jim_String(argv[i]);
			/* Currently unused */
			break;

		case FORMAT_OPT_BASE: {
				if (++i == argc) {
					goto wrongNumArgs;
				}
				if (Jim_GetWide(interp, argv[i], &base) != JIM_OK) {
					Jim_SetResultFormatted(interp, "-base argument \"%#s\" is not a wide integer", argv[i]);
					return JIM_ERR;
				}
				/* Currently unused */
			}
			break;

		case FORMAT_OPT_TIMEZONE:
			if (++i == argc) {
				goto wrongNumArgs;
			}
			timzone = Jim_String(argv[i]);
			/* Currently unused */
			break;
		}
	}

    if (Jim_GetWide(interp, argv[0], &seconds) != JIM_OK) {
        return JIM_ERR;
    }
    t = seconds;

	struct tm tm_value;
	struct tm *tm_ptr;
	if (gmt) {
		tm_ptr = gmtime_r(&t, &tm_value);
	}
	else {
		tm_ptr = localtime_r(&t, &tm_value);
	}

	if (tm_ptr == NULL) {
		Jim_SetResultString(interp, "Error converting numeric to time", -1);
		return JIM_ERR;
	}

    if (strftime(buf, sizeof(buf), format, &tm_value) == 0) {
        Jim_SetResultString(interp, "format string too long", -1);
        return JIM_ERR;
    }

    Jim_SetResultString(interp, buf, -1);

    return JIM_OK;


wrongNumArgs:
	Jim_WrongNumArgs(interp, 1, argv,
		"?-switch ...? exp string ?matchVar? ?subMatchVar ...?");
	return JIM_ERR;
}

#ifdef HAVE_STRPTIME
static int clock_cmd_scan(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    char *pt;
    struct tm tm;
    time_t now = time(0);

    if (!Jim_CompareStringImmediate(interp, argv[1], "-format")) {
        return -1;
    }

    /* Initialise with the current date/time */
    localtime_r(&now, &tm);

    pt = strptime(Jim_String(argv[0]), Jim_String(argv[2]), &tm);
    if (pt == 0 || *pt != 0) {
        Jim_SetResultString(interp, "Failed to parse time according to format", -1);
        return JIM_ERR;
    }

    /* Now convert into a time_t */
    Jim_SetResultInt(interp, mktime(&tm));

    return JIM_OK;
}
#endif

static int clock_cmd_seconds(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    Jim_SetResultInt(interp, time(NULL));

    return JIM_OK;
}

static int clock_cmd_micros(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    Jim_SetResultInt(interp, (jim_wide) tv.tv_sec * 1000000 + tv.tv_usec);

    return JIM_OK;
}

static int clock_cmd_millis(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    Jim_SetResultInt(interp, (jim_wide) tv.tv_sec * 1000 + tv.tv_usec / 1000);

    return JIM_OK;
}

static const jim_subcmd_type clock_command_table[] = {
    {   "seconds",
        NULL,
        clock_cmd_seconds,
        0,
        0,
        /* Description: Returns the current time as seconds since the epoch */
    },
    {   "clicks",
        NULL,
        clock_cmd_micros,
        0,
        0,
        /* Description: Returns the current time in 'clicks' */
    },
    {   "microseconds",
        NULL,
        clock_cmd_micros,
        0,
        0,
        /* Description: Returns the current time in microseconds */
    },
    {   "milliseconds",
        NULL,
        clock_cmd_millis,
        0,
        0,
        /* Description: Returns the current time in milliseconds */
    },
    {   "format",
        "seconds ?-format format?",
        clock_cmd_format,
        1,
		-1,
        /* Description: Format the given time */
    },
#ifdef HAVE_STRPTIME
    {   "scan",
        "str -format format",
        clock_cmd_scan,
        3,
        3,
        /* Description: Determine the time according to the given format */
    },
#endif
    { NULL }
};

int Jim_clockInit(Jim_Interp *interp)
{
    if (Jim_PackageProvide(interp, "clock", "1.0", JIM_ERRMSG))
        return JIM_ERR;

    Jim_CreateCommand(interp, "clock", Jim_SubCmdProc, (void *)clock_command_table, NULL);
    return JIM_OK;
}
