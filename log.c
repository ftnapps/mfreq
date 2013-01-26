/* ************************************************************************
 *
 *   logging, errors and warnings
 *
 *   (c) 1994-2012 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define LOG_C       1


/*
 *  include header files
 */

/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external funtions */


/*
 *  extra definitions if required
 */

/* stdout file descriptor */
#ifndef STDOUT_FILENO
  #define STDOUT_FILENO 1
#endif

/* stderr file descriptor */
#ifndef STDERR_FILENO
  #define STDERR_FILENO 2
#endif



/* ************************************************************************
 *   logging
 * ************************************************************************ */


/*
 *  log stuff (printf syntax)
 *  - write to logfile if one is set up 
 *  - write to console if not logfile is set up
 */

void Log(unsigned short int Type, const char *Line, ...)
{
  va_list           ArgPointer;
  size_t            Length = 0;         /* string length written */
  size_t            Size = 0;           /* used size of buffer */
  char              *Buffer;            /* support pointer */
  char              *Message;
  FILE              *Stream;
  _Bool             ExitFlag = False;   /* exit flag */

  /* sanity checks */
  if ((Line == NULL) || (LogBuffer == NULL)) return;


  /*
   *  init
   */

  Buffer = LogBuffer;                   /* init pointer */
  if (Type == L_ERR) ExitFlag = True;   /* set exit flag for errors */


  /*
   *  add date, time and PID for logfile
   *  format: <date> <time> [<process#>] ...
   */

  if (Env && Env->Log)
  {
    time_t          UnixTime;
    struct tm       *DateTime;  

    /* get current date and time */
    if (time(&UnixTime) != -1)           /* get UNIX time */
    {
      DateTime = localtime(&UnixTime);   /* convert unix time */
      if (DateTime != NULL)
      {
        /* format: year-month-day hh:mm:ss */
        Length = strftime(Buffer, 22, "%F %T", DateTime);
        if (Length > 0)       /* success */
        {
          Buffer += Length;     /* move pointer to end of string */
          Size += Length;       /* update counter */
        }
        else                  /* error */
        {
          Buffer[0] = 0;        /* reset buffer */
        }
      }
    }

    /* add process ID */
    Length = snprintf(Buffer, DEFAULT_BUFFER_SIZE - 2 - Size," [%d]: ", Env->PID);
    Buffer += Length;         /* move pointer to end of string */
    Size += Length;           /* update size */
  }

  Message = Buffer;           /* start of message */


  /*
   *  add log message
   */

  va_start(ArgPointer, Line);   /* set pointer for first unknown arg */

  /* print message to string with size limitation */
  Length = vsnprintf(Buffer, DEFAULT_BUFFER_SIZE - 2 - Size, Line, ArgPointer);

  va_end(ArgPointer);         /* clean up arg list */

  /* add LineFeed */
  Buffer += Length;           /* move pointer to end of string */
  Buffer[0] = '\n';
  Buffer[1] = 0;


  /*
   *  output message
   */

  if (Env && Env->Log)             /* log to file */
  { 
    fputs(LogBuffer, Env->Log);    /* write */
    fflush(Env->Log);              /* flush any buffered output */
  }
  else                             /* log to console */
  {
    /* set output stream */
    if (Type == L_ERR) Stream = stderr;
    else Stream = stdout;

    fputs(Message, Stream);   /* write */
    fflush(Stream);           /* flush any buffered output */
  }


  /*
   *  exit this program on error
   */

  if (ExitFlag) exit(EXIT_FAILURE);
}



/* ************************************************************************
 *   clean up of local definitions
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef LOG_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
