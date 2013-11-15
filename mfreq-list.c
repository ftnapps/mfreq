/* ************************************************************************
 *
 *   mfreq-list
 *
 *   (c) 1994-2013 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define MFREQ_LIST_C


/* defaults for this program */
#define NAME            "mfreq-list"
#define CFG_FILENAME    "list.cfg"


/*
 *  include header files
 */

#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */

/* file stuff */
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


/*
 *  local functions
 */

_Bool ReadConfig(char *Filepath);



/* ************************************************************************
 *   data sorting
 * ************************************************************************ */


/*
 *  sort file info by name
 *  - algorithm: merge sort
 *  - lower case > upper case
 */

Info_Type *MergeSort(Info_Type *List, Info_Type **Last)
{
  Info_Type             *NewList = NULL;       /* return value */
  Info_Type             *LeftSub;              /* left sublist */
  Info_Type             *RightSub;             /* right sublist */
  Info_Type             *Element = NULL;       /* single element */
  Info_Type             *MergedList;           /* merged list */
  unsigned int          LeftSize;              /* number of elements in left sublist */
  unsigned int          RightSize;             /* number of elements in right sublist */
  unsigned int          StepSize;              /* number of elements to process */
  unsigned int          Merges;                /* number of sublist merges */
  _Bool                 Run = True;

  /* sanity check */
  if (List == NULL) return NewList;


  /*
   *  master loop
   */

  StepSize = 1;       /* start with sublists with one element each */
                      /* a 1 element sublist is sorted by definition :-) */

  NewList = List;     /* initialize new list for first loop run */

  while (Run)
  {
    /* prepare this run */
    LeftSub = NewList;       /* start with new list */
    NewList = NULL;          /* reset new list */
    MergedList = NULL;       /* reset merged list */
    Merges = 0;              /* reset counter */


    /*
     *  process list using sublists with StepSize elements  
     */

    /* as long as we haven't reached the lists end */
    while (LeftSub)
    {
      /*
       *  create virtual left and right sublists with StepSize elements
       */
 
      RightSub = LeftSub;      /* starting point */
      LeftSize = 0;            /* reset size */

      /* as long as we don't reach the lists end move SubSize elements to the right */
      while (RightSub && (LeftSize < StepSize))
      {
        LeftSize++;                     /* increase size of left sublist */
        RightSub = RightSub ->Next;     /* move to next element */
      }

      RightSize = StepSize;             /* assume size of right sublist to be StepSize */
                                        /* might be larger than real size */

      /* 
       *  merge both sub lists as long as elements are left
       *  also prevent overrun of right sublist
       */ 

      while ((LeftSize > 0) || ((RightSize > 0) && RightSub))
      {
        /*
         *  select element to merge
         */

        /* no elements left in left sublist */
        if (LeftSize == 0)
        {
          /* so take next element of right sublist */
          Element = RightSub;            /* take element */
          RightSize--;                   /* one element less in sublist */
          RightSub = RightSub->Next;     /* move to next element in sublist */
        }

        /* no elements left in right sublist */
        /* or end of right sublist reached */
        else if ((RightSize == 0) || (RightSub == NULL))
        {
          /* so take next element of left sublist */
          Element = LeftSub;             /* take element */
          LeftSize--;                    /* one element less in sublist */
          LeftSub = LeftSub->Next;       /* move to next element in sublist */
        }

        /* otherwise we have to compare the next elements of both sublists */
        else
        {
          if (LeftSub->Name && RightSub->Name)      /* sanity check */
          {
            if (strcmp(LeftSub->Name, RightSub->Name) <= 0)
            {
              /* element of left sublist is smaller or equal */
              /* take that one */
              Element = LeftSub;             /* take element */
              LeftSize--;                    /* one element less in sublist */
              LeftSub = LeftSub->Next;       /* move to next element in sublist */
            }
            else
            {
              /* element of right sublist is smaller */
              /* take that one */
              Element = RightSub;            /* take element */
              RightSize--;                   /* one element less in sublist */
              RightSub = RightSub->Next;     /* move to next element in sublist */
            }
          }
        }


        /*
         *  merge selected element
         *  by moving it to the merged list
         */

        if (MergedList)             /* if merged list exists */
        {
          MergedList->Next = Element;    /* link element */
        }
        else                        /* otherwise */
        {
          /* it's the first element for the merged list */
          /* and the starting point for the next loop run */
          NewList = Element;        /* set first element of new list */
        }

	MergedList = Element;      /* Element is the new end of merged list */
      }

      Merges++;                    /* another merge done */
      LeftSub = RightSub;          /* move to next sublist pair */
    }

    MergedList->Next = NULL;        /* end merged list */


    /*
     *  loop control/feedback
     */

    /* if only one merge is done we sorted the complete list */ 
    if (Merges <= 1)          /* if job done */
    {
      Run = False;                     /* end master loop */

      /* if requested update pointer to last element */
      if (Last) *Last = MergedList;
    }
    else                      /* another run required */
    {
      StepSize = StepSize * 2;    /* double stepsize of sublist for next run */
    }
  }

  return NewList;
}



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
    /*
     *  We try to lock the logfile to ensure that only one
     *  instance of this tool is running.
     */
 
    Flag = LockFile(Env->Log, Env->LogFilepath);   /* lock file */
    if (Flag == False)
    {
      Log(L_WARN, "Couldn't lock logfile!");
      Log(L_WARN, "Another instance of " NAME " might be running.");
    }
  }
  else                     /* error */
  {
    Log(L_WARN, "Couldn't open logfile (%s)!", Env->LogFilepath);
  }

  Log(L_INFO, NAME" "VERSION" started.");      /* log: start */

  return Flag;
}



/*
 *  open filelist
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool OpenFilelist()
{
  _Bool                  Flag = False;        /* return value */

  /* sanity check */
  if (Env->ListFilepath == NULL) return Flag;

  Env->List = fopen(Env->ListFilepath, "w");     /* truncate and write mode */

  if (Env->List != NULL)    /* success */
  {
    /*
     *  We try to lock the filelist to ensure that only one
     *  instance of this tool is writing the file.
     */
 
    Flag = LockFile(Env->List, Env->ListFilepath);   /* lock file */
    if (Flag == False)
    {
      Log(L_WARN, "Couldn't lock filelist!");
      Log(L_WARN, "Another instance of " NAME " might be running.");
    }
  }
  else                     /* error */
  {
    Log(L_WARN, "Couldn't open filelist (%s)!", Env->ListFilepath);
  }

  return Flag;
}



/*
 *  close filelist
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool CloseFilelist()
{
  _Bool                  Flag = False;        /* return value */
  char                   *Help;

  /* sanity check */
  if (Env->List == NULL) return Flag;

  UnlockFile(Env->List);  /* unlock file */
  fclose(Env->List);      /* close file */
  Env->List = NULL;       /* reset pointer */

  /* log statistics */
  Help = GetFilename(Env->ListFilepath);
  if (Help)
  {
    if (Bytes2String(Env->Bytes, TempBuffer, DEFAULT_BUFFER_SIZE))
      Log(L_INFO, "Processed %ld files / %s for %s.", Env->Files, TempBuffer, Help);
  }

  /* clean up */
  if (Env->ListFilepath)             /* free filepath */
  {
    free(Env->ListFilepath);
    Env->ListFilepath = NULL;
  }
  Env->Files = 0;                    /* reset counters */
  Env->Bytes = 0;
  
  Flag = True;

  return Flag;
}



/* ************************************************************************
 *   universal output
 * ************************************************************************ */


/*
 *  write single file info to a file
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteInfo(FILE *File, Info_Type *Info, Field_Type **Fields, unsigned short Max)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Add;                /* add flag */
  _Bool             Write = True;       /* write flag */
  Field_Type        *Field;             /* data field */
  Field_Type        *DescField = NULL;
  Token_Type        *Text;
  char              *Buffer;            /* virtual buffer */
  long long         Value;
  size_t            BufferSize;         /* size usage of buffer */
  size_t            LineSize;           /* length of current line */
  size_t            Length;             /* string length */
  int               Diff;
  int               Spaces;             /* number of spaces */
  struct tm         *DateTime;
  unsigned short    Counter = 1;        /* line number */

  /* sanity checks */
  if ((File == NULL) || (Info == NULL) ||
      (Fields == NULL)) return Flag;

  Flag = True;           /* success by default :-) */


  /*
   *  first line with file name
   */

  Buffer = OutBuffer;
  Buffer[0] = 0;
  BufferSize = 0;
  LineSize = 0;

  Field = *Fields;
  while (Field)                /* follow fields */
  {
    Add = False;           /* reset flag */
    InBuffer[0] = 0;       /* reset buffer */

    /*
     *  create field data string
     */

    if (Field->Type == FIELD_NAME)          /* name */
    {
      if (Info->Name)
      {
        snprintf(InBuffer, Field->Width + 1, "%s", Info->Name);

        Length = strlen(Info->Name);
        if (Length > Field->Width)        /* width overflow */
        {
          /* truncate name with asterisk */
          InBuffer[Field->Width - 1] = '*';
        }
        else if (Field->Format == FIELD_FORM_DOS)
        {
          /* check name syntax */
          Write = CheckDosFilename(Info->Name);
        }
        
        Add = True;      
      }
    }
    else if (Field->Type == FIELD_SIZE)     /* size */
    {
      Add = Bytes2StringN(Info->Size, Field->Width, Field->Format,
        InBuffer, DEFAULT_BUFFER_SIZE - 1);
    }
    else if (Field->Type == FIELD_DATE)     /* date */
    {
      DateTime = localtime(&(Info->Time));     /* convert unix time */

      if (DateTime == NULL)
      {
        /* nothing */
      }      
      else if (Field->Format == FIELD_FORM_US)    /* MM-DD-YY */
      {
        Length = strftime(InBuffer, 9, "%m-%d-%y", DateTime);
        if (Length > 0) Add = True;
      }
      else if (Field->Format == FIELD_FORM_ISO)   /* YYYY-MM-DD */
      {
        Length = strftime(InBuffer, 11, "%Y-%m-%d", DateTime);
        if (Length > 0) Add = True;
      }
    }
    else if (Field->Type == FIELD_COUNTER)  /* counter */
    {
      /* limit digits to width */
      Value = LimitNumber(Info->Counter, Field->Width - 2);

      /* convert to string */
      if (Field->Format == FIELD_FORM_SQUARE)
      {
        snprintf(InBuffer, DEFAULT_BUFFER_SIZE - 1,
          "[%*lld]", Field->Width - 2, Value);
        Add = True;
      }
    }
    else if (Field->Type == FIELD_DESC)     /* description */
    {
      DescField = Field;    /* save pointer */

      Text = Info->Infos;
      if (Text && (Text->String))      /* got description */
      {
        snprintf(InBuffer, DEFAULT_BUFFER_SIZE - 1, "%s", Text->String);
        Add = True;

        /* width overflow */
        Length = strlen(Text->String);
        Diff = Max - Field->Pos;
        if (Length > Diff)
        {
          if ((Diff > 1) && (Diff < DEFAULT_BUFFER_SIZE - 1))    /* sanity check */
          {
            InBuffer[Diff] = 0;
            InBuffer[Diff - 1] = '~'; 
          }
        }
      }
      else                             /* no description */
      {
        strcpy(InBuffer, "n/a");
        Add = True;
      }
    }

    /*
     *  add field data string to buffer
     */

    if (Add)
    {
      /* line management */
      while (Field->Line > Counter)     /* add lines until we match */
      {
        /* add linefeed */
        snprintf(Buffer, DEFAULT_BUFFER_SIZE - 1 - BufferSize, "\n");
        Length = strlen(Buffer);
        BufferSize += Length;
        Buffer = &(OutBuffer[BufferSize]);
        LineSize = 0;
        Counter++;
      }

      Spaces = 0;
      Length = strlen(InBuffer);

      /* take care about start position */
      Diff = Field->Pos - 1 - LineSize; 
      if (Diff > 0)
      {
        Spaces += Diff;
      }
      /* else if (Diff < 0) shouldn't happen */

      /* take care about right alignment */
      if ((Field->Align == ALIGN_RIGHT) && (Field->Width > 0))
      {
        Diff = Field->Width - Length;
        if (Diff > 0)
        {
          Spaces += Diff;
        }
        /* else if (Diff < 0) shouldn't happen */
      }

      /* fill up with spaces */
      if (Spaces > 0)
      {
        FillString(Buffer, ' ', Spaces, DEFAULT_BUFFER_SIZE - 1 - BufferSize);
        Length = strlen(Buffer);
        LineSize += Length;
        BufferSize += Length;
        Buffer = &(OutBuffer[BufferSize]);
      }

      /* add data string */
      snprintf(Buffer, DEFAULT_BUFFER_SIZE - 1 - BufferSize, "%s", InBuffer);
      Length = strlen(Buffer);
      LineSize += Length;
      BufferSize += Length;
      Buffer = &(OutBuffer[BufferSize]);
    }

    Field = Field->Next;         /* next one */
  }

  if (Write) fprintf(File, "%s\n", OutBuffer);


  /*
   *  additional description lines
   */

  /* check for multiline mode */
  if (Write && DescField && (DescField->Format & FIELD_FORM_MULTI))
  {
    /* skip first line since it's already written */
    Text = Info->Infos;
    if (Text) Text = Text->Next;
 
    while (Text)              /* loop through remaining lines */
    {
      /* reset variables */
      Buffer = OutBuffer;
      Buffer[0] = 0;
      LineSize = 0;

      if (Text->String)       /* sanity check */
      {
        /* take care about start position */
        Spaces = DescField->Pos - 1;
        FillString(Buffer, ' ', Spaces, DEFAULT_BUFFER_SIZE - 1);
        LineSize += strlen(Buffer);
        Buffer = &(OutBuffer[LineSize]);

        /* add description */
        snprintf(Buffer, DEFAULT_BUFFER_SIZE - 1 - LineSize, "%s", Text->String);

        /* prevent width overflow */
        Length = strlen(Buffer);
        Diff = Max - DescField->Pos;
        if (Length > Diff)              /* too long */
        {
          if ((Diff > 1) && (Diff < DEFAULT_BUFFER_SIZE - 1))    /* sanity check */
          {
            /* truncate line */
            Buffer[Diff] = 0;
            Buffer[Diff - 1] = '~'; 
          }
        }

        /* write line */
        fprintf(File, "%s\n", OutBuffer);
      }

      Text = Text->Next;             /* next one */
    }
  }

  return Flag;
}



/* ************************************************************************
 *   files.bbs
 * ************************************************************************ */


/*
 *  parse files.bbs
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Parse_files_bbs(char *Buffer, unsigned int Line, Info_Type **CurrentInfo)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Skip = False;       /* skip flag */
  unsigned short    Pos;                /* position in string */
  unsigned short    NewPos;             /* new position in string */
  unsigned short    Width;              /* sub-string width */
  unsigned short    Length;             /* sub-string width */
  long long         Value;              /* twice as long as off_t */
  char              *Str;               /* support string pointer */
  char              *Data;              /* data sub-string */
  char              *NewData;           /* new data sub-string */
  Info_Type         *Info;              /* fileinfo element */
  Field_Type        *Field;             /* data field element */
  time_t            UnixTime;
  struct tm         DateTime;

  /* sanity checks */
  if ((Buffer == NULL) || (CurrentInfo == NULL) ||
      (Env->Fields_files_bbs == NULL)) return Flag; 

  Flag = True;
  Data = Buffer;
  Field = Env->Fields_files_bbs;
  Pos = 1;

  /*
   *  additional description line
   */

  if (Data[0] == ' ')         /* line starts with space */
  {
    if (*CurrentInfo != NULL)     /* if ok */
    {
      /* skip spaces */
      while (Data[0] == ' ')
      {
        Pos++;
        Data++;
      }

      /* check for string end */
      if (Data[0] == 0)
      {
        Flag = False;
      }

      /* check position if strict checking is enabled */
      else if (Env->InfoMode & INFO_STRICT)
      {
        /* get description data field */
        while (Field && (Field->Type != FIELD_DESC))
        {
          Field = Field->Next;
        }

        if (Field)
        {
          if (Field->Pos != Pos)        /* compare positions */
          {
            Flag = False;
            Log(L_WARN, "Position mismatch (line %d)!", Line);
          }
        }
        /* else something weird happened */
      }

      AddDesc2Info(*CurrentInfo, Data);
    }
  }

  /*
   *  line starting with filename
   */

  else
  {
    while (Field)    /* follow data field list */
    {
      /* skip spaces */
      while (Data[0] == ' ')
      {
        Pos++;
        Data++;
      }

      /* check for string end */
      if (Data[0] == 0)
      {
        /* if missing data fields are allowed */
        if ((Env->InfoMode & INFO_SKIPS) &&
           (Field->Type != FIELD_NAME))
        {
          Skip = True;
        }
        /* otherwise report syntax problem */
        else
        {
          Flag = False;
          Log(L_WARN, "Missing data fields (line %d)!", Line);
        }
      }

      /* check position if strict checking is enabled */
      else if (Env->InfoMode & INFO_STRICT)
      {
        if (Field->Align == ALIGN_LEFT)        /* aligned left */
        {
          /* data should start at given position */
          if (Field->Pos != Pos) Flag = False;
        }
        else if (Field->Align == ALIGN_RIGHT)  /* aligned right */
        {
          /* data should start between position and max. size */
          if ((Pos < Field->Pos) ||
              (Pos > Field->Pos + Field->Width - 1)) Flag = False;
        }

        if (!Flag) Log(L_WARN, "Position mismatch (line %d)!", Line);
      }

      /* separate data field */
      if ((!Skip) && Flag)
      {
        Length = strlen(Data);

        /* if another field follows */
        if ((Field->Width > 0 ) && (Length > Field->Width))
        {
          /* we search from right to left */
          /* allowing spaces inside the data field */
          Str = &Data[Field->Width];   /* char behind field (space) */
          Width = Field->Width + 1;    /* same here */
          NewData = &Data[Width];      /* points to second char behind field */
          NewPos = Pos + Width;        /* same here */

          /* skip spaces (right to left) */
          while (Str[0] == ' ')
          {
            Str--;
            Width--;
          }

          /* process result */
          if (Width <= Field->Width)   /* found the data field's end */
          {
            /* create sub-string */
            Str++;       /* char behind field (space) */
            Str[0] = 0;
          }
          else                         /* max. width exceeded */
          {
            Flag = False;    /* end loop */
          }

          /* check end position if strict checking is enabled */
          /* just for right aligned data */
          if ((Env->InfoMode & INFO_STRICT) &&
              (Field->Align == ALIGN_RIGHT))
          {
            /* end has to be within data fields position range */
            if (Pos + Width > Field->Pos + Field->Width) Flag = False;
            /* problem: if Field->Width is zero */
          }

          if (!Flag) Log(L_WARN, "Maximum field width exceeded (line %d)!", Line);
        }
        else    /* it's the last data field */
        {
          NewData = &Data[Length];    /* points to trailing 0 */
          NewPos = Pos + Length;
          Width = Length;
        }
      }

      /* parse field */
      if (Skip || (Flag == False))
      {
        /* do nothing */
      }
      else if (Field->Type == FIELD_NAME)       /* filename field */
      {
        /*  check name for DOS format (8.3) */
        if (Field->Format == FIELD_FORM_DOS)
        {
          Flag = CheckDosFilename(Data);
          if (!Flag) Log(L_WARN, "No DOS-style filename (line %d)!", Line); 
        }

        /* get info element or create new one if no match is found */
        if (Flag)
        {
          *CurrentInfo = NULL;
          Info = SearchInfoList(Env->InfoList, Data);
          /* problem: truncated long filenames */ 

          if (Info == NULL)      /* info not available */
          {
            /* create new element */
            if (AddInfoElement(Data, 0 , 0))
            {
              Info = Env->LastInfo;
              Info->Status = STAT_NOT_FOUND;
              *CurrentInfo = Info;
            }
          }
          else
          {
            *CurrentInfo = Info;
          }
        }
      }
      else if (Field->Type == FIELD_SIZE)     /* filesize field */
      {
        /* check syntax and convert */
        if (Width > Field->Width)    /* width too large */
        {
          Flag = False;
        }
        else                         /* take any format as input */
        {
          Value = String2Bytes(Data);        /* convert string */

          if (Value >= 0)   /* valid size */
          {
            if (Info->Status & STAT_NOT_FOUND)   /* no value set yet */
            {
              Info->Size = Value;
            }
            else if (Info->Size != Value)        /* sizes differ */
            {
              Info->Status |= STAT_CHANGED;
            }
          }
          else              /* invalid size */
          {
            Flag = False;
          }
        }

        if (!Flag) Log(L_WARN, "Syntax error for size field (line %d)!", Line);
      }
      else if (Field->Type == FIELD_DATE)     /* filedate field */
      {
        /* check syntax and prepare data string */
        if (Width != Field->Width)     /* width doesn't match */
        {
          Flag = False;
        }
        else if (Field->Format == FIELD_FORM_US)  /* MM-DD-YY */
        {
          if ((Data[2] == '-') && (Data[5] == '-'))
          {
            Data[2] = 0;
            Data[5] = 0;
            Flag = Strings2DateTime(&Data[6], &Data[0], &Data[3], &DateTime);
          }
          else Flag = False;
        }
        else if (Field->Format == FIELD_FORM_ISO)  /* YYYY-MM-DD */
        {
          if ((Data[4] == '-') && (Data[7] == '-'))
          {
            Data[4] = 0;
            Data[7] = 0;
            Flag = Strings2DateTime(&Data[0], &Data[5], &Data[8], &DateTime);
          }
          else Flag = False;
        }

        /* convert to struct tm and then time_t */
        if (Flag)
        {
          UnixTime = mktime(&DateTime);
          if (UnixTime != -1)
          {
            if (Info->Time == 0)         /* set time if unset */
            {
              Info->Time = UnixTime;
            }
            else                         /* compare times */
            {
              /* later time than in files.bbs (date + 1 day) */
              if (Info->Time > UnixTime + (60 * 60 * 24))
                Info->Status |= STAT_CHANGED;
            }
          }
          else Flag = False;
        }

        if (!Flag) Log(L_WARN, "Syntax error for date field (line %d)!", Line);
      }
      else if (Field->Type == FIELD_COUNTER)  /* download counter field */
      {
        /* check syntax and prepare data string */
        if (Width != Field->Width)     /* width doesn't match */
        {
          Flag = False;
        }
        else if (Field->Format == FIELD_FORM_SQUARE)   /* square braces */ 
        {
          if ((Data[0] == '[') && (Data[Width - 1] == ']'))
          {
            /* get rid of the braces */
            Data[Width - 1] = 0;       /* remove "]" */
            Data++;                    /* skip "[" */
          }
          else
          {
            Flag = False;
          }
        }

        /* convert counter */
        if (Flag)
        {
          Value = Str2Long(Data);
          if (Value >= 0) Info->Counter = Value;
          else Flag = False;
        }

        if (!Flag) Log(L_WARN, "Syntax error for counter field (line %d)!", Line);
      }
      else if (Field->Type == FIELD_DESC)     /* file description field */
      {
        AddDesc2Info(*CurrentInfo, Data);
      }
      else                                    /* unknown field */
      {
        Flag = False;          /* end loop */
      }

      /* manage loop */
      if (Skip || (!Flag))         /* stop */
      {
        Field = NULL;                /* end loop */
      }
      else                         /* proceed */
      {
        /* update Data and Pos */
        Data = NewData;
        Pos = NewPos;

        Field = Field->Next;         /* next data field */
      }
    }
  }

  return Flag;
}



/*
 *  read files.bbs
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Read_files_bbs(char *Path)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Check;
  FILE              *File;              /* files.bbs stream */
  size_t            Length;
  Info_Type         *Info = NULL;
  unsigned int      Line = 0;           /* line number */

  /* sanity check */
  if (Path == NULL) return Flag;

  Flag = True;

  /* try to open files.bbs or FILES.BBS */
  Check = 2;
  while (Check > 0)
  {
    if (Check == 2) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "files.bbs");
    else if (Check == 1) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "FILES.BBS");

    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1, "%s/%s", Path, TempBuffer2);
    File = fopen(TempBuffer, "r");         /* read mode */    
    if (File == NULL) Check--;
    else Check = 0;
  }

  if (File)        /* file opened */
  {
    Log(L_INFO, "Reading: %s", TempBuffer2);

    while(Run)
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
          if (TempBuffer[Length - 1] != 10)             /* pre-last char is not LF */
          {
            Run = False;                         /* end loop */
            Log(L_WARN, "Input overflow for %s!", TempBuffer2);
          }
        }

        if (Run)       /* if still in business */
        {
          Line++;           /* got another line */

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

          /* if it's not an empty line */
          if (InBuffer[0] != 0)
          {
            /* parse line and extract data fields */
            Flag = Parse_files_bbs(InBuffer, Line, &Info);

            /* if we care about for syntax errors */
            if (!(Env->InfoMode & INFO_RELAX))
            {
              /* end processing on error */
              if (!Flag) Run = False;        /* end loop */ 
            }
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;        /* end loop */
      }
    }

    fclose(File);            /* close file */
  }
  else             /* file error */
  {
    Log(L_WARN, "Couldn't open neither files.bbs nor FILES.BBS!");
  }

  return Flag;
}



/*
 *  write files.bbs
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Write_files_bbs(char *Path)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  FILE              *File;              /* files.bbs stream */
  unsigned short    Check;
  Info_Type         *Info;

  /* sanity check */
  if (Path == NULL) return Flag;

  /* try to open files.bbs or FILES.BBS */
  Check = 2;
  while (Check > 0)
  {
    if (Check == 2) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "files.bbs");
    else if (Check == 1) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "FILES.BBS");

    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1, "%s/%s", Path, TempBuffer2);
    File = fopen(TempBuffer, "r");         /* read mode */    
    if (File == NULL) Check--;
    else Check = 0;
  }


  /* now re-open file in write mode */
  if (File)                            /* got it */
  {
    fclose(File);                        /* close file */
    File = fopen(TempBuffer, "w");       /* truncate & write mode */

    if (File == NULL)                    /* error */
    {
      Run = False;
      Log(L_WARN, "Couldn't write %s!", TempBuffer2);
    }
    else
    {
      Log(L_WARN, "Writing: %s", TempBuffer2);
    }
  }
  else                                 /* none of both */
  {
    Run = False;
    Log(L_WARN, "Couldn't open neither files.bbs nor FILES.BBS!");
  }

  /* follow info list */
  Info = Env->InfoList;
  while (Run && Info)
  {
    if (Info->Status & STAT_OK)      /* ok to list  */
    {
      Run = WriteInfo(File, Info, &(Env->Fields_files_bbs), 78);
    }

    Info = Info->Next;             /* next one */
  }

  /* clean up */
  if (File) fclose(File);

  return Flag = Run;
}



/* ************************************************************************
 *   dir.bbs
 * ************************************************************************ */


/*
 *  read dir.bbs
 *
 *  returns:
 *  - string pointer (new allocation) on success
 *  - NULL on error
 */

char *Read_dir_bbs(char *Path)
{
  char              *Info = NULL;       /* return value */
  _Bool             Run = True;
  unsigned short    Check;
  FILE              *File;
  size_t            Length;

  /* sanity check */
  if (Path == NULL) return Info;

  /* try to open dir.bbs or DIR.BBS */
  Check = 2;
  while (Check > 0)
  {
    if (Check == 2) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "dir.bbs");
    else if (Check == 1) snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "DIR.BBS");

    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1, "%s/%s", Path, TempBuffer2);
    File = fopen(TempBuffer, "r");         /* read mode */    
    if (File == NULL) Check--;
    else Check = 0;
  }

  if (File)        /* file opened */
  {
    Log(L_INFO, "Reading: %s", TempBuffer2);

    /* read just one line */
    if (fgets(InBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
    {
      Length = strlen(InBuffer);

      if (Length == 0)                  /* sanity check */
      {
        Run = False; 
      }
      else if (Length == (DEFAULT_BUFFER_SIZE - 1))         /* maximum size reached */
      {
        /* now check if line matches buffer size exacly or exceeds it */
        /* exact matches should have a LF as last character in front of the trailing 0 */
        if (InBuffer[Length - 1] != 10)             /* pre-last char is not LF */
        {
          Run = False;                         /* end loop */
          Log(L_WARN, "Input overflow for %s!", TempBuffer2);
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

        /* if it's not an empty line */
        if (InBuffer[0] != 0)
        {
          Info = CopyString(InBuffer);
        }
      }
    }

    fclose(File);            /* close file */
  }
  else             /* file error */
  {
    Log(L_WARN, "Couldn't open neither dir.bbs nor DIR.BBS!");
  }

  return Info;
}



/* ************************************************************************
 *   process fileecho
 * ************************************************************************ */


/*
 *  get files from directory
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ScanPath(char *Path)
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* control flag */
  DIR                    *Directory;
  struct dirent          *File;
  struct stat            FileData;

  /* sanity check */
  if (Path == NULL) return Flag;

  Directory = opendir(Path);    /* open directory */
  if (Directory == NULL)        /* error */
  {
    Log(L_WARN, "Can't access directory: %s!", Path);
  }
  else                          /* success */
  {
    if (chdir(Path) != 0)       /* change current directory to path */ 
    {                           /* to simplify lstat later */ 
      Run = False;
    }

    /* get all directory entries and check each name */
    while (Run)
    {
      File = readdir(Directory);     /* get next file */
      if (File)                      /* got it */
      {
        /* check file type */
        if (lstat(File->d_name, &FileData) == 0)
        {
          if (S_ISREG(FileData.st_mode))        /* regular file */
          {
            /* add to global list */
            Flag = AddInfoElement(File->d_name, FileData.st_size, FileData.st_mtime);

            if (Flag)
            {
              /* set file status */
              if (MatchExcludeList(File->d_name))
                Env->LastInfo->Status = STAT_EXCLUDED;
              else
                Env->LastInfo->Status = STAT_OK;
            }
            /* update statistics */
            Env->Files++;
            Env->Bytes += FileData.st_size;
          }
        }
      }
      else                           /* error or no more entries */
      {
        Run = False;                   /* end loop */
      }
    }

    chdir(Env->CWD);            /* change current directory back */
    closedir(Directory);        /* close directory */
  }

  return Flag;
}



/*
 *  process directory
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ProcessPath(char *Name, char *Path, char *AreaInfo)
{
  _Bool             Flag = False;       /* return value */
  char              *DirBBS = NULL;
  char              *Desc = NULL;
  Info_Type         *Info, *Last;
  Field_Type        *Field;
  unsigned short    Limit;

  /* sanity checks */
  if ((Name == NULL) || (Path == NULL)) return Flag;

  Flag = True;

  Log(L_INFO, "Processing path: %s", Path);


  /*
   *  scan directory and get file descriptions
   */

  /* dir.bbs (preceded by info) */
  if ((AreaInfo == NULL) && (Env->InfoMode & INFO_DIR_BBS))
  {
    DirBBS = Read_dir_bbs(Path);
    if (DirBBS == NULL) Flag = False;
  }

  /* scan path for available files */
  if (Flag) Flag = ScanPath(Path);

  /* read file description files */ 
  if (Flag)
  {
    /* files.bbs */
    if (Env->InfoMode & INFO_FILES_BBS)
    {
      Flag = Read_files_bbs(Path);
    }
  }

  /* sort files */
  if (Flag)
  {
    Info = MergeSort(Env->InfoList, &Last);
    Env->InfoList = Info;
    Env->LastInfo = Last;
  }

  /* update file description files */
  if (Flag && (Env->InfoMode & INFO_UPDATE))
  {
    if (Env->InfoMode & INFO_FILES_BBS)
    {
      Write_files_bbs(Path);
    }
  }


  /*
   *  write filelist
   */

  if (Flag)
  {
    /* get line limit */
    Field = Env->Fields_filelist;
    while (Field && (Field->Type != FIELD_DESC)) Field = Field->Next;
    if (Field && (Field->Width > 0)) Limit = Field->Pos + Field->Width - 1;
    else Limit = 78;

    /* separation line */
    FillString(TempBuffer, '-', Limit, DEFAULT_BUFFER_SIZE - 1);
    fprintf(Env->List, "\n%s\n", TempBuffer);

    /* area name */
    fprintf(Env->List, "%s\n", Name);

    /* area description */
    if (AreaInfo) Desc = AreaInfo;
    else if (DirBBS) Desc = DirBBS;
    if (Desc) fprintf(Env->List, "%s\n", Desc);

    /* separation line */
    fprintf(Env->List, "%s\n", TempBuffer);

    /* files */
    Info = Env->InfoList;
    while (Info)                    /* follow list */
    {
      if (Info->Status & STAT_OK)   /* if ok to list  */
      {
        WriteInfo(Env->List, Info, &(Env->Fields_filelist), Limit);
      }

      Info = Info->Next;             /* next one */
    }
  }


  /*
   *  clean up
   */

  if (DirBBS) free(DirBBS);
  if (Env->InfoList)               /* reset global list */
  {
    FreeInfoList(Env->InfoList);
    Env->InfoList = NULL;
    Env->LastInfo = NULL;
  }

  return Flag;
}



/* ************************************************************************
 *   command parser support
 * ************************************************************************ */


/*
 *  convert string into number and check range
 *  - used for data field position or witdh 
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool String2Pos(String, Pos, Min, Max)
  char              *String;
  unsigned short    *Pos;
  unsigned short    Min;
  unsigned short    Max;
{
  _Bool             Flag = False;       /* return value */
  long              Value;

  /* sanity checks */
  if ((String == NULL) || (Pos == NULL)) return Flag;

  if ((Value = Str2Long(String)) > 0)        /* convert */
  {
    if ((Value >= Min) && (Value <= Max))    /* check range */
    {
      *Pos = Value;            /* save value */
      Flag = True;             /* signal success */
    }
  }

  return Flag;
}



/*
 *  convert line-position string into numbers and check ranges
 *  Syntax: [<line>-]<position> or [<line>.]<position>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool String2LinePos(String, Line, Pos, MinPos, MaxPos)
char                *String;
unsigned short      *Line;
unsigned short      *Pos;
unsigned short      MinPos;
unsigned short      MaxPos;
{
  _Bool             Flag = False;       /* return value */
  long              TempLine = 0;
  long              TempPos = 0;
  char              *TempStr;
  char              *Splitter = NULL;   /* pointer to split char */
  int               n = 0;              /* counter for split char */

  /* sanity checks */
  if ((String == NULL) || (Line == NULL) || (Pos == NULL)) return Flag;

  /* search for "." or "-" */
  TempStr = String;
  while (TempStr[0] != 0)       /* follow string */
  {
    if ((TempStr[0] == '.') || (TempStr[0] == '-'))    /* got match */
    {
      Splitter = TempStr;    /* save pointer */
      n++;                   /* increase counter */
    }

    TempStr++;               /* next char */
  }

  if (Splitter && (n == 1))        /* valid splitter */
  {
    Splitter[0] = 0;                 /* create sub string */
    TempLine = Str2Long(String);     /* convert line number */
    Splitter[0] = '-';               /* undo sub string */
    Splitter++;                      /* second sub string */
    TempPos = Str2Long(Splitter);    /* convert position */
  }
  else if (Splitter == NULL)       /* no splitter */
  {
    TempLine = 1;                    /* set default line number */
    TempPos = Str2Long(String);      /* convert position */
  }

  /* check ranges */
  if ((TempLine > 0) && (TempPos > 0))    /* valid conversions */
  {
    /* we limit the line number to 5 in any case */ 
    if ((TempLine <= 5) &&
        ((TempPos >= MinPos) && (TempPos <= MaxPos)))
    {
      *Line = TempLine;          /* save line number */
      *Pos = TempPos;            /* save position */
      Flag = True;               /* signal success */
    }
  }

  return Flag;
}



/* ************************************************************************
 *   commands
 * ************************************************************************ */


/*
 *  include file
 *  Syntax: Include Config <filepath>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Include(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  _Bool                  Error = False;      /* error flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[3] =
    {"Include", "Config", NULL};

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
        case 2:          /* config */
          if (Env->ConfigDepth >= 2)         /* limit cfg depth */
          {
            Log(L_WARN, "Cfg file depth exceeded!");
            Run = False;                     /* end loop */
          }
          else                               /* include cfg file */
          {
            Run = ReadConfig(TokenList->String);
            if (Run == False) Error = True;       /* signal error */
          }
      }

      Keyword = 0;            /* reset */
    }
    else                      /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      switch (Keyword)           /* keywords without data */
      {
        case 0:          /* unknown keyword */
          Run = False;
          break;

        case 1:          /* command itself */
          Keyword = 0;        /* reset keyword */
       }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0))       /* problem occured */
  {
    if (Error == False)                 /* syntax error */
    {
      LogCfgError();
    }
  }
  else                   /* went fine */
  {
    Flag = True;       /* signal success */
  }

  return Flag;
}



/*
 *  set format of files.bbs
 *  Syntax: files.bbs ...
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Define_files_bbs(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  unsigned short    NamePos = 0;
  unsigned short    NameWidth = 0;
  unsigned short    NameAlign = ALIGN_LEFT;
  unsigned short    NameFormat = FIELD_FORM_NONE;
  unsigned short    SizePos = 0;
  unsigned short    SizeWidth = 0;
  unsigned short    SizeAlign = ALIGN_RIGHT;
  unsigned short    SizeFormat = FIELD_FORM_NONE;
  unsigned short    DatePos = 0;
  unsigned short    DateWidth = 0;
  unsigned short    DateAlign = ALIGN_LEFT;
  unsigned short    DateFormat = FIELD_FORM_NONE;
  unsigned short    CounterPos = 0;
  unsigned short    CounterWidth = 0;
  unsigned short    CounterAlign = ALIGN_LEFT;
  unsigned short    CounterFormat = FIELD_FORM_NONE;
  unsigned short    DescPos = 0;
  unsigned short    DescWidth = 0;
  unsigned short    DescAlign = ALIGN_LEFT;
  unsigned short    DescFormat = FIELD_FORM_MULTI;
  static char       *Keywords[11] =
    {"files.bbs", "NameWidth", "NameFormat", "SizePos", "SizeFormat",
     "DatePos", "DateFormat", "CounterPos", "CounterFormat", "DescPos", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* free current field list */
  if (Env->Fields_files_bbs)
  {
    FreeFieldList(Env->Fields_files_bbs);
    Env->Fields_files_bbs = NULL;
  }


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Keyword > 1)           /* get value */
    {
      switch (Keyword)
      {
        case 2:          /* name width */
          Run = String2Pos(TokenList->String, &NameWidth, 10, 60);
          break;

        case 3:          /* name format */
          if (strcasecmp(TokenList->String, "DOS") == 0)
          {
            NamePos = 1;
            NameFormat = FIELD_FORM_DOS;
            NameWidth = 12;    /* 8.3 */
          }
          else if (strcasecmp(TokenList->String, "Long") == 0)
          {
            NamePos = 1;
            NameFormat = FIELD_FORM_LONG;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 4:          /* size position */
          Run = String2Pos(TokenList->String, &SizePos, 1, 120);
          break;

        case 5:          /* size format */
          if (strcasecmp(TokenList->String, "Bytes-8") == 0)
          {
            SizeFormat = FIELD_FORM_BYTES;
            SizeWidth = 8;
          }
          else if (strcasecmp(TokenList->String, "Unit-8") == 0)
          {
            SizeFormat = FIELD_FORM_UNIT;
            SizeWidth = 8;
          }
          else if (strcasecmp(TokenList->String, "Short-8") == 0)
          {
            SizeFormat = FIELD_FORM_SHORT;
            SizeWidth = 8;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 6:          /* date position */
          Run = String2Pos(TokenList->String, &DatePos, 1, 120);
          break;

        case 7:          /* date format */
          if (strcasecmp(TokenList->String, "US") == 0)
          {
            DateFormat = FIELD_FORM_US;
            DateWidth = 8;
          }
          else if (strcasecmp(TokenList->String, "ISO") == 0)
          {
            DateFormat = FIELD_FORM_ISO;
            DateWidth = 10;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 8:          /* counter position */
          Run = String2Pos(TokenList->String, &CounterPos, 1, 120);
          break;

        case 9:          /* counter format */
          if (strcasecmp(TokenList->String, "Square-2") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 4;
          }
          else if (strcasecmp(TokenList->String, "Square-3") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 5;
          }
          else if (strcasecmp(TokenList->String, "Square-4") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 6;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 10:         /* description position */
          Run = String2Pos(TokenList->String, &DescPos, 1, 120);
      }

      Keyword = 0;                       /* reset */
    }
    else                      /* check for known keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0) Run = False;    /* unknown keyword */ 
    }

    TokenList = TokenList->Next;     /* next token */
  }


  /*
   *  check values
   */

  if (Run)
  {
    /* FileFormat must be set */
    if (NameFormat == FIELD_FORM_NONE)
    {
      Run = False;
      Log(L_WARN, "NameFormat argument required!");
    }

    /* for DOS filenames only a width of 12 is valid */
    if ((NameFormat == FIELD_FORM_DOS) && (NameWidth != 12))
    {
      Run = False;
      Log(L_WARN, "Wrong NameWidth!");
    }

    /* long filenames require the width option */
    if ((NameFormat == FIELD_FORM_LONG) && (NameWidth == 0))
    {
      Run = False;
      Log(L_WARN, "NameWidth argument required!");
    }

    /* if size field is enabled, we need to have position and format */
    /* same for date and counter fields */
    if (((SizePos > 0) ^ (SizeFormat != FIELD_FORM_NONE)) ||
        ((DatePos > 0) ^ (DateFormat != FIELD_FORM_NONE)) ||
        ((CounterPos > 0) ^ (CounterFormat != FIELD_FORM_NONE)))
    {
      Run = False;
      Log(L_WARN, "Missing argument (Pos and Format)!");
    } 

    /* DescPos must be set */
    if (DescPos == 0)
    {
      Run = False;
      Log(L_WARN, "DescPos argument required!");
    }

    /* description needs to be the last field */
    if (DescPos > 0)
    {
      if ((DescPos <= SizePos) ||
          (DescPos <= DatePos) ||
          (DescPos <= CounterPos))
      {
        Run = False;
        Log(L_WARN, "Description must be at the end of the line!");
      }
    }
  }


  /*
   *  add info fields to global list
   */

  if (Run)
  {
    if (NamePos > 0) Run &= InsertField(&(Env->Fields_files_bbs), FIELD_NAME,
      1, NamePos, NameWidth, NameAlign, NameFormat); 

    if (SizePos > 0) Run &= InsertField(&(Env->Fields_files_bbs), FIELD_SIZE,
      1, SizePos, SizeWidth, SizeAlign, SizeFormat);

    if (DatePos > 0) Run &= InsertField(&(Env->Fields_files_bbs), FIELD_DATE,
      1, DatePos, DateWidth, DateAlign, DateFormat);

    if (CounterPos > 0) Run &= InsertField(&(Env->Fields_files_bbs), FIELD_COUNTER,
      1, CounterPos, CounterWidth, CounterAlign, CounterFormat);

    if (DescPos > 0) Run &= InsertField(&(Env->Fields_files_bbs), FIELD_DESC,
      1, DescPos, DescWidth, DescAlign, DescFormat);    
  }

  if ((!Run) || (Keyword > 0))
  {
    LogCfgError();
  }
  else
  {
    Flag = True;    /* signal success */
  }

  return Flag;
}



/*
 *  set format of filelist
 *  Syntax: filelist ...
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Define_filelist(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  unsigned short    Check;              /* counter for checking stuff */
  unsigned short    TempLine;
  unsigned short    TempPos;
  unsigned short    NameLine = 1;
  unsigned short    NamePos = 1;
  unsigned short    NameWidth = 0;
  unsigned short    NameAlign = ALIGN_LEFT;
  unsigned short    NameFormat = FIELD_FORM_NONE;
  unsigned short    SizeLine = 1;
  unsigned short    SizePos = 0;
  unsigned short    SizeWidth = 0;
  unsigned short    SizeAlign = ALIGN_RIGHT;
  unsigned short    SizeFormat = FIELD_FORM_NONE;
  unsigned short    DateLine = 1;
  unsigned short    DatePos = 0;
  unsigned short    DateWidth = 0;
  unsigned short    DateAlign = ALIGN_LEFT;
  unsigned short    DateFormat = FIELD_FORM_NONE;
  unsigned short    CounterLine = 1;
  unsigned short    CounterPos = 0;
  unsigned short    CounterWidth = 0;
  unsigned short    CounterAlign = ALIGN_LEFT;
  unsigned short    CounterFormat = FIELD_FORM_NONE;
  unsigned short    DescLine = 1;
  unsigned short    DescPos = 0;
  unsigned short    DescWidth = 0;
  unsigned short    DescAlign = ALIGN_LEFT;
  unsigned short    DescFormat = FIELD_FORM_MULTI;
  static char       *Keywords[14] =
    {"filelist", "NamePos", "NameWidth", "NameFormat", "SizePos",
     "SizeWidth", "SizeFormat", "DatePos", "DateFormat", "CounterPos", 
     "CounterFormat", "DescPos", "DescWidth", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* free current field list */
  if (Env->Fields_filelist)
  {
    FreeFieldList(Env->Fields_filelist);
    Env->Fields_filelist = NULL;
  }


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)     /* loop through tokens */
  {
    if (Keyword > 1)     /* get value */
    {
      switch (Keyword)
      {
        case 2:          /* name position */
          Run = String2LinePos(TokenList->String, &NameLine, &NamePos, 1, 120);
          break;

        case 3:          /* name width */
          Run = String2Pos(TokenList->String, &NameWidth, 12, 60);
          break;

        case 4:          /* name format */
          if (strcasecmp(TokenList->String, "DOS") == 0)
          {
            NameFormat = FIELD_FORM_DOS;
            NameWidth = 12;    /* 8.3 */
          }
          else if (strcasecmp(TokenList->String, "Long") == 0)
          {
            NameFormat = FIELD_FORM_LONG;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 5:               /* size position */
          Run = String2LinePos(TokenList->String, &SizeLine, &SizePos, 1, 120);
          break;

        case 6:          /* size width */
          Run = String2Pos(TokenList->String, &SizeWidth, 4, 12);
          break;

        case 7:          /* size format */
          if (strcasecmp(TokenList->String, "Bytes") == 0)
          {
            SizeFormat = FIELD_FORM_BYTES;
          }
          else if (strcasecmp(TokenList->String, "Unit") == 0)
          {
            SizeFormat = FIELD_FORM_UNIT;
          }
          else if (strcasecmp(TokenList->String, "Short") == 0)
          {
            SizeFormat = FIELD_FORM_SHORT;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 8:          /* date position */
          Run = String2LinePos(TokenList->String, &DateLine, &DatePos, 1, 120);
          break;

        case 9:          /* date format */
          if (strcasecmp(TokenList->String, "US") == 0)
          {
            DateFormat = FIELD_FORM_US;
            DateWidth = 10;
          }
          else if (strcasecmp(TokenList->String, "ISO") == 0)
          {
            DateFormat = FIELD_FORM_ISO;
            DateWidth = 10;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 10:      /* counter position */
          Run = String2LinePos(TokenList->String, &CounterLine, &CounterPos, 1, 120);
          break;

        case 11:      /* counter format */
          if (strcasecmp(TokenList->String, "Square-2") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 4;
          }
          else if (strcasecmp(TokenList->String, "Square-3") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 5;
          }
          else if (strcasecmp(TokenList->String, "Square-4") == 0)
          {
            CounterFormat = FIELD_FORM_SQUARE;
            CounterWidth = 6;
          }
          else Run = False;       /* unknown keyword */
          break;

        case 12:      /* description position */
          Run = String2LinePos(TokenList->String, &DescLine, &DescPos, 1, 120);
          break;

        case 13:      /* description width */
          Run = String2Pos(TokenList->String, &DescWidth, 18, 80);
      }

      Keyword = 0;                       /* reset */
    }
    else                       /* check for known keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0) Run = False;     /* unknown keyword */ 
    }

    TokenList = TokenList->Next;     /* next token */
  }


  /*
   *  check values
   */

  if (Run)
  {
    /* FileFormat must be set */
    if (NameFormat == FIELD_FORM_NONE)
    {
      Run = False;
      Log(L_WARN, "NameFormat argument required!");
    }

    /* for DOS filenames only a width of 12 is valid */
    if ((NameFormat == FIELD_FORM_DOS) && (NameWidth != 12))
    {
      Run = False;
      Log(L_WARN, "Wrong NameWidth!");
    }

    /* long filenames require the width option */
    if ((NameFormat == FIELD_FORM_LONG) && (NameWidth == 0))
    {
      Run = False;
      Log(L_WARN, "NameWidth argument required!");
    }

    /* if size field is enabled, we need to have position, width and format */
    Check = 0;
    if (SizePos > 0) Check++;
    if (SizeWidth> 0) Check++;
    if (SizeFormat != FIELD_FORM_NONE) Check++;
    if ((Check != 0) && (Check != 3))
    {
      Run = False;
      Log(L_WARN, "Missing argument for size field!");
    }

    /* if date field is enabled, we need to have position and format */
    Check = 0;
    if (DatePos > 0) Check++;
    if (DateWidth> 0) Check++;
    if (DateFormat != FIELD_FORM_NONE) Check++;
    if ((Check != 0) && (Check != 3))
    {
      Run = False;
      Log(L_WARN, "Missing argument for date field!");
    }

    /* if date field is enabled, we need to have position and format */
    Check = 0;
    if (CounterPos > 0) Check++;
    if (CounterWidth> 0) Check++;
    if (CounterFormat != FIELD_FORM_NONE) Check++;
    if ((Check != 0) && (Check != 3))
    {
      Run = False;
      Log(L_WARN, "Missing argument for counter field!");
    }

    /* DescPos and DescWidth must be set */
    if ((DescPos == 0) || (DescWidth == 0))
    {
      Run = False;
      Log(L_WARN, "DescPos/DescWidth argument required!");
    }

    /* description needs to be the last field */
    if (DescPos > 0)
    {
      Run = False;         /* reset flag */

      /* find larget line-position */
      TempLine = 0;
      TempPos = 0;

      /* find largest line number */
      if (NameLine > TempLine) TempLine = NameLine;
      if (SizeLine > TempLine) TempLine = SizeLine;
      if (DateLine > TempLine) TempLine = DateLine;
      if (CounterLine > TempLine) TempLine = CounterLine;

      /* if descriptions line number is largest */
      if (DescLine > TempLine) Run = True;

      /* if descriptions line number matches check for position */
      else if (DescLine == TempLine)
      {
        /* find largest position */
        if ((NameLine == TempLine) && (NamePos > TempPos)) TempPos = NamePos;
        if ((SizeLine == TempLine) && (SizePos > TempPos)) TempPos = SizePos;
        if ((DateLine == TempLine) && (DatePos > TempPos)) TempPos = DatePos;
        if ((CounterLine == TempLine) && (CounterPos > TempPos)) TempPos = CounterPos;

        if (DescPos > TempPos) Run = True;
      }

      if (!Run) Log(L_WARN, "Description must be at the end of the (last) line!");
    }
  }


  /*
   *  add info fields to global list
   */

  if (Run)
  {
    if (NamePos > 0) Run &= InsertField(&(Env->Fields_filelist), FIELD_NAME,
      NameLine, NamePos, NameWidth, NameAlign, NameFormat); 

    if (SizePos > 0) Run &= InsertField(&(Env->Fields_filelist), FIELD_SIZE,
      SizeLine, SizePos, SizeWidth, SizeAlign, SizeFormat);

    if (DatePos > 0) Run &= InsertField(&(Env->Fields_filelist), FIELD_DATE,
      DateLine, DatePos, DateWidth, DateAlign, DateFormat);

    if (CounterPos > 0) Run &= InsertField(&(Env->Fields_filelist), FIELD_COUNTER,
      CounterLine, CounterPos, CounterWidth, CounterAlign, CounterFormat);

    if (DescPos > 0) Run &= InsertField(&(Env->Fields_filelist), FIELD_DESC,
      DescLine, DescPos, DescWidth, DescAlign, DescFormat);    
  }

  if ((!Run) || (Keyword > 0))
  {
    LogCfgError();
  }
  else
  {
    Flag = True;    /* signal success */
  }

  return Flag;
}



/*
 *  define syntax of file description file
 *  Syntax: Define [files.bbs ...]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Define(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /*
   *  This is just the pre-parser to find out which description file
   *  will be defined and to call the according parser function.
   */


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: file */
    {
      if (strcasecmp(TokenList->String, "files.bbs") == 0)
      {
        Flag = Cmd_Define_files_bbs(TokenList);
        TokenList = NULL;                 /* end loop */
      }
      else if (strcasecmp(TokenList->String, "Filelist") == 0)
      {
        Flag = Cmd_Define_filelist(TokenList);
        TokenList = NULL;                 /* end loop */
      }
      else Run = False;       /* unknown keyword */

      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "Define") == 0)   /* keyword */
    {
      Get = 1;
    }
    else                                               /* unknown */
    {
      Run = False;
    }

    if (TokenList) TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0))
  {
    LogCfgError();
  }

  return Flag;
}



/*
 *  add directory to filelist (shared with mfreq-index)
 *  Syntax: FileArea Name <name> Path <path> [Info <description>]
 *            [PW <password>] [Depth <depth>] [AutoMagic]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_SharedFileArea(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  char              *Name = NULL;
  char              *Path = NULL;
  char              *Info = NULL;
  static char       *Keywords[8] =
    {"SharedFileArea", "Name", "Path", "Info", "PW",
     "Depth", "AutoMagic", NULL};

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
        case 2:     /* name */
          Name = TokenList->String;
          break;
      
        case 3:     /* path */
          Path = TokenList->String;
          break;

        case 4:     /* info */
          Info = TokenList->String;

        /* Other keywords with values belong to mfreq-index,
           so we simply skip them. */
      }

      Keyword = 0;             /* reset */
    }
    else                   /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      switch (Keyword)        /* keywords without data */
      {
        case 0:               /* unknown keyword */
          Run = False;
          break;

        case 1:               /* command itself */
          /* simply skip it */
          Keyword = 0;        /* reset keyword */
          break;

        case 7:               /* automagic switch */
          /* skip it (mfreq-index) */
          Keyword = 0;        /* reset keyword */
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Name == NULL) || (Path == NULL))
  {
printf("%d - %d - %s - %s\n", Run, Keyword, Name, Path);
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    /* open path and process files */
    Flag = ProcessPath(Name, Path, Info);
  }

  return Flag;
}



/*
 *  add directory to filelist
 *  Syntax: FileArea <name> Path <path> [Info <description>]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_FileArea(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  char              *Name = NULL;
  char              *Path = NULL;
  char              *Info = NULL;
  static char       *Keywords[4] =
    {"FileArea", "Path", "Info", NULL};

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
        case 1:     /* name */
          Name = TokenList->String;
          break;
      
        case 2:     /* path */
          Path = TokenList->String;
          break;

        case 3:     /* info */
          Info = TokenList->String;
      }

      Keyword = 0;             /* reset */
    }
    else                   /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0) Run = False;    /* unknown keyword */
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Name == NULL) || (Path == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    /* open path and process files */
    Flag = ProcessPath(Name, Path, Info);
  }

  return Flag;
}



/*
 *  add text to filelist
 *  Syntax: AddText <text>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_AddText(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *TextToken = NULL;

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
    else if (strcasecmp(TokenList->String, "AddText") == 0)   /* text */
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

  if ((Run == False) || (Get > 0) || (TextToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    if (Env->List)             /* if filelist is opened */
    {
      /* add text to filelist */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1, "%s\n",
        TextToken->String);
      if (fputs(TempBuffer, Env->List) >= 0) Flag = True;
    }
  }

  return Flag;
}



/*
 *  open filelist
 *  Syntax: FileList <filepath>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_FileList(Token_Type *TokenList)
{
  _Bool             Flag = False;            /* return value */
  _Bool             Run = True;              /* control flag */
  unsigned short    Get = 0;                 /* mode control */
  Token_Type        *FilepathToken = NULL;

  /* sanity check */
  if (TokenList == NULL) return Flag;


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
    else if (strcasecmp(TokenList->String, "FileList") == 0)   /* filepath */
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

  if (Run)
  {
    if (Env->Fields_filelist == NULL)
    {
      Run = False;
      Log(L_WARN, "No format for filelist specified!");
      LogCfgError();
    }
  }


  /*
   *  process
   */

  if (Run)
  {
    /* first close old filelist */
    if (Env->List) CloseFilelist();

    Env->ListFilepath = FilepathToken->String;   /* move string */
    FilepathToken->String = NULL;

    /* reset statistics */
    Env->Files = 0;
    Env->Bytes = 0;

    Flag = OpenFilelist();
  }

  return Flag;
}



/*
 *  reset stuff
 *  Syntax: Reset [InfoMode] [Excludes] 
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Reset(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[4] =
    {"Reset", "InfoMode", "Excludes", NULL};

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
        break;

      case 1:       /* command itself */
        /* just skip */
        break;

      case 2:       /* InfoMode */
        Env->InfoMode = INFO_NONE;      /* set switches to default */
        break;

      case 3:       /* Excludes */
        if (Env->ExcludeList)           /* clear file excludes */
        {
          FreeExcludeList(Env->ExcludeList);
          Env->ExcludeList = NULL;
        }
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
 *  file info mode
 *  Syntax: InfoMode [dir.bbs] [files.bbs] [Update] [Strict] |Skips]
 *                   [Relax] [SI-Units] [IEC-Units]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_InfoMode(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  unsigned short         Mode = INFO_NONE;
  unsigned short         Switches = SW_NONE;
  static char            *Keywords[10] =
    {"InfoMode", "dir.bbs", "files.bbs", "Update", "Strict",
     "Skips", "Relax", "SI-Units", "IEC-Units", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    Keyword = GetKeyword(Keywords, TokenList->String);

    switch (Keyword)      /* keywords without data */
    {
      case 0:       /* unknown keyword */
        Run = False;
        break;

      case 1:       /* command itself */
        /* just skip */
        break;

      case 2:       /* dir.bbs */
        Mode |= INFO_DIR_BBS;
        break;

      case 3:       /* files.bbs */
        Mode |= INFO_FILES_BBS;
        if (Env->Fields_files_bbs == NULL)
        {
          Log(L_WARN, "Format of files.bbs not defined!");
          Run = False;
        }
        break;

      case 4:       /* update */
        Mode |= INFO_UPDATE;
        break;

      case 5:       /* strict */
        Mode |= INFO_STRICT;
        break;

      case 6:       /* skips */
        Mode |= INFO_SKIPS;
        break;

      case 7:       /* relax */
        Mode |= INFO_RELAX;
        break;

      case 8:       /* SI units */
        Switches |= SW_SI_UNITS;
        break;

      case 9:       /* IEC units for output */
        Switches |= SW_IEC_UNITS;
        break;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if (Run == False)           /* error */
  {
    LogCfgError();
  }
  else                        /* success */
  {
    Env->InfoMode = Mode;               /* set new infomode */
    /* unset bits for local switches */
    Env->CfgSwitches &= ~(SW_SI_UNITS | SW_IEC_UNITS);
    Env->CfgSwitches |= Switches;       /* update switches */
    Flag = True;
  }

  return Flag;
}



/*
 *  exclude specific filename
 *  Syntax: Exclude <name or pattern>
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Exclude(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *NameToken = NULL;

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)                  /* get value: name */
    {
      NameToken = TokenList;

      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "Exclude") == 0)   /* name */
    {
      Get = 1;
    }
    else                                                 /* unknown */
    {
      Run = False;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Get > 0) || (NameToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    /* add name to global exclude list */
    Flag = AddExcludeElement(NameToken->String);
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

_Bool Cmd_LogFile(Token_Type *TokenList)
{
  _Bool             Flag = False;            /* return value */
  _Bool             Run = True;              /* control flag */
  unsigned short    Get = 0;                 /* mode control */
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
  _Bool                  Flag = False;        /* return value */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[11] =
    {"FileArea", "SharedFileArea", "AddText", "Exclude", "Include",
     "InfoMode", "Reset", "Define", "LogFile", "FileList", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* find command and call corresponding function */
  if (TokenList->String)
  {
    Keyword = GetKeyword(Keywords, TokenList->String);

    switch (Keyword)           /* setting keywords */
    {
      case 0:       /* unknown command */
        Log(L_WARN, "Unknown setting in cfg file (%s), line %d (%s)!",
          Env->CfgInUse, Env->CfgLinenumber, TokenList->String);        
        break;

      case 1:       /* filearea */
        Flag = Cmd_FileArea(TokenList);
        break;

      case 2:       /* shared filearea */
        Flag = Cmd_SharedFileArea(TokenList);
        break;

      case 3:       /* add text */
        Flag = Cmd_AddText(TokenList);
        break;

      case 4:       /* exclude */
        Flag = Cmd_Exclude(TokenList);
        break;

      case 5:       /* include */
        Flag = Cmd_Include(TokenList);
        break;

      case 6:       /* info mode */
        Flag = Cmd_InfoMode(TokenList);
        break;

      case 7:       /* reset */
        Flag = Cmd_Reset(TokenList);
        break;

      case 8:       /* define */
        Flag = Cmd_Define(TokenList);
        break;

      case 9:       /* logfile */
        Flag = Cmd_LogFile(TokenList);
        break;

      case 10:      /* filelist */
        Flag = Cmd_FileList(TokenList);
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
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* loop control */
  FILE                   *File;              /* filestream */
  size_t                 Length;
  char                   *HelpStr;
  Token_Type             *TokenList;
  unsigned int           Line = 0;           /* line number */
  unsigned int           OldLine;            /* old linenumber */
  char                   *OldCfg;            /* old filepath */

  /* sanity check */
  if (Filepath == NULL) return Flag;

  Env->ConfigDepth++;              /* increase cfg file depth */

  File = fopen(Filepath, "r");               /* read mode */
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

        if (Length == 0)                /* sanity check */
        {
          Run = False;                       /* end loop */
        }
        else if (Length == (DEFAULT_BUFFER_SIZE - 1))  /* maximum size reached */
        {
          /* now check if line matches buffer size exacly or exceeds it */
          /* exact matches should have a LF as last character in front of the trailing 0 */
          if (InBuffer[Length - 1] != 10)              /* pre-last char is not LF */
          {
            Run = False;                               /* end loop */
            Log(L_WARN, "Input overflow for line %d in cfg file (%s)!", ++Line, Filepath);
          }
        }

        if (Run)         /* if still in business */
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
            TokenList = Tokenize(HelpStr);        /* tokenize line */

            if (TokenList)
            {
              Flag = ParseConfig(TokenList);
              if (Flag == False) Run = False;     /* end loop on error */
              FreeTokenlist(TokenList);           /* free linked list */
            }
          }
        }
      }
      else                    /* EOF or error */
      {
        Run = False;          /* end loop */
      }
    }

    /* clean up */
    fclose(File);                  /* close file */
    Env->CfgInUse = OldCfg;        /* restore old cfg filepath */
    Env->CfgLinenumber = OldLine;  /* restore old linenumber */
  }
  else                        /* error */
  {
    Log(L_WARN, "Couldn't open cfg file (%s)!", Filepath);
  }

  if (Env->ConfigDepth > 0) Env->ConfigDepth--;   /* decrease cfg file depth */

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
  printf("Usage: "NAME" [options]\n");
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
  _Bool              Flag = True;        /* return value */
  unsigned int       n = 1;              /* loop counter */
  unsigned short     Keyword = 0;        /* keyword ID */
  static char        *Keywords[5] =
    {"-h", "-?", "-c", "-l", NULL};

  /* sanity checks */
  if ((argc == 0) || (argv == NULL)) return False;


  /*
   *  parse command line options
   */

  while (n < argc)                            /* as long as we got options, check: */
  {
    if (Keyword > 2)          /* get value */
    {
      switch (Keyword)
      {
        case 3:     /* cfg file */
          if (Env->CfgFilepath)      /* free old value if already set */
          {
            free(Env->CfgFilepath);
            Env->CfgFilepath = NULL;
          }
          Env->CfgFilepath = CopyString(argv[n]);
          break;

        case 4:     /* log file */
          if (Env->LogFilepath)      /* free old value if already set */
          {
            free(Env->LogFilepath);
            Env->LogFilepath = NULL;
          }
          Env->LogFilepath = CopyString(argv[n]);
          break;
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

  if (Keyword > 0)             /* missing arguement  */
  {
    Log(L_WARN, "Missing argument!");
    Flag = False;
  }

  /* check if we got all required options */
  if (Flag)           /* if everything's fine so far */
  {
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
  _Bool                 Flag = FALSE;

  /* buffers */
  LogBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  InBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  OutBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  TempBuffer = (char *) malloc(DEFAULT_BUFFER_SIZE);
  TempBuffer2 = (char *) malloc(DEFAULT_BUFFER_SIZE);

  /* environment / configuration */
  Env = calloc(1, sizeof(Env_Type));

  /* check pointeris */
  if (LogBuffer && InBuffer && OutBuffer &&
      TempBuffer && TempBuffer2 && Env)
  {
    Flag = TRUE;        /* ok to proceed */
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
    Env->List = NULL;

    /* environment: program control */
    Env->Run = True;
    Env->ConfigDepth = 0;
    Env->CfgInUse = NULL;
    Env->CfgLinenumber = 0;

    /* environment: common configuration */
    Env->CfgSwitches = SW_NONE;

    /* environment: file index */
    Env->ExcludeList = NULL;
    Env->LastExclude = NULL;

    /* environment: filelist */
    Env->ListFilepath = NULL;
    Env->InfoMode = INFO_NONE;
    Env->InfoList = NULL;
    Env->LastInfo = NULL;
    Env->Fields_filelist = NULL;
    Env->Fields_files_bbs = NULL;
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
    /* data inside structure */
    if (Env->CfgFilepath) free(Env->CfgFilepath);
    if (Env->LogFilepath) free(Env->LogFilepath);
    if (Env->CWD) free(Env->CWD);
    if (Env->ListFilepath) free(Env->ListFilepath);

    /* linked lists */
    if (Env->ExcludeList) FreeExcludeList(Env->ExcludeList);
    if (Env->InfoList) FreeInfoList(Env->InfoList);
    if (Env->Fields_filelist) FreeFieldList(Env->Fields_filelist);
    if (Env->Fields_files_bbs) FreeFieldList(Env->Fields_files_bbs);

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
    if (ParseCommandLine(argc, argv))   /* parse local cmd line options */
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
   *  process configuration
   */

  if (Flag && Env->Run)
  {
    Flag = ReadConfig(Env->CfgFilepath);
  }


  /*
   *  clean up
   */

  if (Env)
  {
    if (Env->List) CloseFilelist();

    if (Env->Run)             /* log "done" */
    {
      if (Flag) Log(L_INFO, NAME" "VERSION" ended.");
      else Log(L_INFO, NAME" "VERSION" ended with error!");
    }

    if (Env->Log)             /* close logfile */
    {
      UnlockFile(Env->Log);
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

#undef MFREQ_LIST_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
