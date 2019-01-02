/* ************************************************************************
 *
 *   mfreq-srif (SRIF compatible frequest handler based on fsc-0086.001)
 *
 *   (c) 1994-2018 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define MFREQ_SRIF_C


/* defaults for this program */
#define NAME            "mfreq-srif"
#define CFG_FILENAME    "srif.cfg"


/*
 *  include header files
 */


/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */

/* strings */
#include <ctype.h>

/* files */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>


/*
 *  more local constants
 */

/* update configuration path */
#ifndef CFG_PATH
  #define CFG_PATH       DEFAULT_CFG_PATH
#endif

/* build configuration filepath */
#define CFG_FILEPATH     CFG_PATH"/"CFG_FILENAME

/* update tmp path */
#ifndef TMP_PATH
  #define TMP_PATH       DEFAULT_TMP_PATH
#endif


/*
 *  local variables
 */

/* buffers */
char                *InBuffer2 = NULL;       /* for file reading */
char                *AliasBuffer = NULL;     /* for aliases */

/* counters */
int                 BadPWs = 0;              /* number of bad passwords */



/* ************************************************************************
 *   file handling
 * ************************************************************************ */


/*
 *  open logfile
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool OpenLogfile()
{
  _Bool                  Flag = False;        /* return value */

  /* sanity check */
  if (Env->LogFilepath == NULL) return Flag;

  Env->Log = fopen(Env->LogFilepath, "a");        /* append mode */

  if (Env->Log != NULL)    /* success */
  {
    Flag = True;
  }
  else                     /* error */
  {
    Log(L_WARN, "Couldn't open logfile (%s)!", Env->LogFilepath);
  }

  Log(L_INFO, NAME" "VERSION" started.");      /* log: start */

  return Flag;
}



/*
 *  get filesize
 *
 *  returns:
 *  - size on success
 *  - -1 on error
 */

off_t GetFileSize(char *Filepath)
{
  off_t              Size = -1;           /* return value */
  struct stat        FileData;

  /* sanity check */
  if (Filepath == NULL) return Size;

  if (lstat(Filepath, &FileData) == 0)
  {
    if (S_ISREG(FileData.st_mode))        /* regular file */
    {
      Size = FileData.st_size;
    }
  }

  return Size;
}



/* ************************************************************************
 *   file index alias
 * ************************************************************************ */


/*
 *  get path alias from index alias file
 *
 *  Warning: uses global buffer InBuffer2 to return result
 *
 *  returns:
 *  -  pointer to string on success
 *  -  NULL on error
 */

char *GetAlias(FILE *AliasFile, off_t Offset)
{
  char              *Path = NULL;       /* return value */
  size_t            Length;

  /* sanity check */
  if ((AliasFile == NULL) || (Offset < 0)) return Path;

  /* move to offset position */
  fseeko(AliasFile, Offset, SEEK_SET);

  /* read path from current line */
  if (fgets(InBuffer2, DEFAULT_BUFFER_SIZE, AliasFile) != NULL)
  {
    Length = strlen(InBuffer2);

    /* sanity check for linesize */
    if ((Length > 0) && (Length < DEFAULT_BUFFER_SIZE - 1))
    {
      /* remove LF at end of line */
      if (InBuffer2[Length - 1] == 10) InBuffer2[Length - 1] = 0;

      /* if it's not an empty */
      if (InBuffer2[0] != 0) Path = InBuffer2;
    }
  }

  if (Path == NULL)
    Log(L_WARN, "Broken path alias at offset %ld)!", Offset);

  return Path;
}



/*
 *  process path alias (at strings beginning) 
 *
 *  Warning: uses global buffer AliasBuffer to return result
 *
 *  returns:
 *  -  pointer to string on success
 *  -  NULL on error
 */

char *ProcessAlias(FILE *AliasFile, char *String)
{
  char              *Buffer = NULL;          /* return value */
  char              *Alias, *Remainder;
  char              *Help, *Path;
  off_t             Offset;

  /* sanity check */
  if ((AliasFile == NULL) || (String == NULL)) return Buffer;

  if (String[0] == '%')       /* alias starts with % */
  {
    Alias = String;
    Alias++;                  /* assumed alias */

    /* find end of alias (second %) */
    Help = Alias;
    while ((Help[0] != 0) && (Help[0] != '%')) Help++;

    if (Help[0] == '%')       /* found end of alias */
    {
      Help[0] = 0;                 /* create substring */
      Remainder = Help;
      Remainder++;                 /* /filename */

      Offset = Str2Long(Alias);    /* convert */
      if (Offset >= 0)             /* valid values */
      {
        /* get alias from alias file */
        Path = GetAlias(AliasFile, Offset);

        /* replace alias */
        if (Path)
        {
          snprintf(AliasBuffer, DEFAULT_BUFFER_SIZE - 1,
            "%s%s", Path, Remainder);
          Buffer = AliasBuffer;
        }
      }

      Help[0] = '%';          /* undo change */

      if (Buffer == NULL)
        Log(L_WARN, "Path alias error for %s)!", String);        
    }
  }

  return Buffer;
}



/* ************************************************************************
 *   file index lookup
 * ************************************************************************ */


/*
 *  parse line of lookup file
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ParseIndexLookup(Token_Type *Tokens)
{
  _Bool             Flag = False;       /* return value */
  char              Letter;
  long              Value;
  off_t             Offset;
  unsigned int      Start, Stop;

  /* sanity check */
  if (Tokens == NULL) return Flag;

  /* format: <char> <offset> <start line#> <stop line#> */

  /* get char */
  if (Tokens->String)                   /* sanity check */
  {
    if (strlen(Tokens->String) == 1)    /* single character */
    {
      Letter = Tokens->String[0];       /* save char */
      Tokens = Tokens->Next;            /* next token */
      Flag = True;                      /* ok to proceed */
    }
  }

  /* offset */
  if (Flag)
  {
    Flag = False;                       /* reset flag */

    if (Tokens && Tokens->String)       /* sanity checks */
    {
      Value = Str2Long(Tokens->String);      /* convert */
      if (Value >= 0)                   /* valid value */
      {
        Offset = (off_t)Value;          /* attention: INT_MAX */
        Tokens = Tokens->Next;          /* next token */
        Flag = True;                    /* ok to proceed */        
      }
    }
  }

  /* start line# */
  if (Flag)
  {
    Flag = False;                       /* reset flag */

    if (Tokens && Tokens->String)       /* sanity checks */
    {
      Value = Str2Long(Tokens->String);      /* convert */
      if (Value > 0)                    /* valid value */
      {
        Start = (unsigned int)Value;    /* attention: INT_MAX */
        Tokens = Tokens->Next;          /* next token */
        Flag = True;                    /* ok to proceed */        
      }
    }
  }

  /* stop line# */
  if (Flag)
  {
    Flag = False;                       /* reset flag */

    if (Tokens && Tokens->String)       /* sanity checks */
    {
      Value = Str2Long(Tokens->String);      /* convert */
      if (Value > 0)                    /* valid value */
      {
        Stop = (unsigned int)Value;     /* attention: INT_MAX */

        if (Stop >= Start)              /* sanity check */
        {
          if (Tokens->Next == NULL)     /* no other token expected */
          {
            Flag = True;                /* ok to proceed */
          }
        }       
      }
    }
  }

  /* add lookup element to list */
  if (Flag)
  {
    Flag = AddLookupElement(Letter, Offset, Start, Stop);
  }

  return Flag;
}



/*
 *  read lookup file
 *
 *  format: <first char> <offset> <start line#> <stop line#>LF
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ReadIndexLookup(char *Filepath)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* loop control */
  FILE              *File;              /* filestream */
  size_t            Length;
  Token_Type        *TokenList = NULL;

  /* sanity check */
  if (Filepath == NULL) return Flag;

  /* open file read it line-wise */
  File = fopen(Filepath, "r");         /* read mode */
  if (File)
  {
    while (Run)
    {
      /* read line-wise */
      if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
      {
        Length = strlen(InBuffer);

        if (Length == 0)                  /* sanity check */
        {
          Run = False;                         /* end loop */
        }
        else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
        {
          /* now check if line matches buffer size exacly or exceeds it */
          /* exact matches should have a LF as last character in front of the trailing 0 */
          if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
          {
            Run = False;                         /* end loop */
            Log(L_INFO, "Input overflow for lookup file!");
          }
        }

        if (Run)       /* if still in business */
        {
          Flag = False;

          /* remove LF at end of line */
          if (InBuffer[Length - 1] == 10)
          {
            InBuffer[Length - 1] = 0;
            Length--;
          }

          /* if it's not an empty */
          if (InBuffer[0] != 0)
          {
            /* tokenize line and call parser */
            TokenList = Tokenize(InBuffer);       /* tokenize line */
            if (TokenList)
            {
              Flag = ParseIndexLookup(TokenList);
              FreeTokenlist(TokenList);              /* free list */
              TokenList = NULL;
            }
          }

          if (Flag == False)                       /* parsing error */
          {
            Log(L_WARN, "Syntax error in index lookup file (%s)!", Filepath);
            Run = False;        /* end loop */
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;               /* end loop */

        /* check for error */
        if (ferror(File) != 0)
        {
          Flag = False;            /* signal problem */
          Log(L_WARN, "Read error for index lookup file (%s)!", Filepath);
        }
      }
    }

    fclose(File);           /* close file */
  }
  else
  {
    Log(L_WARN, "Couldn't open index lookup file (%s)!", Filepath);
  }

  return Flag;
}



/*
 *  return lookup element for a specific character
 *
 *  returns:
 *  - pointer on success
 *  - NULL on error
 */

IndexLookup_Type *GetLookupElement(IndexLookup_Type *List, char Char)
{
  IndexLookup_Type       *Element = NULL;    /* return value */

  /* sanity check */
  if ((List == NULL) || (Char == 0)) return Element;

  while (List)                        /* follow linked list */
  {
    if (List->Letter == Char)           /* got a match */
    {
      Element = List;                     /* save pointer */
      List = NULL;                        /* end loop */
    }
    else                                /* no match */
    {
      List = List->Next;                  /* go to next element */
    }
  }

  return Element;
}



/*
 *  return offset for a specific character
 *
 *  returns:
 *  - offset on success
 *  - -1 on error
 */

long GetLetterOffset(IndexLookup_Type *List, char Char)
{
  long              Offset = -1;        /* return value */

  /* sanity check */
  if ((List == NULL) || (Char == 0)) return Offset;

  while (List)                        /* follow linked list */
  {
    if (List->Letter == Char)           /* got a match */
    {
      Offset = List->Offset;              /* save value */
      List = NULL;                        /* end loop */
    }
    else                                /* no match */
    {
      List = List->Next;                  /* go to next element */
    }
  }

  return Offset;
}



/* ************************************************************************
 *   request processing
 * ************************************************************************ */


/*
 *  log file request in more detail
 */

void LogRequest()
{
  Request_Type           *Request;      /* file(s) requested */
  Response_Type          *Response;     /* file found */
  char                   *Help;         /* temporary string */

  Request = Env->RequestList;           /* first request */

  while (Request)                       /* follow requests */
  {
    /* requested filename pattern */
    if (Request->PW)                    /* with password */
    {
      Log(L_INFO, "FReq: %s !%s", Request->Name, Request->PW);
    }
    else                                /* without password */
    {
      Log(L_INFO, "FReq: %s", Request->Name);
    }


    Response = Request->Files;          /* first file */

    while (Response)                    /* follow files */
    {
      Help = GetFilename(Response->Filepath);   /* get pointer to filename */
      if (Help == NULL)                         /* error */
      {
        Help = Response->Filepath;              /* use filepath instead */
      }

      if (Help)       /* sanity check */
      {
        /* file is ok */
        if (Response->Status & RESP_OK)
        {
          if (Bytes2String(Response->Size, TempBuffer, DEFAULT_BUFFER_SIZE - 1))
            Log(L_INFO, "- %s (%s)", Help, TempBuffer);
        }

        /* password error */
        else if (Response->Status & RESP_PWERROR)
        {
          Log(L_INFO, "- PW Error (%s)", Help);
        }

        /* file offline */
        else if (Response->Status & RESP_OFFLINE)
        {
          Log(L_INFO, "- Offline (%s)", Help);
        }

        /* file dupe */
        else if (Response->Status & RESP_DUPE)
        {
          Log(L_INFO, "- Dupe (%s)", Help);
        }
      }

      Response = Response->Next;        /* next file */
    }

    /* no files found */
    if (Request->Status & FREQ_NO_FILE)
    {
      Log(L_INFO, "- nothing found", Help);
    }

    /* ignored */
    else if (Request->Status == FREQ_NONE)
    {
      Log(L_INFO, "- ignored", Help);
    }

    Request = Request->Next;            /* next request */
  }
}



/*
 *  write textmail
 *
 *  returns:
 *  - 1 on success (if any match is found)
 *  - 0 on error
 */

_Bool TextMail()
{
  _Bool             Flag = False;       /* return value */
  FILE              *File = NULL;
  unsigned short    n = 0;
  unsigned int      Value;

  /* create filepath for text file */
  if (Env->TextFilepath == NULL)
  {
    Value = (unsigned int)Env->UnixTime;

    while (n < 10)          /* try up to 10 times */
    {
      /* use unix time plus offset */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s/resp%04x.txt", Env->MailPath, Value & 0xFFFF);

      /* check if file exits */
      /* lstat() might be better solution */
      File = fopen(TempBuffer, "r");       /* read mode */
      if (File)               /* file exists **/
      {
        fclose(File);           /* close */
        Value++;
        n++;                    /* another loop run */
      }
      else                    /* file not available */
      {
        Env->TextFilepath = CopyString(TempBuffer);    /* use this filepath */
        n = 10;                 /* end loop */
      }
    }

    if (Env->TextFilepath == NULL)
    {
      Log(L_WARN, "Couldn't create usable textmail filepath!");
      return Flag;
    }
  }

  /* write feedback */
  File = fopen(Env->TextFilepath, "w");       /* truncate & write mode  */
  if (File)
  {
    Flag = WriteMailContent(File);         /* write response mail */
    fclose(File);                          /* close file */
  }
  else
  {
    Log(L_WARN, "Couldn't open textmail (%s)!", Env->TextFilepath);
  }

  /* clean up */
  if (!Flag)        /* on error */
  {
    /* try to remove broken netmail */
    if (File)
    {
      unlink(Env->TextFilepath);
    }

    /* reset filepath to prevent sending of broken file */
    if (Env->TextFilepath)
    {
      free(Env->TextFilepath);
      Env->TextFilepath = NULL;
    }

    /* log problem */
    Log(L_WARN, "Couldn't create textmail!");
  }

  return Flag;
}



/*
 *  write response (list of files to be send)
 *
 *  returns:
 *  - 1 on success (if any match is found)
 *  - 0 on error
 */

_Bool WriteResponse()
{
  _Bool                  Flag = False;      /* return value */
  _Bool                  Run = True;        /* control flag */
  FILE                   *File;             /* file stream */
  Request_Type           *Request;          /* requests */
  Response_Type          *Response;         /* files found */

  /*
   *  file list syntax: <flag><filepath>
   *  flag:
   *  =  erase file if sent successfully
   *  +  do not erase the file after sent
   *  -  erase the file in any case after session
   */

  /* sanity check */
  if (Env->ResponseFilepath == NULL) return Flag;

  /* open file */
  File = fopen(Env->ResponseFilepath, "w");           /*  truncate & write mode  */
  if (File)           /* file opened */
  {
    Request = Env->RequestList;            /* start of list */

    while (Run && Request)                 /* follow linked list */
    {
      Response = Request->Files;             /* start of list */

      while (Run && Response)                /* follow linked list */
      {
        /* list valid files only */
        if (Response->Filepath && (Response->Status & RESP_OK))
        {
          /* write filepath to response file */

          snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
            "+%s\n", Response->Filepath);

          if (fputs(OutBuffer, File) < 0)    /* write error */
          {
            Run = False;
            Log(L_WARN, "Write error for response file (%s)!", Env->ResponseFilepath);
          }
        }

        Response = Response->Next;      /* next element */
      }

      Request = Request->Next;        /* next element */
    }

    /* add netmail packet */
    if (Run && Env->MailFilepath)
    {
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "=%s\n", Env->MailFilepath);
      if (fputs(OutBuffer, File) < 0)    /* write error */
      {
        Run = False;
        Log(L_WARN, "Write error for response file (%s)!", Env->ResponseFilepath);
      }
    }

    /* add text feedback */
    if (Run && Env->TextFilepath)
    {
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "=%s\n", Env->TextFilepath);
      if (fputs(OutBuffer, File) < 0)    /* write error */
      {
        Run = False;
        Log(L_WARN, "Write error for response file (%s)!", Env->ResponseFilepath);
      }
    }

    fclose(File);             /* close file */ 
    if (Run) Flag = True;     /* signal success */
  }
  else                /* file error */
  {
    Log(L_WARN, "Couldn't open response file (%s)!", Env->ResponseFilepath);
  }

  /* log problem */
  if (!Flag) Log(L_WARN, "Couldn't create response file!");

  return Flag;
}



/*
 *  look for best AKA pair to use (for netmail etc.)
 *
 *  returns:
 *  - 1 on success (if any match is found)
 *  - 0 on error
 */

_Bool ActivateAKAs()
{
  _Bool                  Flag = False;     /* return value */
  _Bool                  Run = True;       /* control flag */
  AKA_Type               *Remote, *Local;

  if (Env->CalledAKA)     /* if called a specific AKA we'll use that one */
  {
    Env->ActiveLocalAKA = Env->CalledAKA;

    /* and try to find a match for the requester */
    Remote = Env->ReqAKA; 
    while (Remote)                    /* follow list */
    {
      if (MatchAKAs(Env->ActiveLocalAKA, Remote))
      {
        Env->ActiveRemoteAKA = Remote;
        Remote = NULL;                      /* end loop */
      }
      else
      {
        Remote = Remote->Next;              /* next one */
      } 
    }

    /* if no match found use the first remote AKA */
    if (Env->ActiveRemoteAKA == NULL)
    {
      Env->ActiveRemoteAKA = Env->ReqAKA;
    }
  }
  else                    /* otherwise find a match */
  {
    Local = Env->AKA;
    while (Run && Local)           /* follow list */
    {
      Remote = Env->ReqAKA;
      while (Run && Remote)          /* follow list */
      {
        if (MatchAKAs(Local, Remote))
        {
          Env->ActiveLocalAKA = Local;
          Env->ActiveRemoteAKA = Remote;
          Run = False;                   /* end loops */
        }

        Remote = Remote->Next;         /* next one */
      }

      Local = Local->Next;           /* next one */
    }

    /* if no match found use the first AKAs */
    if (Run)
    {
      Env->ActiveLocalAKA = Env->AKA;
      Env->ActiveRemoteAKA = Env->ReqAKA;
    }
  }

  /* check if we got the job done */
  if (Env->ActiveLocalAKA && Env->ActiveRemoteAKA) Flag = True;

  return Flag;
}



/*
 *  search request limit rules for specific address
 *  and activate that rule
 *
 *  returns:
 *  - 1 on success (if any match is found)
 *  - 0 on error
 */

_Bool ActivateLimits()
{
  _Bool                  Flag = False;     /* return value */
  AKA_Type               *AKA;
  Limit_Type             *Limit;

  /* take first match */

  AKA = Env->ReqAKA;

  while((Flag == False) && AKA)    /* follow AKA list */
  {
    Limit = Env->LimitList;

    while (Limit)                    /* follow limit list */
    {
      if (MatchPattern(AKA->Address, Limit->Address))  /* match */
      {
        Flag = True;                    /* signal match */

        /* check also flags */
        if (Limit->Flags & REQ_LISTED)    /* node has to be listed */
        {
          if (!(Env->ReqFlags & REQ_LISTED)) Flag = False;
        }

        if (Flag)                  /* if still ok */
        {
          Env->ActiveLimit = Limit;        /* active this one */
          Limit = NULL;                    /* end loop */
        }
      }
      else                             /* no match */
      {
        Limit = Limit->Next;             /* next element */
      }
    }

    AKA = AKA->Next;                 /* next element */
  }

  /* log limits */
  if (Env->ActiveLimit)
  {
    if (Bytes2String(Env->ActiveLimit->Bytes, TempBuffer, DEFAULT_BUFFER_SIZE))
      Log(L_INFO, "Limits used: %ld files / %s / %d BadPWs / %d Freqs",
          Env->ActiveLimit->Files, TempBuffer, Env->ActiveLimit->BadPWs,
          Env->ActiveLimit->Freqs);
  }

  return Flag;
}



/*
 *  search for matches in index data file
 *  - linear search algorithm
 *  - starts at pre-set offset position
 *
 *  requires:
 *  - Pos: position of first wildcard (-1: no wildcards)
 *
 *  returns:
 *  - 1 on success (if any or no matches are found)
 *  - 0 on error
 */

_Bool SearchIndex(FILE *DataFile, FILE *AliasFile, Request_Type *Request, int Pos)
{
  _Bool                  Flag = True;         /* return value */
  _Bool                  Run = True;          /* loop control */
  _Bool                  Match = False;       /* match flag */
  size_t                 Length;              /* string length */
  char                   *Requested;          /* requested file pattern */
  char                   *Help;               /* temporary string */
  char                   *Name, *Filepath, *Password;
  int                    Check;               /* test value */
  Response_Type          *Response;           /* response element */
  
  /* sanity checks */
  if ((DataFile == NULL) ||
      (AliasFile == NULL) ||
      (Request == NULL))
    return False;

  Requested = Request->SearchName;
  if (Requested == NULL) return False;

  while (Run)                 /* processing loop */
  {
    /* reset variables */
    Name = NULL;
    Filepath = NULL;
    Password = NULL;
    Match = False;


    /*
     *  read line-wise
     */

    if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, DataFile) != NULL)
    {
      Length = strlen(InBuffer);

      if (Length == 0)                  /* sanity check */
      {
        Run = False;                         /* end loop */
        Flag = False;
      }
      else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
      {
        /* now check if line matches buffer size exacly or exceeds it */
        /* exact matches should have a LF as last character in front of the trailing 0 */
        if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
        {
          Run = False;                         /* end loop */
          Flag = False;
          Log(L_WARN, "Input overflow for request file!");
        }
      }

      if (Run)       /* if still in business */
      {
        /* remove LF at end of line */
        if (InBuffer[Length - 1] == 10)
        {
          InBuffer[Length - 1] = 0;
          Length--;
        }

        /* if it's not empty */
        if (InBuffer[0] != 0)
        {
          /*
           *  parse line
           *  format: <name><sep><filepath>[<sep><password>]
           *  sep: ascii 31 (unit separator, octal 037)
           */

          Help = InBuffer;
          Name = InBuffer;          /* start of string */

          /* get name */
          while ((Help[0] != 0) && (Help[0] != 31)) Help++;
          if (Help[0] == 31)
          {
            Help[0] = 0;            /* create substring */
            Help++;
            Filepath = Help;        /* start of string */

            /* get filepath */
            while ((Help[0] != 0) && (Help[0] != 31)) Help++;
            if (Help[0] == 31)      /* PW follows */
            {
              Help[0] = 0;            /* create substring */
              Help++;
              Password = Help;        /* start of string */
            }
          }
          else                        /* syntax error */
          {
            Name = NULL;
          }
        }
      }
    } 
    else                     /* EOF or error */
    {
      Run = False;        /* end loop */
    }


    /*
     *  compare
     */

    if (Run && Name && Filepath)
    {
      /* select best search algorithm based on wildcard position */

      /*
       *  filename pattern without any wildcard:
       *  - simple string compare
       *  - stop when index data > request
       */

      if (Pos == -1)               /* no wildcard at all */
      {
        Check = strcmp(Name, Requested);
        if (Check == 0)            /* match */
        {
          Match = True;            /* add file */
        }
        else if (Check > 0)        /* index data > request */
        {
          Run = False;             /* end loop */
        }
      }

      /*
       *  filename pattern starting with some char(s) and a wildcard:
       *  - simple string compare for first part of request
       *  - on match perform pattern matching for remaining part
       *  - stop when index data > first part of request 
       */
      
      else if (Pos > 0)            /* wildcard follows */
      {
        Check = strncmp(Name, Requested, Pos);
        if (Check == 0)               /* first part matches */
        {
          /* perform pattern matching */
          if (MatchPattern(Name, Requested))      /* match */
          {
            Match = True;             /* add file */
          }
        }
        else if (Check > 0)           /* index data > request */
        {
          Run = False;                /* end loop */
        }
      }

      /*
       *  filename pattern starts with a wildcard:
       *  - perform pattern matching
       *  - stop when we reach EOF (done by master loop anyway)
       */

      else if (Pos == 0)           /* first char is a wildcard */
      {
        if (MatchPattern(Name, Requested))   /* match */
        {
          Match = True;            /* add file */
        }
      }
    }


    /*
     *  prepare processing of matching file
     *  - filepath
     *  - response element
     */

    if (Match)           /* got match */
    {
      /*
       *  <filepath>: <path>/[<filename>]
       *              %<alias offset>%/[filename]
       *  - %<alias offset>% for automatic path aliasing
       *  - <filename> can be omitted if same as <name>
       */

      /* automatic filepath */
      Length = strlen(Filepath);
      if (Filepath[Length - 1] == '/')       /* filename is missing */
      {
        /* add filename to path */
        snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1,
          "%s%s", Filepath, Name);
        Filepath = TempBuffer2;
      }

      /* check for path alias */
      if (Filepath[0] == '%')      /* alias starts with % */
      {
        /* process alias and use result on success */
        Help = ProcessAlias(AliasFile, Filepath);
        if (Help) Filepath = Help;
      }

      /* create response element and add it to the request */
      Response = CreateResponseElement(Filepath);
      if (Response)                /* got element */
      {
        if (Request->LastFile) Request->LastFile->Next = Response;
        else Request->Files = Response;
        Request->LastFile = Response;
      }
      else                         /* error */
      {
        Match = False;             /* skip file */
      }
    }


    /*
     *  check for any duplicate in current request
     *  - caused by AnyCase search
     *  - mark file if it's a duplicate
     */

    if (Match)           /* proceed with match */
    {
      if (DuplicateResponse(Request, Response))   /* is duplicate */
      {
        Response->Status |= RESP_INTDUPE;    /* set flag */
        Match = False;                       /* skip file */
      }
    }


    /*
     *  check password (if required)
     *  - mark file in case of a bad PW
     *  - check limit for bad PWs
     */

    if (Match && Password)
    {
      /* check if the request password matches the index one */
      if ((Request->PW == NULL) || (strcmp(Request->PW, Password) != 0))
      {
        Response->Status |= RESP_PWERROR;    /* set flag */
        Match = False;                       /* skip file */

        /* log PW error if not done later on */
        if (!(Env->CfgSwitches & SW_LOG_REQUEST))
        {
          Help = GetFilename(Response->Filepath);
          if (Help == NULL) Help = Response->Filepath;

          if (Request->PW)
            Log(L_INFO, "PW error: %s (req: %s !%s)", Help, Request->Name, Request->PW);
          else
            Log(L_INFO, "PW error: %s (req: %s)", Help, Request->Name);
        }

        /* manage limit */
        BadPWs++;                       /* another bad PW */

        if (Env->ActiveLimit)           /* sanity check */
        {
          /* check if limit for bad PWs is exceeded */
          if ((Env->ActiveLimit->BadPWs >= 0) &&
              (BadPWs > Env->ActiveLimit->BadPWs))
          {
            /* update global frequest status: PW limit exceeded */
            Env->FreqStatus |= FREQ_PWLIMIT | FREQ_LIMIT;

            Run = False;                     /* end loop */
          }
        }
      }
    }


    /*
     *  check for any duplicate in all requests
     *  - to prevent sending the same file twice
     *  - mark file if it's a duplicate
     */

    if (Match)
    {
      if (AnyDuplicateResponse(Response))    /* is duplicate */
      {
        Request->Status = FREQ_FOUND_FILE;   /* got a file */
        Response->Status |= RESP_DUPE;       /* file is a dupe */
        Match = False;                       /* skip file */
      }
    }


    /*
     *  check file and request limits
     */

    if (Match)                /* passed pre-processing */
    {
      Response->Size = GetFileSize(Response->Filepath);   /* get file size */

      if (Response->Size > -1)          /* got file size */
      {
        Env->Bytes += Response->Size;   /* add to global counter */
        Env->Files++;                   /* increase global counter */
 
        if (Env->ActiveLimit)      /* sanity check */
        {
          /* check if file number limit is exceeded */
          if ((Env->ActiveLimit->Files >= 0) &&
              (Env->Files > Env->ActiveLimit->Files))
          {
            /* update global frequest status; file limit exceeded */
            Env->FreqStatus |= FREQ_FILELIMIT | FREQ_LIMIT;

            Match = False;                   /* skip file */
            Run = False;                     /* end loop */
          }

          /* check if byte limit is exceeded */
          if ((Env->ActiveLimit->Bytes >= 0) &&
              (Env->Bytes > Env->ActiveLimit->Bytes))
          {
            /* update global frequest status: byte limit exceeded */
            Env->FreqStatus |= FREQ_BYTELIMIT | FREQ_LIMIT;

            Match = False;                   /* skip file */
            Run = False;                     /* end loop */
          }

          if (!Run)           /* some limit exceeded */
          {
            /* correct global counters */
            Env->Bytes -= Response->Size;
            Env->Files--;
          }
        }
      }
      else                                 /* error */
      {
        Request->Status = FREQ_FOUND_FILE;   /* got a file */
        Response->Status |= RESP_OFFLINE;    /* currently not available */
      }
    }


    if (Match)                /* passed all checks */
    {
      if (Response->Status == RESP_NONE)     /* not set yet */
      {
        Response->Status |= RESP_OK;         /* ok to send */
      }

      Request->Status = FREQ_FOUND_FILE;     /* found a file */
    }
  }

  return Flag;
}



/*
 *  pre-search start position in index data file
 *  - binary search algorithm
 *  - sets the file offset for the real search
 *  - doesn't support case-insensitive search
 *
 *  requires:
 *  - Request: requested file/pattern
 *  - Pos: position of first wildcard (-1: no wildcards)
 *  - Letter: first char
 *
 *  returns:
 *  - offset on success
 *  - -2 if no match is found
 *  - -1 on error
 *
 */

off_t BinaryPreSearch(FILE *DataFile, FILE *OffsetFile, char *Request, int Pos, char Letter)
{
  off_t             Offset = -1;             /* return value */
  _Bool             Run = False;             /* control flag */
  IndexLookup_Type  *LookupElement;          /* lookup element */
  unsigned int      Start;                   /* lower filenumber */
  unsigned int      Stop;                    /* upper filenumber */
  unsigned int      Middle;                  /* middle filenumber */
  unsigned int      Files;                   /* number of files */
  unsigned int      Length;                  /* string length */
  int               Check;
  off_t             TempOffset;              /* file offset */
  char              *HelpStr;

  /* sanity checks */
  if ((DataFile == NULL) ||
      (OffsetFile == NULL) ||
      (Request == NULL) ||
      (Pos == 0))
    return Offset;


  /*
   *  get data for first char from lookup list
   *  - lower and upper filenumbers limit the search range
   */

  LookupElement = GetLookupElement(Env->LookupList, Letter);

  if (LookupElement)               /* found char */
  {
    Start = LookupElement->Start;
    Stop = LookupElement->Stop;

    if (Stop >= Start)             /* sanity check */
    {
      Run = True;                  /* ok to proceed */
    }
  }

 /* todo:
    if lookup list isn't available we could fall back to the complete fileindex
    by getting the size of the offset file:
    fileno & fstat
    Start = 1 
    Stop = filesize/sizeof(off_t)
 */


  /*
   *  search loop
   */

  while (Run)
  {
    Middle = (Stop + Start) / 2;      /* determine middle */

    /*
     *  get offset for middle filename from offset file
     */

    Run = False;                      /* reset flag */
    TempOffset = sizeof(off_t) * (Middle - 1);     /* calculate offset */ 

    /* set file position */      
    if (fseeko(OffsetFile, TempOffset, SEEK_SET) == 0)
    {
      /* read offset stored */
      if (fread(&TempOffset, sizeof(off_t), 1, OffsetFile) == 1)
      {
        Run = True;         /* ok to proceed */
      }
    }


    /*
     *  get middle filename from data file
     */

    if (Run)
    {
      Run = False;                      /* reset flag */

      /* set file position */      
      if (fseeko(DataFile, TempOffset, SEEK_SET) == 0)
      {        
        /* read line */
        if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, DataFile) != NULL)
        {
          Length = strlen(InBuffer);

          if ((Length > 0) && (InBuffer[0] != 10))     /* not empty */
          {
            /* get filename */
            HelpStr = InBuffer;
            while ((HelpStr[0] != 0) && (HelpStr[0] != 31)) HelpStr++;

            if (HelpStr[0] == 31)     /* got field separator */
            {
              HelpStr[0] = 0;         /* create sub string */
              Run = True;             /* ok to proceed */
            }
          }
        }
      }
    }


    /*
     *  compare & search logic
     */

    if (Run)
    {
      Files = Stop - Start + 1;         /* number of files in span */

      /* compare strings */
      if (Pos > 0)                 /* up to position of wildcard */
      {
        Check = strncmp(InBuffer, Request, Pos);
      }
      else                         /* complete strings */
      {
        Check = strcmp(InBuffer, Request);
      }

      if (Check < 0)          /* filename < request */
      {
        Start = Middle;            /* proceed with upper half */
        if (Start < Stop) Start++; /* don't re-check middle */
      }
      else if (Check > 0)     /* filename > request */
      {
        Stop = Middle;             /* proceed with lower half */
        if (Stop > Start) Stop--;  /* don't re-check middle */
      }
      else                    /* filename = request */
      {
        Run = False;               /* end loop */
        Offset = TempOffset;       /* signal success */
      }

      /* just one file left */
      if (Files == 1)         /* break condition */
      {
        Run = False;                    /* end loop anyway */
      }
    }
  }

  return Offset;
}



/*
 *  process file request
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ProcessRequest()
{
  _Bool             Flag = True;             /* return value */
  _Bool             Run;                     /* loop control */
  _Bool             Result;                  /* result flag */
  _Bool             Limit = False;           /* limits exceeded */
  _Bool             BinSearch = False;       /* binary search */
  Index_Type        *Index;                  /* file index list */
  Request_Type      *Request;                /* file request list */
  FILE              *DataFile;               /* index data file */
  FILE              *AliasFile;              /* index alias file */
  FILE              *OffsetFile;             /* index offset file */
  char              *Help;                   /* support pointer */
  int               Pos;                     /* position of first wildcard */
                                             /* -1: no wildcard at all */
  off_t             Offset;                  /* file offset */
  char              Letter;                  /* first char */

  /* update flags based on configuration */
  if (Env->CfgSwitches & SW_BINARY_SEARCH) BinSearch = True;


  /*
   *  process all file indexes
   */

  Index = Env->IndexList;
  if (Index == NULL)
  {
    Log(L_WARN, "No fileindex specified!");
    Flag = False;                            /* signal error */
  }


  while (Index)               /* follow index list */
  {
    /* reset variables to defaults */
    Run = True;
    DataFile = NULL;
    AliasFile = NULL;
    OffsetFile = NULL;


    /*
     *  check index options 
     */

    if (Index->MountingPoint)      /* IfMounted option enabled */
    {
      /* check if filesystem is mounted */
      Run = IsMountingPoint(Index->MountingPoint);
      if (!Run)
      {
        Log(L_WARN, "Skipped index (%s): fs not mounted", Index->Filepath);
      }
    }


    /*
     *  open index files
     */

    if (Run)
    {
      Run = False;            /* reset flag */

      /* open data file */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s."SUFFIX_DATA, Index->Filepath);
      DataFile = fopen(TempBuffer, "r");          /* read mode */

      /* open alias file */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s."SUFFIX_ALIAS, Index->Filepath);
      AliasFile = fopen(TempBuffer, "r");         /* read mode */

      /* open offset file (BinarySearch) */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s."SUFFIX_OFFSET, Index->Filepath);
      OffsetFile = fopen(TempBuffer, "r");        /* read mode */

      /* check if we got all files */
      if (DataFile)
      {
        if (AliasFile)
        {
          if (OffsetFile)
          {
            /* read lookup file */
            snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
              "%s."SUFFIX_LOOKUP, Index->Filepath);
            Run = ReadIndexLookup(TempBuffer);
          }
          else
          {
            Log(L_WARN, "Couldn't open index offset file (%s)!", Index->Filepath);
          }
        }
        else
        {
          Log(L_WARN, "Couldn't open index alias file (%s)!", Index->Filepath);
        }
      }
      else
      {
        Log(L_WARN, "Couldn't open index data file (%s)!", Index->Filepath);
      }

      if (!Run) Flag = False;                /* signal error */
    }


    /*
     *  process each filerequest in the current file index
     */

    Request = Env->RequestList;       /* start of list */
    while (Run && Request)            /* follow request list */
    {
      /* init request status */
      if (Request->Status == FREQ_NONE)     /* no status yet */
      { 
        Request->Status = FREQ_NO_FILE;     /* "no file" by default */
      }

      if (Request->SearchName)              /* sanity check */
      {
        /* check if we got any wildcards */
        Help = Request->SearchName;
        Pos = 0;
        Offset = -1;

        /* find first wildcard */
        while ((Help[0] != 0) && (Help[0] != '*') && (Help[0] != '?'))
        {
          Help++;                /* next char */
          Pos++;
        }

        if (Help[0] == 0) Pos = -1;   /* reset position if end of line is reached */
                                      /* e.g. no wildcard at all */

        /* set position of data file to speed up search */
        if (Pos == 0)              /* first char of request is a wildcard */
        {
          Offset = 0;
        }
        else                       /* first char of request is no wildcard */
        {
          Letter = Request->SearchName[0];

          if (BinSearch)                /* binary pre-search */
          {
            Offset = BinaryPreSearch(DataFile, OffsetFile, Request->SearchName, Pos, Letter);
          }

          if (Offset == -1)        /* no or failed binary pre-search */
          {
            /* get offset from lookup list for first char */
            Offset = GetLetterOffset(Env->LookupList, Letter);
          }
        }

        /* search index data file */
        if (Offset >= 0)           /* valid offset */
        {
          /* set start position */
          if (fseeko(DataFile, Offset, SEEK_SET) == 0) 
          {
            Result = SearchIndex(DataFile, AliasFile, Request, Pos);
            if (!Result) Flag = False;            /* signal error */
          }
        }
      }

      /* any limits exceeded */
      if (Env->FreqStatus & FREQ_LIMIT)
      {
        Limit = True;              /* set flag */
        Run = False;               /* end loop (requests) */
      }

      Request = Request->Next;        /* next element */
    }

    /* clean up */
    if (OffsetFile) fclose(OffsetFile);  /* close offset file */
    if (AliasFile) fclose(AliasFile);    /* close alias file */
    if (DataFile) fclose(DataFile);      /* close data file */
    if (Env->LookupList)                 /* free lookup list */
    {
      FreeLookupList(Env->LookupList);
      Env->LookupList = NULL;
      Env->LastLookup = NULL;
    }

    if (Limit)                /* any limits exceeded */
    {
      Index = NULL;           /* end loop (indexes) */
    }
    else                      /* keep going */
    {
      Index = Index->Next;    /* next element */
    }
  }

  return Flag;
}



/*
 *  read request file (loosely based on FTS-0006)
 *
 *  format: <name/pattern> [!<password>]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ReadRequest()
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* loop control */
  FILE                   *File;               /* filestream */
  int                    Freqs = 0;           /* number of frequests */
  size_t                 Length;              /* string length */
  Token_Type             *TokenList = NULL;   /* token list */
  char                   *Password = NULL;    /* password */

  File = fopen(Env->RequestFilepath, "r");         /* read mode */
  if (File)
  {
    Flag = True;          /* signal success */

    while (Run)
    {
      /* read line-wise */
      if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
      {
        Length = strlen(InBuffer);

        if (Length == 0)                  /* sanity check */
        {
          Run = False;                         /* end loop */
        }
        else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
        {
          /* now check if line matches buffer size exacly or exceeds it */
          /* exact matches should have a LF as last character in front of the trailing 0 */
          if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
          {
            Run = False;                         /* end loop */
            Log(L_WARN, "Input overflow for request file!");
          }
        }

        if (Run)       /* if still in business */
        {
          /* remove LF at end of line */
          if (InBuffer[Length - 1] == 10)
          {
            InBuffer[Length - 1] = 0;
            Length--;
          }

          /* remove CR at end of line */
          if (InBuffer[Length - 1] == 13)
          {
            InBuffer[Length - 1] = 0;
            Length--;
          }

          /* if it's not an empty */
          if (InBuffer[0] != 0)
          {
            /* we expect: <name> [!<password>] */

            TokenList = Tokenize(InBuffer);          /* tokenize line */
            if (TokenList && TokenList->String)      /* sanity check */
            {
              Freqs++;                  /* another frequest */

              /* check if limit for frequests is exceeded */
              if (Env->ActiveLimit)           /* sanity check */
              {
                if ((Env->ActiveLimit->Freqs >= 0) &&
                    (Freqs > Env->ActiveLimit->Freqs))
                {
                  /* update global frequest status: frequest limit exceeded */
                  Env->FreqStatus |= FREQ_FREQLIMIT;

                  Run = False;            /* end loop */
                }
              }

              /* check for optional PW */
              if (TokenList->Next)                   /* got second token */
              {
                Password = TokenList->Next->String;

                if (Password && (Password[0] == '!'))
                {
                  Password++;      /* skip "!" */
                }
                else
                {
                  Password = NULL;
                  /* or should we allow passwords without "!" ? */ 
                }
              }

              /* we ignore any stuff following the password */

              if (Run)
              {
                /* add request to global list */
                AddRequestElement(TokenList->String, Password);
                Password = NULL;                     /* reset string */
              }

              FreeTokenlist(TokenList);              /* free list */
              TokenList = NULL;
            }
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;               /* end loop */

        /* check for error */
        if (ferror(File) != 0)
        {
          Log(L_WARN, "Read error for request file!");
        }
      }
    }

    fclose(File);           /* close file */

    /* log problem */
    if (!Flag) Log(L_WARN, "Couldn't read request file!");
  }
  else
  {
    Log(L_WARN, "Couldn't open request file (%s)!", Env->RequestFilepath);
  }

  return Flag;
}



/* ************************************************************************
 *   SRIF
 * ************************************************************************ */


/*
 *  parse SRIF
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ParseSRIF(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[12] =
    {"Sysop", "AKA", "Baud", "Time", "RequestList",
     "ResponseList", "RemoteStatus", "SystemStatus", "OurAKA", "CallerID",
     "SessionType", NULL};
  AKA_Type               *AKA;

  /* sanity checks */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Keyword > 0)        /* get value */
    {
      switch (Keyword)
      {
        case 1:     /* sysop */
          if (Env->Sysop)                    /* free old one */
          {
            free(Env->Sysop);
            Env->Sysop = NULL;
          }
          /* sysops name may have several parts (tokens) */
          Env->Sysop = UnTokenize(TokenList);
          break;
      
        case 2:     /* AKA */
          if ((AKA = NewAKA(TokenList->String)))  /* create AKA */
          {
            /* add to global list */
            if (Env->ReqLastAKA) Env->ReqLastAKA->Next = AKA;
            else Env->ReqAKA = AKA;
            Env->ReqLastAKA = AKA; 
          }
          break;

        case 3:     /* baud */
          Env->BPS = Str2Long(TokenList->String);
          break;

        case 4:     /* time */
          Env->ReqTime = Str2Long(TokenList->String);
          break;

        case 5:     /* request file */
          if (Env->RequestFilepath)          /* free old one */
          {
            free(Env->RequestFilepath);
            Env->RequestFilepath = NULL;
          }
          Env->RequestFilepath = TokenList->String;     /* move string */
          TokenList->String = NULL;
          break;

        case 6:     /* response file */
          if (Env->ResponseFilepath)         /* free old one */
          {
            free(Env->ResponseFilepath);
            Env->ResponseFilepath = NULL;
          }
          Env->ResponseFilepath = TokenList->String;    /* move string */
          TokenList->String = NULL;
          break;

        case 7:     /* remote status */
          /* check for keyword */
          if (strcasecmp(TokenList->String, "UNPROTECTED") == 0)
          {
            Env->ReqFlags |= REQ_UNPROTECTED;
          }
          else if (strcasecmp(TokenList->String, "PROTECTED") == 0)
          {
            Env->ReqFlags |= REQ_PROTECTED;
          }
          break;

        case 8:     /* system status */
          /* check for keyword */
          if (strcasecmp(TokenList->String, "UNLISTED") == 0)
          {
            Env->ReqFlags |= REQ_UNLISTED;
          }
          else if (strcasecmp(TokenList->String, "LISTED") == 0)
          {
            Env->ReqFlags |= REQ_LISTED;
          }
          break;

        case 9:     /* our AKA (optional) */
          if (Env->CalledAKA)                     /* free old AKA */
          {
            FreeAKAlist(Env->CalledAKA);
            Env->CalledAKA = NULL;
          }
          if ((AKA = NewAKA(TokenList->String)))  /* create AKA */
          {
            Env->CalledAKA = AKA;
          }
          break;

        case 10:    /* Caller ID (optional) */
          if (Env->CallerID)            /* free old one */
          {
            free(Env->CallerID);
            Env->CallerID = NULL;
          }
          /* in case the caller ID has several parts (tokens) */
          Env->CallerID = UnTokenize(TokenList);
          break;

        case 11:    /* session type (optional) */
          if (Env->SessionType)         /* free old one */
          {
            free(Env->SessionType);
            Env->SessionType = NULL;
          }
          /* copy session type */
          Env->SessionType = CopyString(TokenList->String);
          break;
      }

      Keyword = 0;             /* reset */
    }
    else                   /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0)       /* unknown keyword */
      {
        Run = False;               /* end loop */
        Log(L_WARN, "Unknown keyword in SRIF (%s)!", TokenList->String);
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  Since some statements are optional and even might lack data
   *  we assume everything's fine :)
   */

  Flag = True;

  return Flag;
}



/*
 *  read SRIF
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ReadSRIF()
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* loop control */
  FILE                   *File;               /* filestream */
  size_t                 Length;
  Token_Type             *TokenList;

  File = fopen(Env->SRIF_Filepath, "r");         /* read mode */

  if (File)
  {
    while (Run)
    {
      /* read line-wise */
      if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
      {
        Length = strlen(InBuffer);

        if (Length == 0)                  /* sanity check */
        {
          Run = False;                         /* end loop */
        }
        else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
        {
          /* now check if line matches buffer size exacly or exceeds it */
          /* exact matches should have a LF as last character in front of the trailing 0 */
          if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
          {
            Run = False;                         /* end loop */
            Log(L_WARN, "Input overflow for SRIF file!");
          }
        }

        if (Run)       /* if still in business */
        {
          /* remove LF at end of line */
          if (InBuffer[Length - 1] == 10)
          {
            InBuffer[Length - 1] = 0;
            Length--;
          }

          /* if it's not an empty line */
          if (InBuffer[0] != 0)
          {
            TokenList = Tokenize(InBuffer);          /* tokenize line */

            if (TokenList)
            {
              Flag = ParseSRIF(TokenList);           /* parse line */
              if (Flag == False) Run = False;        /* end loop on error */
              FreeTokenlist(TokenList);              /* free linked list */
            }
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;               /* end loop */

        /* check for error */
        if (ferror(File) != 0)
        {
          Flag = False;            /* signal problem */
          Log(L_WARN, "Read error for SRIF!");
        }
      }
    }

    fclose(File);           /* close file */

    /* log problem */
    if (!Flag) Log(L_WARN, "Couldn't read SRIF!");
  }
  else
  {
    Log(L_WARN, "Couldn't open SRIF file (%s)!", Env->SRIF_Filepath);
  }

  return Flag;
}



/*
 *  check SRIF data for required stuff
 *
 *  returns:
 *  - 1 on success (if any match is found)
 *  - 0 on error
 */

_Bool CheckSRIF()
{
  _Bool                  Flag = False;     /* return value */

  /* we don't care obout Baud, Time, RemoteStatus and SystemStatus */

  /*
   *  we require request file, response file and one AKA
   */

  if (Env->RequestFilepath && Env->ResponseFilepath && Env->ReqAKA)
  {
    Flag = True;
  }
  else
  {
    Log(L_WARN, "Required settings are missing in SRIF!");
  }


  /*
   *  missing sysop
   */

  if (Env->Sysop == NULL)
  {
    /* in case we should send a netmail */
    if (Env->CfgSwitches & SW_SEND_NETMAIL)
    {
      /* change that into a textmail */
      Env->CfgSwitches |= SW_SEND_TEXT;           /* enable textmail */
      Env->CfgSwitches &= ~SW_SEND_NETMAIL;       /* disable netmail */
    }

    /* set pseudo name for logging */
    Env->Sysop = CopyString("Sysop");
  }

  return Flag;
}



/* ************************************************************************
 *   configuration parser support
 * ************************************************************************ */


/*
 *  log warning for bad or unknown keyword
 */

void LogBadKeyword(char *KeyWord)
{
  if (KeyWord)      /* sanity check */
  {
    Log(L_WARN, "Bad or unknown Keyword: %s!", KeyWord);
  }
}



/* ************************************************************************
 *   configuration settings
 * ************************************************************************ */


/*
 *  add request limits
 *  Syntax: Limit <FTS address pattern> [Files <number of>] [Bytes <number of>]
 *                [IfListed]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Add_Limit(Token_Type *TokenList)
{
  _Bool             Flag = False;            /* return value */
  _Bool             Run = True;              /* control flag */
  unsigned short    Keyword = 0;             /* keyword ID */
  char              *Address = NULL;         /* FTS address patern */
  long              Files = 20;              /* number of files */
  long long         Bytes = 2000000;         /* number of bytes */
  int               BadPWs = 2;              /* number of bad passwords */
  int               Freqs = 10;              /* number of frequests (*/
  unsigned short    ReqFlags = REQ_NONE;     /* conditions */
  static char       *Keywords[7] =
    {"Limit", "Files", "Bytes", "BadPWs", "Freqs", "IfListed", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Keyword > 0)          /* get value */
    {
      switch (Keyword)
      {
        case 1:     /* FTS address pattern */
          Address = TokenList->String;
          break;

        case 2:     /* files */
          Files = Str2Long(TokenList->String);
          if (Files < -1) Run = False;
          break;

        case 3:     /* bytes */
          Bytes = String2Bytes(TokenList->String);
          if (Bytes < -1) Run = False;
          break;

        case 4:     /* bad PWs */
          BadPWs = Str2Long(TokenList->String);
          if (BadPWs < -1) Run = False;
          break;

        case 5:     /* frequests (filename pattern) */
          Freqs = Str2Long(TokenList->String);
          if (Freqs < -1) Run = False;
      }

      Keyword = 0;            /* reset */
    }
    else                      /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      switch (Keyword)        /* keywords without data */
      {
        case 0:               /* unknown keyword */
          Run = False;
          LogBadKeyword(TokenList->String);
          break;

        case 6:               /* if listed */
          ReqFlags |= REQ_LISTED;
          Keyword = 0;        /* reset keyword */
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Address == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  add to global list
   */

  if (Run)
  {
    Flag = AddLimitElement(Address, Files, Bytes, BadPWs, Freqs, ReqFlags);
  }

  return Flag;
}



/*
 *  add mail footer/header text
 *  Syntax: MailHeader <text>
 *          MailFooter <text>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Add_MailText(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Type = 0;           /* text type */ 
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *TextToken = NULL;  /* token with mail text */
  Token_Type        *HelpToken = NULL;  /* token before TextToken */

  #define FOOTER   1
  #define HEADER   2

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: text */
    {
      TextToken = TokenList;

      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "MailHeader") == 0)   /* header */
    {
      HelpToken = TokenList;           /* save token */
      Type = HEADER;
      Get = 1;
    }
    else if (strcasecmp(TokenList->String, "MailFooter") == 0)   /* footer */
    {
      HelpToken = TokenList;           /* save token */
      Type = FOOTER;
      Get = 1;
    }
    else                                               /* unknown */
    {
      Run = False;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0) || (TextToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  add to global list
   */

  if (Run)
  {
    /* move token from token list to mailheader/footer list */

    HelpToken->Next = TextToken->Next;         /* unlink token */
    TextToken->Next = NULL;                    /* single token */

    if (Type == FOOTER)                 /* footer */
    {
      /* add token to end of list */
      if (Env->LastFooter) Env->LastFooter->Next = TextToken;
      else Env->MailFooter = TextToken;
      Env->LastFooter = TextToken;
    }
    else if (Type == HEADER)            /* header */
    {
      /* add token to end of list */
      if (Env->LastHeader) Env->LastHeader->Next = TextToken;
      else Env->MailHeader = TextToken;
      Env->LastHeader = TextToken;
    }

    Flag = True;       /* signal success */
  }

  return Flag;
}



/*
 *  set path for netmail
 *  Syntax: NetMailDir <path>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Set_MailPath(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *PathToken = NULL;

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* prevent any additional calls of this command */
  if (Env->MailPath)
  {
    Log(L_WARN, "MailPath already set!");
    return Flag;
  }


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: path */
    {
      PathToken = TokenList;
      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "MailDir") == 0)   /* path */
    {
      Get = 1;
    }
    else                                               /* unknown */
    {
      Run = False;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }

  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0) || (PathToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    Env->MailPath = PathToken->String;        /* move string */
    PathToken->String = NULL;
    Flag = True;
  }

  return Flag;
}



/*
 *  add file index
 *  Syntax: Index <filepath> [IfMounted <mounting point>]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Add_Index(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  char              *IndexFilepath = NULL;
  char              *MountingPoint = NULL;
  static char       *Keywords[3] =
    {"Index", "IfMounted", NULL};


  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Keyword > 0)        /* get value */
    {
      switch (Keyword)
      {
        case 1:     /* filepath of file index */
          IndexFilepath = TokenList->String;
          break;
      
        case 2:     /* path of mounting point */
          MountingPoint = TokenList->String;
          break;
      }

      Keyword = 0;             /* reset */
    }
    else                   /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0)       /* unknown keyword */
      {
        Run = False;
        LogBadKeyword(TokenList->String);
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (IndexFilepath == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  add to global list
   */

  if (Run)
  {
    Flag = AddIndexElement(IndexFilepath, MountingPoint);
  }

  return Flag;
}



/*
 *  add FTS address / AKA
 *  Syntax: <zone>:<net>/<node>[.<point>][@<domain>]
 *          
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */
_Bool Add_AKA(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *AddressToken = NULL;
  AKA_Type          *AKA;

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: address */
    {
      AddressToken = TokenList;
      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "Address") == 0)   /* address */
    {
      Get = 1;
    }
    else                                               /* unknown */
    {
      Run = False;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0) || (AddressToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  add to global list
   */

  if (Run)
  {
    if ((AKA = NewAKA(AddressToken->String)))    /* create AKA */
    {
      /* add to global list */
      if (Env->LastAKA) Env->LastAKA->Next = AKA;
      else Env->AKA = AKA;
      Env->LastAKA = AKA;

      Flag = True;     /* signal success */
    }
  }

  return Flag;
}



/*
 *  set mode
 *  Syntax: SetMode [NetMail] [TextMail] [RemoveReq] [AnyCase]
 *                  [BinarySearch] [LogRequest] [SI-Units]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Set_Mode(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[11] =
    {"SetMode", "NetMail", "NetMail+", "TextMail", "RemoveReq",
     "AnyCase", "BinarySearch", "LogRequest", "SI-Units", "IEC-Units", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    Keyword = GetKeyword(Keywords, TokenList->String);

    switch (Keyword)           /* keywords without data */
    {
      case 0:       /* unknown keyword */
        Run = False;
        LogBadKeyword(TokenList->String);
        break;

      case 1:       /* command itself */
        /* just skip */
        break;

      case 2:       /* netmail type-2 */
        Env->CfgSwitches |= SW_SEND_NETMAIL | SW_TYPE_2;
        break;

      case 3:       /* netmail type-2+ */
        Env->CfgSwitches |= SW_SEND_NETMAIL | SW_TYPE_2PLUS;
        break;

      case 4:       /* textmail */
        Env->CfgSwitches |= SW_SEND_TEXT;
        break;

      case 5:       /* remove .req */
        Env->CfgSwitches |= SW_DELETE_REQUEST;
        break;

      case 6:       /* any case */
        Env->CfgSwitches |= SW_ANY_CASE;
        break;

      case 7:       /* binary search */
        Env->CfgSwitches |= SW_BINARY_SEARCH;
        break;

      case 8:       /* log request */
        Env->CfgSwitches |= SW_LOG_REQUEST;
        break;

      case 9:       /* SI units */
        Env->CfgSwitches |= SW_SI_UNITS;
        break;

      case 10:      /* IEC units for output */
        Env->CfgSwitches |= SW_IEC_UNITS;
        break;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if (Run == False)
  {
    LogCfgError();
  }
  else
  {
    Flag = True;       /* signal success */
  }

  return Flag;
}



/*
 *  open logfile
 *  Syntax: LogFile <filepath>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Set_LogFile(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *FilepathToken = NULL;

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* command line takes precedence */
  /* also prevent any additional logfile command */
  if (Env->LogFilepath)
  {
    Log(L_WARN, "Logfile already set!");
    return Flag;
  }


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: filepath */
    {
      FilepathToken = TokenList;
      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "LogFile") == 0)   /* filepath */
    {
      Get = 1;
    }
    else                                               /* unknown */
    {
      Run = False;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0) || (FilepathToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    Env->LogFilepath = FilepathToken->String;    /* move string */
    FilepathToken->String = NULL;
    Flag = OpenLogfile();                        /* open file */
  }

  return Flag;
}



/* ************************************************************************
 *   configuration
 * ************************************************************************ */


/*
 *  parse configuration
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ParseConfig(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[9] =
    {"MailHeader", "MailFooter", "Limit", "Address", "Index",
     "LogFile", "MailDir", "SetMode", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* find parameter and call corresponding function */
  if (TokenList->String)
  {
    Keyword = GetKeyword(Keywords, TokenList->String);

    switch (Keyword)           /* setting keywords */
    {
      case 0:       /* unknown command */
        Log(L_WARN, "Unknown setting in cfg file (%s), line %d (%s)!",
          Env->CfgInUse, Env->CfgLinenumber, TokenList->String);        
        break;

      case 1:       /* mail header */
        Flag = Add_MailText(TokenList);
        break;

      case 2:       /* mail footer */
        Flag = Add_MailText(TokenList);
        break;

      case 3:       /* limit */
        Flag = Add_Limit(TokenList);
        break;

      case 4:       /* address */
        Flag = Add_AKA(TokenList);
        break;

      case 5:       /* index */
        Flag = Add_Index(TokenList);
        break;

      case 6:       /* logfile */
        Flag = Set_LogFile(TokenList);
        break;

      case 7:       /* maildir */
        Flag = Set_MailPath(TokenList);
        break;

      case 8:       /* set mode */
        Flag = Set_Mode(TokenList);
        break;
    }
  }

  return Flag;
}



/*
 *  read configuration
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ReadConfig(char *Filepath)
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* loop control */
  FILE                   *File;               /* filestream */
  size_t                 Length;
  char                   *HelpStr;
  Token_Type             *TokenList;
  unsigned int           Line = 0;            /* line number */
  unsigned int           OldLine;            /* old linenumber */
  char                   *OldCfg;            /* old filepath */

  /* sanity check */
  if (Filepath == NULL) return Flag;

  File = fopen(Filepath, "r");                /* read mode */
  if (File)
  {
    /* setup data for logging */
    OldCfg = Env->CfgInUse;        /* save old cfg filepath */
    Env->CfgInUse = Filepath;      /* update cfg filepath */
    OldLine = Env->CfgLinenumber;  /* save old linenumber */

    while (Run)
    {
      /* read line-wise */
      if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
      {
        Length = strlen(InBuffer);

        if (Length == 0)                  /* sanity check */
        {
          Run = False;                         /* end loop */
        }
        else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
        {
          /* now check if line matches buffer size exacly or exceeds it */
          /* exact matches should have a LF as last character in front of the trailing 0 */
          if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
          {
            Run = False;                         /* end loop */
            Log(L_WARN, "Input overflow for line %d in cfg file (%s)!", ++Line, Filepath);
          }
        }

        if (Run)       /* if still in business */
        {
          Line++;                       /* got another line */
          Env->CfgLinenumber = Line;    /* update linenumber for logging */

          /* remove LF at end of line */
          if (InBuffer[Length - 1] == 10)
          {
            InBuffer[Length - 1] = 0;
            Length--;
          }

          /* find first non whitespace character */
          HelpStr = InBuffer;
          while ((HelpStr[0] == ' ') || (HelpStr[0] == '\t')) HelpStr++;

          /* if it's not an empty line or a comment line, process it */
          if ((HelpStr[0] != 0) && (HelpStr[0] != '#'))
          {
            TokenList = Tokenize(HelpStr);      /* tokenize line */

            if (TokenList)
            {
              Flag = ParseConfig(TokenList);
              if (Flag == False) Run = False;        /* end loop on error */
              FreeTokenlist(TokenList);              /* free linked list */
            }
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;               /* end loop */

        /* check for error */
        if (ferror(File) != 0)
        {
          Flag = False;            /* signal problem */
          Log(L_WARN, "Read error for cfg file (%s)!", Filepath);
        }
      }
    }

    /* clean up */
    fclose(File);                  /* close file */
    Env->CfgInUse = OldCfg;        /* restore old cfg filepath */
    Env->CfgLinenumber = OldLine;  /* restore old linenumber */
  }
  else
  {
    Log(L_WARN, "Couldn't open cfg file (%s)!", Filepath);
  }

  return Flag;
}



/*
 *  check configuration for required stuff
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool CheckConfig()
{
  _Bool                  Flag = False;        /* return value */

  if (Env->AKA && Env->IndexList)
  {
    Flag = True;
  }
  else       /* missing settings */
  {
    Log(L_WARN, "Required settings are missing in cfg file!");
  }

  if (Flag)
  {
    /* set default path for MailDir if not set by cfg */
    if (Env->MailPath == NULL)
    {
      Env->MailPath = CopyString(TMP_PATH);
    }

    /* create default limit if none set by cfg */
    if (Env->LimitList == NULL)
    {
      /* 20 files, 2000000 Bytes, 2 bad PWs, 10 Freqs */
      AddLimitElement("*", 20, 2000000, 2, 10, REQ_NONE);
    }
  }

  return Flag;
}



/* ************************************************************************
 *   command line
 * ************************************************************************ */


/*
 *  print command line options
 */

void PrintUsage()
{
  printf(NAME" "VERSION" "COPYRIGHT"\n");
  printf("Usage: "NAME" [options] -s <SRIF file>\n");
  printf("Options:\n");
  printf("  -h, -?                 Print this brief help.\n");
  printf("  -c <config file>       Use specified configuration file.\n");
  printf("  -l <log file>          Use specified log file.\n");
}



/*
 *  parse command line options
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ParseCommandLine(int argc, char *argv[])
{
  _Bool              Flag = TRUE;        /* return value */
  unsigned int       n = 1;              /* loop counter */
  unsigned short     Keyword = 0;        /* keyword ID */
  static char        *Keywords[6] =
    {"-h", "-?", "-c", "-l", "-s", NULL};

  /* sanity checks */
  if ((argc == 0) || (argv == NULL)) return False;


  /*
   *  get command line options
   */

  while (n < argc)                            /* as long as we got options, check: */
  {
    if (Keyword > 2)          /* get value */
    {
      switch (Keyword)
      {
        case 3:     /* cfg filepath */
          if (Env->CfgFilepath)      /* free old value if already set */
          {
            free(Env->CfgFilepath);
            Env->CfgFilepath = NULL;
          }
          Env->CfgFilepath = CopyString(argv[n]);
          break;

        case 4:     /* log filepath */
          if (Env->LogFilepath)      /* free old value if already set */
          {
            free(Env->LogFilepath);
            Env->LogFilepath = NULL;
          }
          Env->LogFilepath = CopyString(argv[n]);
          break;

        case 5:     /* SRIF filepath */
          if (Env->SRIF_Filepath)    /* free old value if already set */
          {
            free(Env->SRIF_Filepath);
            Env->SRIF_Filepath = NULL;
          }  
          Env->SRIF_Filepath = CopyString(argv[n]);
      }

      Keyword = 0;            /* reset */
    }
    else                      /* get keyword */
    {
      Keyword = GetKeyword(Keywords, argv[n]);

      if (Keyword == 0)       /* unknown keyword */
      {
        Flag = False;           /* treat this as problem */
        Log(L_WARN, "Unknown option \"%s\"!", argv[n]);
        n = argc;               /* end loop */
      }
      else if (Keyword < 3)   /* help */
      {
        Env->Run = False;       /* don't proceed */
        n = argc;               /* end loop */
        Keyword = 0;
        PrintUsage();           /* print usage */
      }
    }

    n++;             /* next arg */
  }


  /*
   *  check parser results
   */

  if (Keyword > 0)            /* missing argument  */
  {
    Log(L_WARN, "Missing argument!");
    Flag = False;
  }

  /* check if we got all required options */
  if (Flag && Env->Run)       /* if everything's fine so far */
  {
    /* we must have the SRIF filepath */
    if (Env->SRIF_Filepath == NULL)
    {
      Log(L_WARN, "Missing SRIF filepath!");
      Flag = False;
    }

    /* set default config filepath */
    if (Env->CfgFilepath == NULL)
    {
      Env->CfgFilepath = CopyString(CFG_FILEPATH);
    }
  }
 
  /* on problems give user a hint */
  if (Flag == False)
  {
    Log(L_WARN, "Please run "NAME" -h!");
  }

  return Flag;
}



/* ************************************************************************
 *   initialization
 * ************************************************************************ */


/*
 *  initialization of special stuff
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool InitStuff(void)
{
  _Bool                 Flag = False;

  /* get current working directory */
  Env->CWD = getcwd(NULL, 0);           /* Linux glibc allocates buffer */ 

  /* get process ID */
  Env->PID = getpid();

  /* check values */
  if (Env->CWD && (Env->PID > 0)) Flag = True;

  /* get current time */
  if (Flag)
  {
    Flag = False;                 /* reset flag */

    if (time(&(Env->UnixTime)) != -1)      /* get UNIX time */
    {
      /* convert unix time */
      if (localtime_r(&(Env->UnixTime), &(Env->DateTime)) != NULL)
      {
        Flag = True;
      }
    }
  }

  return Flag;
}



/* ************************************************************************
 *   global memory management
 * ************************************************************************ */


/*
 *  allocate memory for global variables
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool GetAllocations(void)
{
  _Bool                 Flag = False;

  /* buffers */
  LogBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  InBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  InBuffer2 = (char *) malloc(DEFAULT_BUFFER_SIZE);
  OutBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  TempBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  TempBuffer2 = (char *) malloc(DEFAULT_BUFFER_SIZE);
  AliasBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);

  /* environment / configuration */
  Env = calloc(1, sizeof(Env_Type));

  /* check pointeris */
  if (LogBuffer && InBuffer && InBuffer2 && OutBuffer &&
      TempBuffer && TempBuffer2 && AliasBuffer && Env)
  {
    Flag = True;        /* ok to proceed */
  }

  /* set default values */
  if (Flag)
  {
    /* environment */
    Env->CfgFilepath = NULL;
    Env->LogFilepath = NULL;

    /* environment: process */
    Env->CWD = NULL;
    Env->PID = 0;

    /* environment: file streams */
    Env->Log = NULL;

    /* environment: program control */
    Env->Run = True;          /* run by default */
    Env->CfgInUse = NULL;
    Env->CfgLinenumber = 0;

    /* environment: common configuration */
    Env->CfgSwitches = SW_NONE;

    /* environment: file index */
    Env->LookupList = NULL;
    Env->LastLookup = NULL;

    /* environment: frequest configuration */
    Env->MailPath = NULL;
    Env->MailHeader = NULL;
    Env->LastHeader = NULL;
    Env->MailFooter = NULL;
    Env->LastFooter = NULL;
    Env->AKA = NULL;
    Env->LastAKA = NULL;
    Env->IndexList = NULL;
    Env->LastIndex = NULL;
    Env->LimitList = NULL;
    Env->LastLimit = NULL;

    /* environment: frequest filepaths */
    Env->SRIF_Filepath = NULL;
    Env->RequestFilepath = NULL;
    Env->ResponseFilepath = NULL;
    Env->MailFilepath = NULL;
    Env->TextFilepath = NULL;

    /* request(er) details */
    Env->Sysop = NULL;
    Env->ReqAKA = NULL;
    Env->ReqLastAKA = NULL;
    Env->CalledAKA = NULL;
    Env->BPS = 0;
    Env->ReqTime = 0;
    Env->ReqFlags = REQ_NONE;
    Env->CallerID = NULL;
    Env->SessionType = NULL;
    Env->RequestList = NULL;
    Env->LastRequest = NULL;

    /* environment: frequest runtime stuff */
    Env->ActiveLimit = NULL;
    Env->Files = 0;
    Env->Bytes = 0;
    Env->FreqStatus = FREQ_NONE;
    Env->ActiveLocalAKA = NULL;
    Env->ActiveRemoteAKA = NULL;
  }

  return Flag;
}



/*
 *  free memory of variables
 */

void FreeAllocations()
{
  /*  structures */
  if (Env)
  {
    /* free strings */
    if (Env->CfgFilepath) free(Env->CfgFilepath);
    if (Env->LogFilepath) free(Env->LogFilepath);
    if (Env->CWD) free(Env->CWD);
    if (Env->MailPath) free(Env->MailPath);
    if (Env->SRIF_Filepath) free(Env->SRIF_Filepath);
    if (Env->RequestFilepath) free(Env->RequestFilepath);
    if (Env->ResponseFilepath) free(Env->ResponseFilepath);
    if (Env->MailFilepath) free(Env->MailFilepath);
    if (Env->TextFilepath) free(Env->TextFilepath);
    if (Env->Sysop) free(Env->Sysop);
    if (Env->CallerID) free(Env->CallerID);
    if (Env->SessionType) free(Env->SessionType);

    /* free lists */
    if (Env->MailHeader) FreeTokenlist(Env->MailHeader);
    if (Env->MailFooter) FreeTokenlist(Env->MailFooter);
    if (Env->AKA) FreeAKAlist(Env->AKA);
    if (Env->IndexList) FreeIndexList(Env->IndexList);
    if (Env->LimitList) FreeLimitList(Env->LimitList);
    if (Env->ReqAKA) FreeAKAlist(Env->ReqAKA);
    if (Env->CalledAKA) FreeAKAlist(Env->CalledAKA);
    if (Env->RequestList) FreeRequestList(Env->RequestList);

    /* structure itself */
    free(Env);
    Env = NULL;
  }

  /* buffers */
  if (LogBuffer)
  {
    free(LogBuffer);
    LogBuffer = NULL;
  }
  if (InBuffer)
  {
    free(InBuffer);
    InBuffer = NULL;
  }
  if (InBuffer2)
  {
    free(InBuffer2);
    InBuffer2 = NULL;
  }
  if (OutBuffer)
  {
    free(OutBuffer);
    OutBuffer = NULL;
  }
  if (TempBuffer)
  {
    free(TempBuffer);
    TempBuffer = NULL;
  }
  if (TempBuffer2)
  {
    free(TempBuffer2);
    TempBuffer = NULL;
  }
  if (AliasBuffer)
  {
    free(AliasBuffer);
    AliasBuffer = NULL;
  }
}



/* ************************************************************************
 *   the one and only main()
 * ************************************************************************ */


/*
 *  main function
 */

int main(int argc, char *argv[])
{
  int                RetVal = EXIT_FAILURE;   /* return value */
  _Bool              Flag = False;            /* control flag */

  /* sanity checks */
  if ((argc == 0) || (argv == NULL)) return RetVal;


  /* 
   *  basic initialization
   */

  if (GetAllocations())                 /* allocate global variables */
  {
    if (ParseCommandLine(argc, argv))   /* parse cmd line options */
    {
      Flag = True;                      /* ok for next part */
    }
  }

  if (Flag && Env->Run)
  {
    Flag = False;                       /* reset flag */

    if (InitStuff())                    /* init special stuff */
    {
      /* open log file if requested by cmd line */
      if (Env->LogFilepath) Flag = OpenLogfile();
      else Flag = True;
    }
  }


  /*
   *  read cfg, read SRIF and init stuff
   */

  if (Flag && Env->Run)
  {
    Flag = False;                       /* reset flag */

    if (ReadConfig(Env->CfgFilepath))   /* read config */
    {
      if (CheckConfig())                /* check cfg for required stuff */
      {
        if (ReadSRIF())                 /* read SRIF */
        {
          if (CheckSRIF())              /* check SRIF for required stuff */
          {
            if (ActivateAKAs())         /* find best AKA pair */
            {
              Flag = True;              /* ok to proceed */
            }
          }
        }
      }
    } 
  }


  /*
   *  process request
   */

  if (Flag && Env->Run)
  {
    Flag = False;                       /* reset flag */

    /* log request */
    Log(L_INFO, "Request from %s %s",
      Env->Sysop, Env->ActiveRemoteAKA->Address);
    if (Env->CallerID)                  /* caller ID available */
    {
      Log(L_INFO, "Caller ID: %s", Env->CallerID);
    }
    if (Env->SessionType)               /* session type available */
    {
      Log(L_INFO, "Session Type: %s", Env->SessionType);
    }

    ActivateLimits();         /* apply rule for frequest limits */

    if (ReadRequest())        /* read request */
    {
      Flag = True;

      /* delete request file if requested */
      if (Env->CfgSwitches & SW_DELETE_REQUEST)
        unlink(Env->RequestFilepath);

      Flag &= ProcessRequest();    /* search for requested files */

      /* create netmail if requested */
      if (Env->CfgSwitches & SW_SEND_NETMAIL)
        Flag &= NetMail();

      /* create textmail if requested */
      if (Env->CfgSwitches & SW_SEND_TEXT)
        Flag &= TextMail();

      /* log request detailed if requested */ 
      if (Env->CfgSwitches & SW_LOG_REQUEST) LogRequest();

      Flag &= WriteResponse();     /* write file list */

      /* log results */
      if (Env->FreqStatus & FREQ_FREQLIMIT)
        Log(L_INFO, "frequest limit exceeded");
      if (Env->FreqStatus & FREQ_FILELIMIT)
        Log(L_INFO, "file limit exceeded");
      if (Env->FreqStatus & FREQ_BYTELIMIT)
        Log(L_INFO, "byte limit exceeded");
      if (Env->FreqStatus & FREQ_PWLIMIT)
        Log(L_INFO, "bad PW limit exceeded");

      if (Bytes2String(Env->Bytes, TempBuffer, DEFAULT_BUFFER_SIZE))
        Log(L_INFO, "Totals: %ld files / %s", Env->Files, TempBuffer);
    }
  }


  /*
   *  clean up
   */

  if (Env)
  {
    if (Env->Run)             /* log "done" */
    {
      if (Flag) Log(L_INFO, NAME" "VERSION" ended.");
      else Log(L_INFO, NAME" "VERSION" ended with error!");
    }

    if (Env->Log)             /* close logfile */
    {
      fclose(Env->Log);
      Env->Log = NULL;
    }
  }

  FreeAllocations();          /* free memory */

  if (Flag) RetVal = EXIT_SUCCESS;     /* on success update return value */
  return RetVal;
}



/* ************************************************************************
 *   clean-up of local constants
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef MFREQ_SRIF_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
