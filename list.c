/* ************************************************************************
 *
 *   functions for filelist specific data management
 *
 *   (c) 1994-2014 by Markus Reschke
 *
 * ************************************************************************ */

/*
 *  local constants
 */

#define LIST_C


/*
 *  include header files
 */


/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */



/* ************************************************************************
 *   file information data (linked list)
 * ************************************************************************ */


/*
 *  free list of info elements
 *
 *  requires:
 *  - pointer to fileinfo linked list
 */

void FreeInfoList(Info_Type *List)
{
  Info_Type            *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Name) free(List->Name);
    if (List->Infos) FreeTokenlist(List->Infos);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new info element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddInfoElement(char *Name, off_t Size, time_t Time)
{
  _Bool               Flag = False;        /* return value */
  Info_Type           *Element;            /* new element */

  /* sanity check */
  if (Name == NULL) return Flag;  

  Element = malloc(sizeof(Info_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Counter = 0;
    Element->Status = STAT_NONE;
    Element->Infos = NULL;
    Element->LastInfo = NULL;
    Element->Next = NULL;

    /* copy data */
    Element->Name = CopyString(Name);
    Element->Size = Size;
    Element->Time = Time;

    /* add new element to list */
    if (Env->LastInfo) Env->LastInfo->Next = Element;  /* just link */
    else Env->InfoList = Element;                      /* start list */
    Env->LastInfo = Element;                           /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/*
 *  search for info element matching a specific name
 *
 *  requires:
 *  - pointer to fileinfo linked list
 *  - pointer to filename string
 *
 *  returns:
 *  - pointer to element on match
 *  - NULL on error or mismatch
 */

Info_Type *SearchInfoList(Info_Type *List, char *Name)
{
  Info_Type           *Element = NULL;       /* return value */
  _Bool               AnyCase = False;       /* case sensitivity flag */

  /* sanity check */
  if (Name == NULL) return Element;

  /* check for case-insensitive search */
  if (Env->CfgSwitches & SW_ANY_CASE) AnyCase = True; 

  while (List)                /* follow list */
  {
    if (List->Name)           /* sanity check */
    {
      if (AnyCase == False)   /* case sensitive */
      {
        /* check for match */
        if (strcmp(Name, List->Name) == 0) Element = List;
      }
      else                    /* case insensitive */
      {
        /* check for match */
        if (strcasecmp(Name, List->Name) == 0) Element = List;
      }
    }

    if (Element != NULL)      /* found match */
    {
      List = NULL;                 /* end loop */
    }
    else                      /* no match */
    {
      List = List->Next;           /* next element */
    }
  }

  return Element;
}



/*
 *  create and add new description line to Info element
 *
 *  requires:
 *  - pointer to fileinfo element
 *  - pointer to description string
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddDesc2Info(Info_Type *Info, char *Data)
{
  _Bool               Flag = False;        /* return value */
  Token_Type          *Token = NULL;

  /* sanity checks */
  if ((Info == NULL) || (Data == NULL)) return Flag;

  Token = malloc(sizeof(Token_Type));
  if (Token)
  {
    /* setup element */
    Token->Next = NULL;
    Token->String = CopyString(Data);

    /* and add it to list */
    if (Info->LastInfo) Info->LastInfo->Next = Token;
    else Info->Infos = Token;
    Info->LastInfo = Token;

    Flag = True;    /* signal success */
  }
  else
  {
    Log(L_ERR, "Could not allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   file description fields
 * ************************************************************************ */


/*
 *  free list of field elements
 */

void FreeFieldList(Field_Type *List)
{
  Field_Type            *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and insert new field element to a global list
 *  based on line and position (-> sorted list)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool InsertField(Fields, Type, Line, Pos, Width, Align, Format)
  Field_Type        **Fields;
  unsigned short    Type;
  unsigned short    Line;
  unsigned short    Pos;
  unsigned short    Width;
  unsigned short    Align;
  unsigned short    Format;
{
  _Bool             Flag = False;       /* return value */
  Field_Type        *Field;             /* new element */
  Field_Type        *List;
  Field_Type        *Prev = NULL;
  Field_Type        *Next = NULL;
  unsigned short    Test;

  /* sanity check */
  if (Fields == NULL) return Flag;

  Field = malloc(sizeof(Field_Type));    /* allocate memory */
  if (Field)
  {
    /* set defaults */
    Field->Next = NULL;

    /* copy values */
    Field->Type = Type;
    Field->Line = Line;
    Field->Pos = Pos;
    Field->Width = Width;
    Field->Align = Align;
    Field->Format = Format;

    Flag = True;

    /* search for neighbours */
    List = *Fields;
    while (List)
    {
      if (List->Line < Field->Line)         /* line above */
      {
        Prev = List;
        List = List->Next;        
      }
      else if (List->Line == Field->Line)   /* same line */
      {
        /* take care about positions */
        if (List->Pos < Field->Pos)    /* left neighbour */
        {
          Prev = List;
          List = List->Next;
        }
        else                           /* right neighbour */
        {
          Next = List;
          List = NULL;
        }
      }
      else                                  /* line below */
      {
        Next = List;
        List = NULL;
      }
    }

    /* link new element */
    if (Prev)                /* got left neighbour */
    {
      /* insert after element in front of */
      Field->Next = Prev->Next;
      Prev->Next = Field;
    }
    else                     /* no left neighbour */
    {
      /* insert at list start */
      Field->Next = *Fields;
      *Fields = Field;
    }

    /* check offset to left field in same line */
    if (Prev && (Prev->Line == Field->Line))
    {
      Test = Prev->Pos + Prev->Width;
      if (Field->Pos <= Test) Flag = False;
    }

    /* check offset to right field in same line */
    if (Next && (Next->Line == Field->Line))
    {
      Test = Field->Pos + Field->Width;
      if (Next->Pos <= Test) Flag = False;
    }

    if (!Flag)
    {
      Log(L_WARN, "Overlapping data fields!");
    }
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   file description fields
 * ************************************************************************ */


/*
 *  convert date sub-string into struct tm
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Strings2DateTime(char *Year, char *Month, char *Day, struct tm *DateTime)
{
  _Bool               Flag = False;        /* return value */
  int                 Value;               /* temp value */
  int                 Counter = 0;
  char                *Test = NULL;        /* test pointer */

  /* sanity checks */
  if ((DateTime == NULL) || (Year == NULL) ||
      (Month == NULL) || (Day == NULL)) return Flag;

  /* set default values */
  DateTime->tm_sec = 0;
  DateTime->tm_min = 0;
  DateTime->tm_hour = 0;
  DateTime->tm_mday = 0;
  DateTime->tm_mon = 0;
  DateTime->tm_year = 0;
  DateTime->tm_wday = 0;
  DateTime->tm_yday = 0;
  DateTime->tm_isdst = 0;

  /* year */
  Value = strtol(Year, &Test, 10);        /* convert */
  if ((Test != NULL) && (Test[0] == 0))   /* got valid value */
  {
    if ((Value >= 0) && (Value <= 99))    /* 2 digits */
    {
      /* todo: select century based on current year (Env->DateTime) */
      if (Value < 70) Value += 100;       /* before or past 2000? */
      DateTime->tm_year = Value;          /* years since 1900 */
      Counter++;
    }
    else if (Value >= 1000)               /* 4 digits or more */
    {
      DateTime->tm_year = Value - 1900;   /* years since 1900 */
      Counter++;
    }
  }

  /* month */
  Value = strtol(Month, &Test, 10);       /* convert */
  if ((Test != NULL) && (Test[0] == 0))   /* got valid value */
  {
    if ((Value >= 1) && (Value <= 12))    /* check range */
    {
      DateTime->tm_mon = Value - 1;       /* 0 to 11 */
      Counter++;
    }
  }

  /* day */
  Value = strtol(Day, &Test, 10);         /* convert */
  if ((Test != NULL) && (Test[0] == 0))   /* got valid value */
  {
    if ((Value >= 1) && (Value <= 31))    /* check range */
    {
      DateTime->tm_mday = Value;
      Counter++;
    }
  }

  if (Counter == 3) Flag = True;          /* got all three parts */

  return Flag;
}



/* ************************************************************************
 *   output support functions
 * ************************************************************************ */


/*
 *  fill string with a given character
 */

void FillString(char *String, char Char, unsigned short Number, unsigned short Max)
{
  /* sanity check */
  if (String == NULL) return;

  if (Number > Max) Number = Max;    /* limit */

  while (Number > 0)
  {
    String[0] = Char;
    String++;
    Number--;
  }

  String[0] = 0;            /* end string */
}



/*
 *  limit digits of a number
 *
 *  returns:
 *  - limited number
 */

long long LimitNumber(long long Number, unsigned short Digits)
{
  long long         Value = 1;     /* return value */

  /* calculate upper limit to match number of digits */
  while (Digits > 0)
  {
    Value = Value * 10;
    Digits--;
  }

  /* limit number to maximum if limit is exeeded */
  if (Number < Value) Value = Number;
  else Value--;

  return Value;
}



/*
 *  convert number of bytes into string with a specific format
 *
 *  requires:
 *  - number of bytes
 *  - maximun allowed string width
 *  - output format (FIELD_FORM_*)
 *  - pointer to string buffer
 *  - size of string buffer
 *
 *  returns:
 *  - 1 on success
 *  - 0 on any problem
 */

_Bool Bytes2StringN(Bytes, Width, Format, Buffer, Size)
  long long         Bytes;
  unsigned short    Width;
  unsigned short    Format;
  char              *Buffer;
  size_t            Size;
{
  _Bool             Flag = False;       /* return value */
  long long         Limit = 1;          /* limit for value */
  unsigned short    Counter;            /* simple counter */
  unsigned short    UnitLength = 0;     /* length of unit */
  _Bool             SI_Flag = False;    /* decimal/binary flag */
  _Bool             IEC_Flag = False;   /* IEC units */
  char              Unit[4] = {0, 0, 0, 0};  /* unit */


  /* sanity checks */
  if ((Buffer == NULL) || (Width < 4) || (Width >= Size)) return Flag;

  /* check for use of SI units */
  if (Env->CfgSwitches & SW_SI_UNITS) SI_Flag = True;

  /* check for use of IEC units */
  if (Env->CfgSwitches & SW_IEC_UNITS)
  {
    IEC_Flag = True;
    SI_Flag = False;     /* IEC units imply binary factor */
  }

  /* determine required space for prefix and unit */
  /* also set unit specifics */
  if (Format == FIELD_FORM_UNIT)        /* bytes with unit */
  {
    if (IEC_Flag)        /* KiB */
    {
      UnitLength = 3;
      Unit[1] = 'i';
    }
    else                 /* kB */
    {
      UnitLength = 2;
    }

    Unit[UnitLength - 1] = 'B';
  }
  else if (Format == FIELD_FORM_SHORT)  /* bytes with short unit */
  {
    if (IEC_Flag)        /* Ki */
    {
      UnitLength = 2;
      Unit[1] = 'i';
    }
    else                 /* k */
    {
      UnitLength = 1;
    }
  }

  /* calculate upper value limit which fits the width */
  Counter = Width;            /* maximum number of digits */
  while (Counter > 0)         /* for each digit */
  {
    Limit *= 10;              /* multiply by 10^1 */
    Counter--;
  }
  Limit--;                    /* -1 to fit width */

  /* byte limit */
  if (Format == FIELD_FORM_BYTES)  /* bytes without unit */
  {
    if (Bytes > Limit)             /* limit exceeded */
    {
      Bytes = Limit;               /* use limit */ 
    }
  }

  /* scale bytes to required width */
  if (UnitLength > 0)         /* output unit */
  {
    if (Bytes > Limit)        /* limit exceeded */
    {
      /* first adjust limit to length if unit */
      Counter = UnitLength;
      while (Counter > 0)         /* for each char */
      {
        Limit /= 10;              /* remove a digit */
        Counter--;
      }

      /* scale down bytes */
      while (Bytes > Limit)
      {
        if (SI_Flag)       /* use 1000 */
        {
          Bytes /= 1000;
        }
        else               /* use 1024 */
        {
          Bytes /= 1024;
        }

        Counter++;
      }
    }
  }

  /* prefix */
  switch (Counter)
  {
    case 1:
      if (IEC_Flag) Unit[0] = 'K';
      else Unit[0] = 'k';
      break;

    case 2:
      Unit[0] = 'M';
      break;

    case 3:
      Unit[0] = 'G';
      break;
  }

  /* build string */
  if (Counter > 0)       /* with prefix and unit */
  {  
    if (sprintf(Buffer, "%*lld%s", Width - UnitLength, Bytes, &Unit[0]) > 0) Flag = True;
  }
  else                   /* without unit, just bytes */
  {
    if (sprintf(Buffer, "%*lld", Width, Bytes) > 0) Flag = True;
  }
 
  return Flag;
}


/* ************************************************************************
 *   test functions
 * ************************************************************************ */


/*
 *  check if filename matches 8.3 DOS-style
 *
 *  returns:
 *  - 1 on match
 *  - 0 on mismatch or any problem
 */

_Bool CheckDosFilename(char* Name)
{
  _Bool             Flag = False;       /* return value */
  unsigned short    Counter = 0;
  unsigned short    Dot = 0;
  unsigned short    Check = 0;

  /* sanity check */
  if (Name == NULL) return Flag;

  /* search for dots and invalid characters */
  while (Name[0] != 0)
  {
    Counter++;           /* got another char */

    if (Name[0] == '.')          /* dot */
    {
      if (Dot == 0) Dot = Counter;   /* save position for first dot */
      else Check = 1;                /* another dot */ 
    }
    else if (Name[0] == ' ')     /* space */
    {
      Check = 2;
    }

    Name++;              /* next char */
  }


  if (Check == 0)        /* if ok so far */
  {
    if (Dot == 0)           /* no dot */
    {
      /* check length of name */
      if ((Counter > 0) && (Counter <= 8))
      {
        Flag = True;
      }
    }
    else                    /* single dot */
    {
      /* check length of name and extension */
      Check = Dot - 1;             /* length of name */
      if ((Check > 0) && (Check <= 8))
      {
        Check = Counter - Dot;     /* length of extension */
        if ((Check > 0) && (Check <= 3))
        {
          Flag = True;
        }
      }
    }
  }

  return Flag;
}



/* ************************************************************************
 *   clean-up of local constants
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef LIST_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
