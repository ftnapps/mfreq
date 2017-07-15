/* ************************************************************************
 *
 *   mfreq-index
 *
 *   (c) 1994-2017 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define MFREQ_INDEX_C


/* defaults for this program */
#define NAME            "mfreq-index"
#define CFG_FILENAME    "index.cfg"


/*
 *  include header files
 */


/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */


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


/*
 *  local functions
 */

_Bool ReadConfig(char *Filepath);



/* ************************************************************************
 *   data sorting
 * ************************************************************************ */


/*
 *  sort file data by name
 *  - algorithm: merge sort
 *  - lower case > upper case
 */

IndexData_Type *MergeSort(IndexData_Type *List, IndexData_Type **Last)
{
  IndexData_Type        *NewList = NULL;       /* return value */
  IndexData_Type        *LeftSub;              /* left sublist */
  IndexData_Type        *RightSub;             /* right sublist */
  IndexData_Type        *Element = NULL;       /* single element */
  IndexData_Type        *MergedList;           /* merged list */
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
      Run = False;              /* end master loop */

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
 *  check if file exists and is a regular file
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool CheckFile(char *Filepath)
{
  _Bool                  Flag = False;        /* return value */
  struct stat            FileData;

  /* sanity check */
  if (Filepath == NULL) return Flag;

  /* check file type */
  if (lstat(Filepath, &FileData) == 0)
  {
    if (S_ISREG(FileData.st_mode))        /* regular file */
    {
      Flag = True;
    }
    else
    {
      Log(L_WARN, "No regular file (%s)!", Filepath); 
    }
  }
  else
  {
    Log(L_WARN, "Can't access file (%s)!", Filepath);
  }

  return Flag;
}



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
 *  read magic file and add filepaths to fileindex
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ReadMagicFile(char *Filename)
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* control flag */
  FILE                   *File;               /* filestream */
  size_t                 Length;

  File = fopen(Filename, "r");         /* read mode */
  if (File)
  {
    while (Run)
    {
      /* read line-wise */
      if (fgets(TempBuffer, DEFAULT_BUFFER_SIZE, File) != NULL)
      {
        Length = strlen(TempBuffer);

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
            Log(L_WARN, "Input overflow for magic file (%s)!", Filename);
          }
        }

        if (Run)       /* if still in business */
        {
          /* remove LF at end of line */
          if (TempBuffer[Length - 1] == 10)
          {
            TempBuffer[Length - 1] = 0;
            Length--;
          }

          /* check if filepath is a regular file */
          if (CheckFile(TempBuffer))
          {
            /* add to file index */
            Run = AddDataElement(Filename, TempBuffer, NULL);
            if (Run) Env->Files++;       /* increase file counter */
          }
        }
      }
      else                     /* EOF or error */
      {
        Run = False;        /* end loop */
      }
    }

    Flag = True;            /* signal success */
    fclose(File);           /* close file */
  }
  else
  {
    Log(L_WARN, "Couldn't open magic file (%s)!", Filename);
  }

  return Flag;
}


/*
 *  get magic files from directory
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ProcessMagicPath(char *Path)
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
    Log(L_WARN, "Can't access directory (%s)!", Path);
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
            /* let's read the magic file if it's not excluded */
            if (! MatchExcludeList(File->d_name))
              Flag = ReadMagicFile(File->d_name);
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
 *  get files from a directory and add them to the fileindex
 *  - supports recursive directory processing
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ProcessPath(char *Path, char *PW, int Depth, _Bool AutoMagic)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  _Bool                  AliasFlag = False;  /* path aliases */
  _Bool                  AnyCase = False;    /* case-insensive search */
  DIR                    *Directory;
  struct dirent          *File;
  struct stat            FileData;
  char                   *LocalPath = NULL;  /* absolute path */
  char                   *SubPath = NULL;    /* path of sub-directory */
  char                   *Help, *LastDot;
  size_t                 Length;
  unsigned int           AliasNumber = 0;    /* alias counter */
  off_t                  AliasOffset = -1;   /* alias file offset */

  /* sanity check */
  if ((Path == NULL) || (Depth < 0)) return Flag;

  /* case-insensive search */
  if (Env->CfgSwitches & SW_ANY_CASE) AnyCase = True;

  /* build absolute path if necessary */
  if (Path[0] != '/')      /* relative path */
  {
    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
      "%s/%s", Env->CWD, Path);
    LocalPath = CopyString(TempBuffer);
    Path = LocalPath;
  }

  Directory = opendir(Path);    /* open directory */

  if (Directory != NULL)        /* dir opened */
  {
    if (chdir(Path) != 0)       /* change current directory to path */ 
    {                           /* to simplify lstat later */ 
      Run = False;
    }

    /*
     *  path aliasing
     */

    if (Env->CfgSwitches & SW_PATH_ALIASES)   /* if enabled */
    {
      /* get and set top number */
      if (Env->LastAlias) AliasNumber = Env->LastAlias->Number;
      AliasNumber++;

      /* check for overflow */
      if (AliasNumber <= 10000)        /* limit to 10000 */
      {
        /* add alias to list */
        if (AddAliasElement(AliasNumber, Path))
        {
          AliasOffset = Env->LastAlias->Offset;
          AliasFlag = True;            /* perform aliasing */
        }
        else
        {
          Run = False;
        }
      }
      /* else: AliasFlag is still false */
    }


    /* intermediate check */
    if (Run)
    {
      Flag = True;            /* now true by default */
    }
    else
    {
      Log(L_WARN, "Processing error (%s)!", Path);
    }


    /*
     *  get all directory entries and process each one
     */

    while (Run)
    {
      File = readdir(Directory);     /* get next file */

      if (File)                      /* got it */
      {
        /* copy file name (File isn't re-entrant) */
        snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "%s", File->d_name);

        /* check file type */
        if (lstat(TempBuffer2, &FileData) == 0)
        {
          if (S_ISREG(FileData.st_mode))        /* regular file */
          {
            /* add file to index if not excluded */
            if (! MatchExcludeList(TempBuffer2))
            {
              /* build filepath */
              /* omit filename (automatic filepath) */ 
              if (AliasFlag)            /* path alias enabled */
              {
                /* create aliased path without filename */
                /* format: %<alias offset>%/ */
                snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
                  "%%%ld%%/", AliasOffset);
              }
              else                      /* path alias disabled */
              {
                /* create full path whithout filename */
                snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
                  "%s/", Path);
              }

              if (AnyCase)              /* case-insensitive search */
              {
                /*
                 *  For AnyCase we have to convert the filename to uppper case
                 *  later on. So we need to add the original filename to the path.
                 */

                /* add filename to path */
                Length = strlen(TempBuffer);
                Help = &TempBuffer[Length];       /* end of path */
                snprintf(Help, DEFAULT_BUFFER_SIZE - 1 - Length,
                    "%s", TempBuffer2);                
              }

              Flag = AddDataElement(TempBuffer2, TempBuffer, PW);
              if (Flag) Env->Files++;       /* increase file counter */

              if (AutoMagic)         /* auto magic enabled */
              {
                /* find last "." in filename */
                Help = TempBuffer2;
                LastDot = NULL;
                while (Help[0] != 0)         /* scan string */
                {
                  if (Help[0] == '.') LastDot = Help;
                  Help++;                    /* next char */
                }

                /* create magic */
                if (LastDot)                 /* got extension */
                {
                  /* add filename to path */
                  Length = strlen(TempBuffer);
                  Help = &TempBuffer[Length];    /* end of path */
                  snprintf(Help, DEFAULT_BUFFER_SIZE - 1 - Length,
                    "%s", TempBuffer2);

                  LastDot[0] = 0;            /* create sub-string */

                  Flag = AddDataElement(TempBuffer2, TempBuffer, PW); 
                }
              }
            }
          }
          else if (S_ISDIR(FileData.st_mode))   /* directory */
          {
            /* enter recursion but skip "." and ".." */
            if ((Depth > 0) &&
                (strcmp(TempBuffer2, ".") != 0) &&
                (strcmp(TempBuffer2, "..") != 0))
            {
              /* create sub path */
              snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
                "%s/%s", Path, TempBuffer2);
              SubPath = CopyString(TempBuffer);
              Flag = ProcessPath(SubPath, PW, Depth - 1, AutoMagic);

              if (SubPath)         /* free path */
              {
                free(SubPath);
                SubPath = NULL;
              }

              chdir(Path);     /* chdir back to current directory */
            }
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
  else                          /* error */
  {
    Log(L_WARN, "Can't access directory (%s)!", Path);
  }

  /* clean up */
  if (LocalPath) free(LocalPath);

  return Flag;
}



/*
 *  get files from a directory and add them to global filelist
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool ScanPath(char *Path, char *Pattern)
{
  _Bool                  Flag = False;        /* return value */
  _Bool                  Run = True;          /* loop control flag */
  _Bool                  Add;                 /* data control flag */
  DIR                    *Directory;
  struct dirent          *File;
  struct stat            FileData;

  /* sanity check */
  if (Path == NULL) return Flag;

  Directory = opendir(Path);    /* open directory */

  if (Directory != NULL)        /* dir opened */
  {
    if (chdir(Path) != 0)       /* change current directory to path */ 
    {                           /* to simplify lstat later */ 
      Run = False;
    }

    Flag = Run;                 /* update flag */


    /*
     *  get all directory entries and process each one
     */

    while (Run)
    {
      File = readdir(Directory);     /* get next file */

      if (File)                      /* got it */
      {
        /* copy file name (File isn't re-entrant) */
        snprintf(TempBuffer2, DEFAULT_BUFFER_SIZE - 1, "%s", File->d_name);

        /* get file details */
        if (lstat(TempBuffer2, &FileData) == 0)
        {
          /* only process regular files */
          if (S_ISREG(FileData.st_mode))        /* regular file */
          {
            /* if name is not excluded */
            if (! MatchExcludeList(TempBuffer2))
            {
              Add = True;               /* add file by default */

              /* check if name matches pattern  */
              if (Pattern && !MatchPattern(TempBuffer2, Pattern)) Add = False;

              if (Add)                  /* add file to list */
              {
                Run = AddFileElement(TempBuffer2, FileData.st_mtime);
                Flag = Run;             /* update flag */
              }
            }
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
  else                          /* error */
  {
    Log(L_WARN, "Can't access directory (%s)!", Path);
  }

  return Flag;  
}



/* ************************************************************************
 *   commands
 * ************************************************************************ */


/*
 *  write index
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteIndex(char *Filepath)
{
  _Bool             Flag = False;            /* return value */
  _Bool             Run = True;              /* control flag */
  IndexData_Type    *IndexData = NULL;       /* data list */
  IndexData_Type    *LastData;               /* last element in data list */
  IndexLookup_Type  *IndexLookup = NULL;     /* lookup list */
  IndexAlias_Type   *IndexAlias = NULL;      /* alias list */
  FILE              *DataFile = NULL;        /* index data file */
  FILE              *LookupFile = NULL;      /* index lookup file */
  FILE              *AliasFile = NULL;       /* index alias file */
  FILE              *OffsetFile = NULL;      /* index offset file */
  _Bool             DataLock = False;        /* data file locked */
  _Bool             LookupLock = False;      /* lookup file locked */
  _Bool             AliasLock = False;       /* alias file locked */
  _Bool             OffsetLock = False;      /* offset file locked */
  char              FirstChar = 0;           /* first char of filename */
  unsigned int      Counter = 0;             /* filename/line counter */
  off_t             Offset;                  /* file offset */
  char              *Help;
  _Bool             OffsetFlag = False;      /* flag for binary search mode */

  /* sanity check */
  if (Filepath == NULL) return False;

  /* update flag for binary search (create offset file) */
  if (Env->CfgSwitches & SW_BINARY_SEARCH) OffsetFlag = True;


  /*
   *  sort data
   */

  if (Run)
  {
    Run = False;                               /* reset flag */
    LastData = NULL;                           /* reset pointer */

    IndexData = MergeSort(Env->DataList, &LastData);   /* sort */
    Env->DataList = IndexData;                 /* update list start */
    Env->LastData = LastData;                  /* update list end */

    if (IndexData != NULL) Run = True;         /* ok for next part */
  }


  /*
   *  open index files
   */

  if (Run)
  {
    Run = False;                                /* reset flag */

    /* data file */
    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
      "%s."SUFFIX_DATA, Filepath);
    DataFile = fopen(TempBuffer, "w");       /* truncate & write mode */

    /* lookup file (binary search) */
    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
      "%s."SUFFIX_LOOKUP, Filepath);
    LookupFile = fopen(TempBuffer, "w");     /* truncate & write mode */

    /* alias file */
    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
      "%s."SUFFIX_ALIAS, Filepath);
    AliasFile = fopen(TempBuffer, "w");      /* truncate & write mode */

    /* offset file */
    snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
      "%s."SUFFIX_OFFSET, Filepath);
    OffsetFile = fopen(TempBuffer, "w");     /* truncate & write mode */

    /* check */
    if (DataFile && LookupFile && AliasFile && OffsetFile)
    {
      Run = True;                  /* ok for next part */
    }
    else
    {
      Log(L_WARN, "Can't open index files (%s)!", Filepath);
    }
  }


  /*
   *  lock index files
   */

  if (Run)
  {
    Run = False;                             /* reset flag */

    /* lock files */
    DataLock = LockFile(DataFile, NULL);
    LookupLock = LockFile(LookupFile, NULL);
    AliasLock = LockFile(AliasFile, NULL);
    OffsetLock = LockFile(OffsetFile, NULL);

    /* check */
    if (DataLock && LookupLock && AliasLock && OffsetLock)
    {
      Run = True;                  /* ok for next part */
    }
    else
    {
      Log(L_WARN, "Can't lock index files (%s)!", Filepath);
    }
  }


  /*
   *  write data file and optional offset file
   *  also build lookup list
   *
   *  format: <name>0x1F<filepath>[0x1F<password>]LF
   *  We use the ascii unit separator 31 (octal 037) as field separator.
   *  <filepath>: <path>/[<filename>] or %<alias offset>%/[filename]
   *  - %<alias offset>% for automatic path aliasing
   *  - <filename> can be omitted if same as <name>
   */

  if (Run)
  {
    while (IndexData)                   /* follow list */
    {
      Counter++;              /* another file */

      /* catch change of first letter */
      if (IndexData->Name[0] != FirstChar)
      {
        /* update last line counter for old character */
        IndexLookup = Env->LastLookup;     /* get current pointer */
        if (IndexLookup) IndexLookup->Stop = Counter - 1;

        FirstChar = IndexData->Name[0];    /* save new character */

        /* add lookup element for new character */
        Offset = ftello(DataFile);      /* get current offset of the data file */
        AddLookupElement(FirstChar, Offset, Counter, 0);
      }

      /*
       *  write offset file (BinarySearch)
       *  format: <binary offset>
       */

      if (OffsetFlag)
      {
        Offset = ftello(DataFile);      /* get current offset of the data file */

        /* write offset of the data file to the offset file */
        if (fwrite(&Offset, sizeof(off_t), 1, OffsetFile) != 1)
        {
          Run = False;               /* signal problem */
          IndexData = NULL;          /* end loop */
          Log(L_WARN, "Write error for index offset file (%s)!", Filepath);
        }
      }

      /*
       *  write data file
       */

      /* build data buffer */
      if (IndexData->PW)           /* password required */
      {
        snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
          "%s\037%s\037%s\n", IndexData->Name, IndexData->Filepath, IndexData->PW);
      }
      else                         /* no password */
      {
        snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
          "%s\037%s\n", IndexData->Name, IndexData->Filepath);
      }

      IndexData = IndexData->Next;      /* go to next element */

      /* and write to data file */
      if (fputs(OutBuffer, DataFile) < 0)    /* got an error */
      {
        Run = False;               /* signal problem */
        IndexData = NULL;          /* end loop */
        Log(L_WARN, "Write error for index data file (%s)!", Filepath);
      }

      /* check for counter overflow */
      if (Counter >= 1000000)      /* keep it reasonable, not UINT_MAX */
      {
        Run = False;               /* signal problem */
        IndexData = NULL;          /* end loop */
        Log(L_WARN, "Fileindex overrun (%s)!", Filepath);
      } 
    }

    /* update stop line# of last char */
    if (Run)
    {
      IndexLookup = Env->LastLookup;     /* get current pointer */
      if (IndexLookup) IndexLookup->Stop = Counter;
    } 
  }


  /*
   *  write lookup file
   *
   *  format: <char> <file offset> <start line#> <stop line#>LF
   */

  if (Run)
  {
    IndexLookup = Env->LookupList;

    while (IndexLookup)            /* follow list */
    {
      /* build data buffer */
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%c %ld %u %u\n", IndexLookup->Letter, IndexLookup->Offset,
        IndexLookup->Start, IndexLookup->Stop);
      
      IndexLookup = IndexLookup->Next;     /* go to next element */

      if (fputs(OutBuffer, LookupFile) < 0)    /* got an error */
      {
        Run = False;
        IndexLookup = NULL;
        Log(L_WARN, "Write error for index lookup file (%s)!", Filepath);
      }
    }
  }


  /*
   *  write alias file
   *
   *  format: <path>LF
   *  To access the paths later on the offsets to the paths are stored as aliases
   *  in the data file.
   */

  if (Run)
  {
    IndexAlias = Env->AliasList;

    while (IndexAlias)             /* follow list */
    {
      /* build data buffer */
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1, "%s\n", IndexAlias->Path);
      
      IndexAlias = IndexAlias->Next;         /* go to next element */

      if (fputs(OutBuffer, AliasFile) < 0)   /* got an error */
      {
        Run = False;
        IndexAlias = NULL;
        Log(L_WARN, "Write error for index alias file (%s)!", Filepath);
      }
    }
  }


  /*
   *  check & log
   */

  if (Run)
  {
    Flag = True;             /* signal success */

    /* log statistics */
    Help = GetFilename(Filepath);
    if (Help) Log(L_INFO, "Processed %ld files for index \"%s\".", Env->Files, Help);
    Env->Files = 0;          /* reset counter */
  }


  /*
   *  clean up
   */

  /* unlock and close files */
  if (OffsetLock) UnlockFile(OffsetFile);
  if (AliasLock) UnlockFile(AliasFile);
  if (LookupLock) UnlockFile(LookupFile);
  if (DataLock) UnlockFile(DataFile);
  if (OffsetFile) fclose(OffsetFile);
  if (AliasFile) fclose(AliasFile);
  if (LookupFile) fclose(LookupFile);
  if (DataFile) fclose(DataFile);

  /* free file index lists */
  FreeDataList(Env->DataList);            /* free index data list */
  Env->DataList = NULL;
  Env->LastData = NULL;
  FreeLookupList(Env->LookupList);        /* free index lookup list */
  Env->LookupList = NULL;
  Env->LastLookup = NULL;
  FreeAliasList(Env->AliasList);          /* free index alias list */
  Env->AliasList = NULL;
  Env->LastAlias = NULL;

  return Flag;
}



/* ************************************************************************
 *   commands
 * ************************************************************************ */


/*
 *  include file
 *  Syntax: Include [Config <filepath>]
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

  if ((Run == False) || (Keyword > 0))  /* problem occured */
  {
    if (Error == False)                 /* syntax error */
    {
      LogCfgError();
    }
  }
  else                                  /* went fine */
  {
    Flag = True;       /* signal success */
  }

  return Flag;
}



/*
 *  write file index
 *  Syntax: Index <filepath>
 *  (.data, .lookup, .alias and .offset will be appended)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Index(Token_Type *TokenList)
{
  _Bool             Flag = False;            /* return value */
  _Bool             Run = True;              /* control flag */
  unsigned short    Get = 0;                 /* mode control */
  Token_Type        *FilepathToken = NULL;   /* filepath token */

  /* sanity check */
  if (TokenList == NULL) return Flag;


  /*
   *  parse tokens
   */

  while (Run && TokenList && TokenList->String)
  {
    if (Get == 1)        /* get value: filepath */
    {
      FilepathToken = TokenList;
      Get = 0;                     /* reset */
    }
    else if (strcasecmp(TokenList->String, "Index") == 0)   /* filepath */
    {
      Get = 1;
    }
    else                 /* unknown */
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
   *  write index
   */

  if (Run)
  {
    Flag = WriteIndex(FilepathToken->String);
  }

  return Flag;
}



/*
 *  add directory to files (shared with mfreq-list)
 *  Syntax: SharedFileArea Path <path> [PW <password>] [Depth <depth>] [AutoMagic]
 *            [Name <name>] [Info <description>]
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
  char              *Path = NULL;
  char              *Password = NULL;
  int               Depth = 0;          /* depth of recursion */
  _Bool             AutoMagic = False;
  static char       *Keywords[8] =
    {"SharedFileArea", "Path", "AutoMagic", "Depth", "PW",
     "Name", "Info", NULL};

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
        case 2:     /* path */
          Path = TokenList->String;
          break;

        case 4:     /* depth */
          Depth = Str2Long(TokenList->String);
          if ((Depth < 0) || (Depth > 10))    /* invalid value */
          {
            Log(L_WARN, "Invalid recursion depth!");
            Run = False;           /* end loop & signal problem */
          }
          break;

        case 5:     /* password */
          Password = TokenList->String;

        /* Other keywords with values belong to mfreq-list,
           so we simply skip them. */
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
          break;

        case 1:               /* command itself */
          /* simply skip it */
          Keyword = 0;        /* reset keyword */
          break;

        case 3:               /* automagic switch */
          AutoMagic = True;
          Keyword = 0;        /* reset keyword */
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Path == NULL))
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
    Flag = ProcessPath(Path, Password, Depth, AutoMagic);
  }

  return Flag;
}



/*
 *  add directory to files
 *  Syntax: FileArea <path> [PW <password>] [Depth <depth>] [AutoMagic]
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
  char              *Path = NULL;
  char              *Password = NULL;
  int               Depth = 0;          /* depth of recursion */
  _Bool             AutoMagic = False;
  static char       *Keywords[5] =
    {"FileArea", "AutoMagic", "Depth", "PW", NULL};

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
        case 1:     /* path */
          Path = TokenList->String;
          break;

        case 3:     /* depth */
          Depth = Str2Long(TokenList->String);
          if ((Depth < 0) || (Depth > 10))    /* invalid value */
          {
            Log(L_WARN, "Invalid recursion depth!");
            Run = False;           /* end loop & signal problem */
          }
          break;

        case 4:     /* password */
          Password = TokenList->String;
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
          break;

        case 2:               /* automagic switch */
          AutoMagic = True;
          Keyword = 0;        /* reset keyword */
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Path == NULL))
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
    Flag = ProcessPath(Path, Password, Depth, AutoMagic);
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
    if (Get == 1)                  /* get value: path */
    {
      NameToken = TokenList;
      Get = 0;                       /* reset */      
    }
    else if (strcasecmp(TokenList->String, "Exclude") == 0)   /* path */
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
 *  add directory with magics to files
 *  Syntax: MagicPath <path>
 *  (magic = filename / filepath = content of file)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_MagicPath(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Get = 0;            /* mode control */
  Token_Type        *PathToken = NULL;

  /* sanity check */
  if (TokenList == NULL) return Flag;


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
    else if (strcasecmp(TokenList->String, "MagicPath") == 0)   /* path */
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

  if ((Run == False) || (Get >0) || (PathToken == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    /* open path and process magic files */
    Flag = ProcessMagicPath(PathToken->String);
  }

  return Flag;
}



/*
 *  add smart magic to files
 *  Syntax: SmartMagic <name> Path <path> [Pattern <pattern>] [Latest]
 *          [PW <password>]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_SmartMagic(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  char              *Name = NULL;       /* magic name */
  char              *Path = NULL;       /* path name */
  char              *Pattern = NULL;    /* filename pattern */
  char              *Password = NULL;   /* password for magic */
  char              *LocalPath = NULL;  /* absolute path */
  _Bool             Latest = False;     /* switch for latest file */
  File_Type         *File;
  File_Type         *Next;
  time_t            Time;               /* time (seconds)*/
  static char       *Keywords[6] =
    {"SmartMagic", "Path", "Pattern", "Latest", "PW", NULL};

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
        case 1:     /* name */
          Name = TokenList->String;
          break;
      
        case 2:     /* path */
          Path = TokenList->String;
          break;

        case 3:     /* pattern */
          Pattern = TokenList->String;
          break;

        case 5:     /* password */
          Password = TokenList->String;
      }

      Keyword = 0;            /* reset */
    }
    else                      /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      switch (Keyword)        /* keywords without data */
      {
        case 0:          /* unknown keyword */
          Run = False;
          break;

        case 4:          /* latest */
          Latest = True;
          Keyword = 0;        /* reset keyword */
      }
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Name == NULL) || (Path == NULL) ||
      ((Pattern == NULL) && (Latest == False)))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process directory
   */

  if (Run)
  {
    Run = False;                   /* reset flag */

    /* build absolute path if necessary */
    if (Path[0] != '/')      /* relative path */
    {
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s/%s", Env->CWD, Path);
      LocalPath = CopyString(TempBuffer);
      Path = LocalPath;
    }

    /* scan directory (considering excludes and pattern) */
    if (ScanPath(Path, Pattern))
    {
      if (Env->FileList)           /* got some files */
      {
        Run = True;                /* ok to proceed */
      }
      else                         /* no matching files */
      {
        Flag = True;               /* job done */
      }
    }
  }


  /*
   *  process filedate
   */

  if (Run && Latest)               /* select latest file */
  {
    Time = 0;                      /* start with 0 */

    /* find latest time */
    File = Env->FileList;          /* start of list */

    while (File)                   /* follow list */
    {
      if (File->Time > Time)       /* newer */
      {
        Time = File->Time;              /* update time */
      }

      File = File->Next;           /* next element */
    }

    /* mark all older files */
    File = Env->FileList;          /* start of list */

    while (File)                   /* follow list */
    {
      if (File->Time < Time)       /* older */
      {
        File->Flags |= FILE_SKIP;       /* mark file */
      }

      File = File->Next;           /* next element */
    }
  }


  /*
   *  process matching files
   */

  if (Run)
  {
    File = Env->FileList;          /* start of list */

    while (File)                   /* follow list */
    {
      Next = File->Next;           /* get next element */

      /* add match to global index */
      if (!(File->Flags & FILE_SKIP))
      {
        /* create full path */
        snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
          "%s/%s", Path, File->Name);
        Flag = AddDataElement(Name, TempBuffer, Password);
        if (Flag) Env->Files++;       /* increase file counter */
        else Next = NULL;             /* end loop */
      }

      File = Next;                 /* next element */
    }
  }


  /*
   *  clean up
   */

  if (Env->FileList)               /* global file list */
  {
    FreeFileList(Env->FileList);   /* free list */
    Env->FileList = NULL;          /* reset global variables */
    Env->LastFile = NULL;
  }

  if (LocalPath) free(LocalPath);  /* local path */

  return Flag;
}



/*
 *  add magic to files
 *  Syntax: Magic <name> File <filepath> [PW <password>]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_Magic(Token_Type *TokenList)
{
  _Bool             Flag = False;       /* return value */
  _Bool             Run = True;         /* control flag */
  unsigned short    Keyword = 0;        /* keyword ID */
  char              *Name = NULL;       /* magic name */
  char              *Filepath = NULL;   /* filepath */
  char              *Password = NULL;   /* password for magic */
  static char       *Keywords[4] =
    {"Magic", "File", "PW", NULL};

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
        case 1:     /* name */
          Name = TokenList->String;
          break;
      
        case 2:     /* filepath */
          Filepath = TokenList->String;
          break;

        case 3:     /* password */
          Password = TokenList->String;
      }

      Keyword = 0;            /* reset */
    }
    else                      /* get keyword */
    {
      Keyword = GetKeyword(Keywords, TokenList->String);

      if (Keyword == 0) Run = False;    /* unknown keyword */
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if ((Run == False) || (Keyword > 0) || (Name == NULL) || (Filepath == NULL))
  {
    Run = False;
    LogCfgError();
  }


  /*
   *  process
   */

  if (Run)
  {
    /* check if filepath is a regular file */
    if (CheckFile(Filepath))
    {
      /* add magic to global file index */
      Flag = AddDataElement(Name, Filepath, Password);
      if (Flag) Env->Files++;       /* increase file counter */
    }
  }

  return Flag;
}



/*
 *  reset stuff
 *  Syntax: Reset [SetMode]
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
    {"Reset", "SetMode", "Excludes", NULL};

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

      case 2:       /* SetMode */
        Env->CfgSwitches = SW_NONE;     /* set switches to default */
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
 *  set mode
 *  Syntax: SetMode [PathAliases] [BinarySearch]
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Cmd_SetMode(Token_Type *TokenList)
{
  _Bool                  Flag = False;       /* return value */
  _Bool                  Run = True;         /* control flag */
  unsigned short         Keyword = 0;        /* keyword ID */
  static char            *Keywords[5] =
    {"SetMode", "PathAliases", "BinarySearch", "AnyCase", NULL};

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

      case 2:       /* path aliases */
        Env->CfgSwitches |= SW_PATH_ALIASES;
        break;

      case 3:       /* binary search */
        Env->CfgSwitches |= SW_BINARY_SEARCH;
        break;

      case 4:       /* case-insensitive search */
        Env->CfgSwitches |= SW_ANY_CASE;
        break;
    }

    TokenList = TokenList->Next;     /* goto to next token */
  }


  /*
   *  check parser results
   */

  if (Run == False)
  {
    LogCfgError();            /* syntax error */
  }
  else
  {
    Flag = True;              /* signal success */
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
    Run = False;
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
    else if (strcasecmp(TokenList->String, "LogFile") == 0)  /* filepath */
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
  static char            *Keywords[12] =
    {"FileArea", "SharedFileArea", "Magic", "SmartMagic", "MagicPath",
     "Exclude", "Include", "SetMode", "Reset", "LogFile",
     "Index", NULL};

  /* sanity check */
  if (TokenList == NULL) return Flag;

  /* find command and call corresponding function */
  if (TokenList->String)
  {
    Keyword = GetKeyword(Keywords, TokenList->String);

    switch (Keyword)           /* command keywords */
    {
      case 0:       /* unknown command */
        Log(L_WARN, "Unknown command in cfg file (%s), line %d (%s)!",
          Env->CfgInUse, Env->CfgLinenumber, TokenList->String);        
        break;

      case 1:       /* filearea */
        Flag = Cmd_FileArea(TokenList);
        break;

      case 2:       /* shared filearea */
        Flag = Cmd_SharedFileArea(TokenList);
        break;

      case 3:       /* magic */
        Flag = Cmd_Magic(TokenList);
        break;

      case 4:       /* smart magic */
        Flag = Cmd_SmartMagic(TokenList);
        break;

      case 5:       /* magicpath */
        Flag = Cmd_MagicPath(TokenList);
        break;

      case 6:       /* exclude */
        Flag = Cmd_Exclude(TokenList);
        break;

      case 7:       /* include */
        Flag = Cmd_Include(TokenList);
        break;

      case 8:       /* set mode */
        Flag = Cmd_SetMode(TokenList);
        break;

      case 9:       /* reset */
        Flag = Cmd_Reset(TokenList);
        break;

      case 10:       /* logfile */
        Flag = Cmd_LogFile(TokenList);
        break;

      case 11:      /* index */
        Flag = Cmd_Index(TokenList);
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

  File = fopen(Filepath, "r");     /* open file in read mode */
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
   *  get command line options
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
  _Bool                 Flag = False;

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
    /* environment: filepaths */
    Env->CfgFilepath = NULL;
    Env->LogFilepath = NULL;

    /* environment: process */
    Env->CWD = NULL;
    Env->PID = 0;

    /* environment: file streams */
    Env->Log = NULL;

    /* environment: program control */
    Env->Run = True;
    Env->ConfigDepth = 0;
    Env->CfgInUse = NULL;
    Env->CfgLinenumber = 0;

    /* environment: common configuration */
    Env->CfgSwitches = SW_NONE;

    /* environment: file index */
    Env->DataList = NULL;
    Env->LastData = NULL;
    Env->LookupList = NULL;
    Env->LastLookup = NULL;
    Env->AliasList = NULL;
    Env->LastAlias = NULL;
    Env->ExcludeList = NULL;
    Env->LastExclude = NULL;
    Env->FileList = NULL;
    Env->LastFile = NULL;

    /* environment: frequest runtime stuff */
    Env->Files = 0;         /* will misuse that for statistics */
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
    /* free lists */
    if (Env->FileList) FreeFileList(Env->FileList);
    if (Env->DataList) FreeDataList(Env->DataList);
    if (Env->LookupList) FreeLookupList(Env->LookupList);
    if (Env->AliasList) FreeAliasList(Env->AliasList);
    if (Env->ExcludeList) FreeExcludeList(Env->ExcludeList);

    /* free strings */
    if (Env->CfgFilepath) free(Env->CfgFilepath);
    if (Env->LogFilepath) free(Env->LogFilepath);
    if (Env->CWD) free(Env->CWD);

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

  if (GetAllocations())                   /* allocate global variables */
  {
    if (ParseCommandLine(argc, argv))     /* parse local cmd line options */
    {
      Flag = True;                        /* ok for next part */
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
    Flag = ReadConfig(Env->CfgFilepath);     /* read and parse config */
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
      UnlockFile(Env->Log);        /* unlock file */
      fclose(Env->Log);            /* close file */
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

#undef MFREQ_INDEX_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
