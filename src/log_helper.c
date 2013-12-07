/*
 * vim:expandtab:shiftwidth=4:tabstop=4:
 *
 * Copyright   (2013)      Contributors
 * Contributor : chitr   chitr.prayatan@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ---------------------------------------
 */
/*
 * vim:expandtab:shiftwidth=4:tabstop=4:
 *
 * Copyright   (2013)      Contributors
 * Contributor : chitr   chitr.prayatan@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ---------------------------------------
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "log_helper.h"


char logFile[]="/var/log/simpleTaskqueue.log";
int  GlobalLogLevel=0;
char LOGTYPE[LOGTYPECOUNT][MAXLOGTYPELEN]={"LOG_DEBUG","LOG_INFO","LOG_CRITICAL","LOG_ERROR"};

void STQ_LOG(int loglevel,char * fmt, ...) {
    if(loglevel>=GlobalLogLevel){
        char fmtString[MAX_MSG_LEN];
        memset(fmtString, 0, MAX_MSG_LEN);
        int close = 1;
        FILE *logFilePtr = NULL;
        int errsv;
        logFilePtr = fopen(logFile, "a+");
        errsv = errno;
        if(!logFilePtr) {
            fprintf(stderr, "Failed to open log file %s %s %d \n", logFile,
                    strerror(errsv), errsv);
            close = 0;
            logFilePtr = stderr;
        }
        char date[200];
        time_t tm = time(NULL);
        int ret = strftime(date, 200, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tm));
        if(!ret) {
            memset(date, 0, 200);
        }
        va_list fmtArgs;
        va_start(fmtArgs, fmt);
        snprintf(fmtString, sizeof(fmtString), "%s:%s :  :Error : %s\n",
                LOGTYPE[(loglevel>LOG_ERROR)?LOG_ERROR:loglevel],date, fmt);
            vfprintf(logFilePtr, fmtString, fmtArgs);
        va_end(fmtArgs);
        if(close) {
            fclose(logFilePtr);
        }	
    }
}


struct timeval 
time_diff(struct timeval time_from, struct timeval time_to)
{

  struct timeval result;

  if(time_to.tv_usec < time_from.tv_usec)
    {
      result.tv_sec = time_to.tv_sec - time_from.tv_sec - 1;
      result.tv_usec = 1000000 + time_to.tv_usec - time_from.tv_usec;
    }
  else
    {
      result.tv_sec = time_to.tv_sec - time_from.tv_sec;
      result.tv_usec = time_to.tv_usec - time_from.tv_usec;
    }

  return result;
}

