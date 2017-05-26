/* ************************************************************************
 *
 *   misc support functions
 *
 *   (c) 2012-2015 by Markus Reschke
 *
 * ************************************************************************ */

/*
 *  local constants
 */

#define MISC_C


/*
 *  include header files
 */

/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */

/* string stuff */
#include <ctype.h>

/* files */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>



/* ************************************************************************
 *   string functions
 * ************************************************************************ */


/*
 *  copy string while allocating memory for new string
 *
 *  returns:
 *  - Pointer on success
 *  - NULL on error
 */

char *CopyString(const char *Source)
{
  char      *Destination = NULL;
  
  if (Source == NULL) return(Destination);     /* check argument */

  Destination = malloc(strlen(Source) + 1);    /* get memory */
  if (Destination == NULL)
  {
    Log(L_ERR, "No memory!");
  }
  else
  {
    strcpy(Destination, Source);
  }
  
  return(Destination);
}



/*
 *  convert string to long integer
 *
 *  returns:
 *  - -2 on problem (-1 is a valid value)
 *  - value on success
 */

long Str2Long(const char *Token)
{
  long             Value = -2;        /* return value */
  long             Temp = 0;          /* temp value */
  char             *Test = NULL;      /* test pointer */

  if (Token == NULL) return Value;       /* sanity check */

  Temp = strtol(Token, &Test, 10);       /* convert */

  if ((Test != NULL) && (Test[0] == 0))  /* got valid value */
  {
    Value = Temp;
  }

  return Value;
}



/*
 *  convert long long integer into string with byte unit
 *
 *  requires:
 *  - number of bytes
 *  - pointer to string buffer
 *  - size of string buffer
 *
 *  returns:
 *  - 1 on success
 *  - 0 on any problem
 */

_Bool Bytes2String(long long Bytes, char *Buffer, size_t Size)
{
  _Bool             Flag = False;            /* return value */
  unsigned int      Counter = 0;
  _Bool             SI_Flag = False;         /* decimal/binary flag */
  _Bool             IEC_Flag = False;        /* IEC units */ 
  char              Prefix[3] = {0, 0, 0};   /* prefix */


  /* sanity checks */
  if ((Buffer == NULL) || (Size < 12)) return Flag;

  /* check for use of SI units */
  if (Env->CfgSwitches & SW_SI_UNITS) SI_Flag = True;

  /* check for use of IEC units */
  if (Env->CfgSwitches & SW_IEC_UNITS)
  {
    IEC_Flag = True;
    SI_Flag = False;     /* IEC units imply binary factor */
  }

  /* scale down large value */
  if (SI_Flag)                /* use 1000 */
  {
    while (Bytes > 10000)
    {
      Bytes = Bytes / 1000;        /* scale by 1000 */
      Counter++;                   /* increase factor */
    }
  }
  else                        /* use 1024 */
  {
    while (Bytes > 10240)
    {
      Bytes = Bytes / 1024;        /* scale by 1024 */
      Counter++;                   /* increase factor */
    }
  }

  /* prefix */
  switch (Counter)
  {
    case 1:
      if (IEC_Flag) Prefix[0] = 'K'; 
      else Prefix[0] = 'k';
      break;

    case 2:
      Prefix[0] = 'M';
      break;

    case 3:
      Prefix[0] = 'G';
      break;
  }

  if (IEC_Flag)          /* KiB etc. */
  {
    Prefix[1] = 'i';               /* add "i" */
  }

  /* build string */
  if (Counter > 0)     /* with prefix and unit */
  { 
    if (snprintf(Buffer, 10, "%lld %sB", Bytes, &Prefix[0]) > 0) Flag = True;
  }
  else                 /* without prefix, just bytes */
  {
    /* we write "Bytes" instead of just "B", seems nicer */
    if (snprintf(Buffer, 12, "%lld Bytes", Bytes) > 0) Flag = True;
  }

  return Flag;
}



/*
 *  convert string with number and byte unit into long long integer
 *
 *  returns:
 *  - byte value on success
 *  - -2 on any problem
 */


long long String2Bytes(char *Token)
{
  long long         Bytes = -2;         /* return value */
  long              Value;              /* number */
  size_t            Length;             /* string length */
  char              *Test = NULL;       /* test pointer */
  char              Prefix;             /* unit prefix */
  _Bool             SI_Flag = False;    /* decimal/binary flag */

  /* sanity check */
  if (Token == NULL) return Bytes;

  /*
   *  We convert just a long integer because the unit will increase
   *  the value.
   */

  Value = strtol(Token, &Test, 10);       /* convert number */

  if (Test != NULL)
  {
    if (Test == Token)               /* no digits at all */
    {
      /* no work to be done */
    }
    else if (Test[0] == 0)           /* got valid value (i.e no unit) */
    {
      Bytes = Value;                 /* just digits, no unit */
    }
    else                             /* value plus unit */
    {
      /*
       *  get prefix of unit
       */

      /* skip whitespaces */
      while ((Test[0] == ' ') || (Test[0] == '\t'))
      {
        Test++;
      }

      /* check for use of SI units */
      if (Env->CfgSwitches & SW_SI_UNITS) SI_Flag = True;

      Length = strlen(Test);       /* length of remaining string */
      Prefix = toupper(Test[0]);   /* assumed prefix */
      Test++;                      /* skip first char */

      /* check syntax */
      if (Length == 1)             /* k etc. */
      {
        /* special case: "B" for bytes */
        if (Prefix == 'B')
        {
          Bytes = Value;           /* no prefix */
          Prefix = 0;              /* reset prefix */
        }
      }
      else if (Length == 2)        /* kB etc. */
      {
        /* check for "B" */
        if (strcasecmp(Test, "B") != 0) Prefix = 0;
      }
      else if (Length == 3)        /* KiB etc. */
      {
        /* check for "iB" */
        if (strcasecmp(Test, "iB") != 0) Prefix = 0;

        SI_Flag = False;           /* IEC unit implies binary */
      }
      else                         /* something else */
      {
        Prefix = 0;
      }

      /* manage multiplicator */
      if (Prefix > 0)              /* got prefix */
      {
        switch(Prefix)
        {
          case 'K':
            if (SI_Flag) Bytes = Value * 1000;
            else Bytes = Value * 1024;
            break;

          case 'M':
            if (SI_Flag) Bytes = Value * 1000000;
            else Bytes = Value * 1024 * 1024;
            break;

          case 'G':
            if (SI_Flag) Bytes = Value * 1000000000;
            else Bytes = Value * 1024 * 1024 * 1024;
            break;
        }
      }
    }
  }

  return Bytes;
}




/* ************************************************************************
 *   pattern matching
 * ************************************************************************ */


/*
 *  check if a string matches a pattern
 *
 *  supports following wildcards:
 *  - * for any number of characters
 *  - ? for a single character
 *
 *  returns:
 *  - 1 on match
 *  - 0 on error or mismatch
 */

_Bool MatchPattern(char *String, char *Pattern)
{
  _Bool                Flag = False;             /* return value */
  _Bool                Run = True;               /* control flag */
  _Bool                SubRun;                   /* control flag */
  unsigned int         SubLength;                /* length of sub pattern */
  unsigned int         Length;
  unsigned int         n;
  char                 *SubPattern = NULL;       /* pointer to sub pattern */
  char                 *VirtString;
  char                 *VirtPattern;

  /* sanity check */
  if ((String == NULL) || (Pattern == NULL)) return Flag;

  while (Run)
  {
    if (String[0] == 0)           /* end of string */
    {
      /* if also end of pattern is reached */
      if (Pattern[0] == 0) Flag = True;       /* string matches */
      /* if pattern has just "*" left */
      else if (Pattern[0] == '*')
      {
        /* skip multiples of wildcard */
        while (Pattern[1] == '*') Pattern++;       

        if (Pattern[1] == 0) Flag = True;     /* string matches */
      }

      Run = False;                            /* end loop */
    }
    else if (Pattern[0] == 0)     /* end of pattern */
    {
      /* if also end of string is reached */
      if (String[0] == 0) Flag = True;        /* string matches */
      Run = False;                            /* end loop */
    }
    else if (Pattern[0] == '*')   /* wildcard "*" */
    {
      /* skip multiples of wildcard */
      while (Pattern[1] == '*') Pattern++;

      /* move pattern to next "*" */
      /* and build virtual pattern substring by counting characters */
      SubPattern = Pattern;    /* save start */
      SubPattern++;            /* skip "*" */
      SubLength = 0;
      Pattern++;               /* skip "*" */
      while ((Pattern[0] != 0) && (Pattern[0] != '*'))
      {
        SubLength++;
        Pattern++;
      }

      if (SubLength == 0)          /* "*" at end of pattern */
      {
        /* matches anything left in string */
        Flag = True;        /* string matches */
        Run = False;        /* end loop */
      }
      else if (Pattern[0] == 0)    /* no more *s in pattern */
      {
        /* let's check if SubPattern matches end of string */

        /* check remaining length */
        Length = strlen(String);
        if (SubLength <= Length)      /* enough chars left in string */
        {
          /* move string pointer to the position at which the string
             contains the same amount of characters as the pattern */
          String += (Length - SubLength);

          /* compare the ramaining characters */
          /* while supporting "?" wildcard */
          SubRun = True;
          while (SubRun && (String[0] != 0))
          {
            if ((String[0] == SubPattern[0]) || (SubPattern[0] == '?')) 
            {
              String++;              /* go to next character */
              SubPattern++;
            }
            else                   /* chars mismatch */
            {
              SubRun = False;        /* end subloop */
              Run = False;           /* end loop */
            }
          }

          if (SubRun)          /* ends match */
          {
            Run = False;        /* end loop */
            Flag = True;        /* got match */
          }
        }
        else                          /* not enough chars left in string */
        {
          Run = False;          /* end loop */
        }
      }
      else                         /* more *s in pattern */
      {
        /* search for nearest match */

        /* init loop */
        VirtString = String;
        VirtPattern = SubPattern;
        n = SubLength;

        while ((VirtString[0] != 0) && (n != 0))
        {
          if ((VirtString[0] == VirtPattern[0]) ||
              (VirtPattern[0] == '?'))                   /* chars match */
          {
            VirtString++;        /* go to next character */
            VirtPattern++;       /* next pattern character */
            n--;                 /* one char less  to check */
          }
          else                 /* chars mismatch */
          {
            String++;                   /* go to next character */
            VirtString = String;
            VirtPattern = SubPattern;   /* start again */
            n = SubLength;              /* with old length */
          }
        }

        /* draw conclusions */
        if (n == 0)                  /* match found */
        {
          /* advance string pointer */
          String = VirtString;         /* char behind match */
        }
        else                         /* no match */
        {
          Run = False;                 /* end loop */
        }
      }
    }
    else if (Pattern[0] == '?')   /* wildcard "?" */
    {
      /* "?" matches any single character */
      /* no need for comparison */
      String++;             /* go to next character */
      Pattern++;      
    }
    else                          /* standard character */
    {
      /* compare the current characters */
      if (String[0] == Pattern[0])     /* characters match */
      {
        String++;           /* go to next character */
        Pattern++;
      }
      else                             /* mismatch */
      {
        Run = False;        /* end loop */
      }      
    }
  }

  return Flag;
}



/* ************************************************************************
 *   file functions
 * ************************************************************************ */


/*
 *  unlock file
 */

void UnlockFile(FILE *File)
{
  int           FileDescriptor;
  int           Value;

  /* sanity check */
  if (File == NULL) return;

  FileDescriptor = fileno(File);              /* get fd */
  Value = lockf(FileDescriptor, F_ULOCK, 0);  /* try to unlock */

  if (Value == -1) Log(L_WARN, "File unlock error!");
}



/*
 *  lock file
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool LockFile(FILE *File, char *Filepath)
{
  _Bool         Flag = False;      /* return value */
  int           FileDescriptor;
  int           Value;

  /* sanity check */
  if (File == NULL) return Flag;

  FileDescriptor = fileno(File);          /* get fd */

  /* we use lockf since it's related to fcntl */
  Value = lockf(FileDescriptor, F_TLOCK, 0);  /* try to lock */

  if (Value == 0)          /* success */
  {
    Flag = True;
  }
  else if (Value == -1)    /* error */
  {
    if (Filepath) Log(L_WARN, "Couldn't lock file (%s)!", Filepath);
  }

  return Flag;
}




/*
 *  Check if a path is a mounting point for a filesystem, i.e.
 *  if a filesystem is currently mounted at that path.
 *
 *  requires:
 *  - path of mounting point
 *
 *  returns:
 *  - 1 if filesystem is mounted
 *  - 0 on error or if fs is not mounted
 */

_Bool IsMountingPoint(char *Path)
{
  _Bool             Flag = False;      /* return value */
  struct stat       FileData;
  dev_t             DeviceID;
  size_t            n;
  char              *ParentPath = NULL;
  char              *ErrPath = NULL;

  /* sanity check */
  if (Path == NULL) return Flag;

  /* remove trailing slashes */
  n = strlen(Path);
  if (n > 0) n--;                  /* adjust for index */
  while ((n > 0) && (Path[n] == '/'))
  {
    Path[n] = 0;
    n--;
  }

  /* '/' is a mounting point by default */
  if (n == 0) return Flag = True;

  /* first get device ID of filesystem of mounting point */ 
  if (stat(Path, &FileData) == 0)
  {
    DeviceID = FileData.st_dev;

    /* get parent directory */
    ParentPath = CopyString(Path);

    /* find next slash in path */
    n = strlen(ParentPath);
    if (n > 0) n--;                /* adjust for index */    
    while ((n > 0) && (ParentPath[n] != '/'))
    {
      ParentPath[n] = 0;
      n--;
    }

    if (n > 0) ParentPath[n] = 0;       /* remove trailing slash */

    /* the parent directory should have a different device ID */
    if (stat(ParentPath, &FileData) == 0)
    {
      if (FileData.st_dev != DeviceID) Flag = True;
    }
    else            /* stat() failed */
    {
      ErrPath = ParentPath;
    }
  }
  else              /* stat() failed */
  {
    ErrPath = Path;
  }

  /* log errors */
  if (ErrPath != NULL) Log(L_WARN, "Couldn't stat() path (%s)!", ErrPath);

  /* clean up */
  if (ParentPath != NULL) free(ParentPath);

  return Flag;
}



/* ************************************************************************
 *   filename/path functions
 * ************************************************************************ */


/*
 *  get filename from filepath
 *
 *  returns:
 *  - pointer to name
 *  - NULL on any problem
 */

char *GetFilename(char *Filepath)
{
  char              *Name = NULL;

  /* sanity check */
  if (Filepath == NULL) return Name;

  /* simply look for rightmost "/" in filepath */
  while (Filepath[0] != 0)
  {
    if (Filepath[0] == '/')
    {
      Name = Filepath;        /* save ptr */
    }

    Filepath++;               /* next char */
  }

  /* move pointer to first char behind "/" */
  if (Name)
  {
    Name++;                             /* next char */
    if (Name[0] == 0) Name = NULL;      /* string end */
  }

  return Name;
}



/* ************************************************************************
 *   parser support functions
 * ************************************************************************ */


/*
 *  search keyword list for known keyword
 *
 *  requires:
 *  - array of char pointers terminated by a NULL pointer
 *
 *  returns:
 *  - keyword ID on success
 *  - 0 on error
 */

unsigned short GetKeyword(char **Keywords, char *String)
{
  unsigned short    Keyword = 0;        /* return value */
  unsigned short    n = 1;              /* keyword counter */

  /* sanity checks */
  if ((Keywords == NULL) || (String == NULL)) return Keyword;

  /* loop through keywords */
  while ((n > 0) && (*Keywords != NULL))
  {
    if (strcasecmp(String, *Keywords) == 0)    /* got match */
    {
      Keyword = n;        /* save keyword number */
      n = 0;              /* end loop */
    }
    else                                       /* no match */
    {
      n++;
      Keywords++;         /* next keyword */
    }
  }

  return Keyword;
}



/* ************************************************************************
 *   clean-up of local constants
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef MISC_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
