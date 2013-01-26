/* ************************************************************************
 *
 *   misc support functions
 *
 *   (c) 2012 by Markus Reschke
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
    Log(L_ERR, "No memory!\n");
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
 *  returns:
 *  - 1 on success
 *  - 0 on any problem
 */

_Bool LongLong2ByteString(long long Bytes, char *Buffer, size_t Size)
{
  _Bool                Flag = False;             /* return value */
  unsigned int         Counter = 0;
  char                 Unit;

  /* sanity checks */
  if ((Buffer == NULL) || (Size < 10)) return Flag;

  /* break down large value */
  while (Bytes > 10240)
  {
    Bytes = Bytes / 1024;
    Counter++;
  }

  /* unit */
  switch (Counter)
  {
    case 1:
      Unit = 'k';
      break;
    case 2:
      Unit = 'M';
      break;
    case 3:
      Unit = 'G';
      break;
  }

  /* build string */
  if (Counter > 0)     /* with unit */
  { 
    if (snprintf(Buffer, 10, "%lld %cB", Bytes, Unit) > 0) Flag = True;
  }
  else                 /* no unit */
  {
    if (snprintf(Buffer, 10, "%lld B", Bytes) > 0) Flag = True;
  }

  return Flag;
}



/*
 *  convert string with number and byte unit into long long integer
 *
 *  returns:
 *  - value on success
 *  - -2 on any problem
 */


long long ByteString2LongLong(char *Token)
{
  long long         Bytes = -2;         /* return value */
  long              Value;
  char              *Test = NULL;       /* test pointer */

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
    else if (Test[0] == 0)           /* got valid value */
    {
      /* just digits, no unit */
      Bytes = Value;
    }
    else                             /* unit follows */
    {
      /* skip whitespaces */
      while ((Test[0] == ' ') || (Test[0] == '\t'))
      {
        Test++;
      }

      /* get unit */
      if (strlen(Test) == 2)           /* 2 chars left */
      {
        /* if second char is a "B" for bytes */
        if ((Test[1] == 'b') || (Test[1] == 'B'))
        {
          /* then the first char is the multiplier */
          switch (Test[0])
          {
            case 'k':
              Bytes = Value * 1024;
              break;
            case 'M':
              Bytes = Value * 1024 * 1024;
              break;
            case 'G':
              Bytes = Value * 1024 * 1024 * 1024;
              break;
          }
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
