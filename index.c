/* ************************************************************************
 *
 *   functions for file index specific data management
 *
 *   (c) 1994-2015 by Markus Reschke
 *
 * ************************************************************************ */

/*
 *  local constants
 */

#define INDEX_C


/*
 *  include header files
 */


/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */

/* strings */
#include <ctype.h>



/* ************************************************************************
 *   file index data (linked list)
 * ************************************************************************ */


/*
 *  free list of data elements
 */

void FreeDataList(IndexData_Type *List)
{
  IndexData_Type          *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Name) free(List->Name);
    if (List->Filepath) free(List->Filepath);
    if (List->PW) free(List->PW);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new data element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddDataElement(char *Name, char *Filepath, char *PW)
{
  _Bool               Flag = False;        /* return value */
  IndexData_Type      *Element;            /* new element */
  char                *Help;               /* string pointer */

  /* sanity check */
  if ((Name == NULL) || (Filepath == NULL)) return Flag;  

  Element = malloc(sizeof(IndexData_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* convert name to upper case for case-insensitive search */
    if (Env->CfgSwitches & SW_ANY_CASE)
    {
      /* convert name to upper case */
      Help = Name;
      while (Help[0] != 0)
      {
        Help[0] = toupper(Help[0]);
        Help++;                       /* next char */
      }
    }

    /* set defaults */
    Element->Next = NULL;
    Element->PW = NULL;

    /* copy data */
    Element->Name = CopyString(Name);
    Element->Filepath = CopyString(Filepath);
    if (PW) Element->PW = CopyString(PW);

    /* add new element to list */
    if (Env->LastData) Env->LastData->Next = Element;  /* just link */
    else Env->DataList = Element;                      /* start list */
    Env->LastData = Element;                           /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   file index lookup (linked list)
 * ************************************************************************ */


/*
 *  free list of lookup elements
 */

void FreeLookupList(IndexLookup_Type *List)
{
  IndexLookup_Type        *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer */

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new lookup element to global list
 */

_Bool AddLookupElement(char Letter, off_t Offset, unsigned int Start, unsigned int Stop)
{
  _Bool                    Flag = False;        /* return value */
  IndexLookup_Type         *Element = NULL;     /* new element */

  /* sanity check */
  if (Letter == 0) return Flag;

  /* allocate memory */
  Element = malloc(sizeof(IndexLookup_Type));

  if (Element)
  {
    /* set defaults */
    Element->Next = NULL;

    /* copy data */
    Element->Letter = Letter;
    Element->Offset = Offset;
    Element->Start = Start;
    Element->Stop = Stop;

    /* add new element to list */
    if (Env->LastLookup) Env->LastLookup->Next = Element;   /* just link */
    else Env->LookupList = Element;                         /* start list */
    Env->LastLookup = Element;                              /* save new list end */

    Flag = True;            /* signal success */
  }
  else
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Element;
}


/* ************************************************************************
 *   file index path aliases (linked list)
 * ************************************************************************ */


/*
 *  free list of alias elements
 */

void FreeAliasList(IndexAlias_Type *List)
{
  IndexAlias_Type             *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer */

    /* free data */
    if (List->Path) free(List->Path);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new alias element to global list
 */

_Bool AddAliasElement(unsigned int Number, char *Path)
{
  _Bool                    Flag = False;        /* return value */
  IndexAlias_Type          *Element = NULL;     /* new element */

  /* sanity check */
  if (Path == NULL) return Flag;

  /* allocate memory */
  Element = malloc(sizeof(IndexAlias_Type));

  if (Element)
  {
    /* set defaults */
    Element->Offset = -1;
    Element->Next = NULL;

    /* copy data */
    Element->Number = Number;
    Element->Path = CopyString(Path);

    /* update file offset */
    if (Env->LastAlias)            /* other elements in list already */
    {
      /* take last offset, add length of last path and 1 for the linefeed */
      Element->Offset = Env->LastAlias->Offset;
      Element->Offset += strlen(Env->LastAlias->Path);
      Element->Offset++;
    }
    else                           /* first element in list */
    {
      Element->Offset = 0;         /* file starts at 0 :-) */
    }

    /* add new element to list */
    if (Env->LastAlias) Env->LastAlias->Next = Element;     /* just link */
    else Env->AliasList = Element;                          /* start list */
    Env->LastAlias = Element;                               /* save new list end */

    Flag = True;            /* signal success */
  }
  else
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Element;
}



/* ************************************************************************
 *   file excludes (linked list)
 * ************************************************************************ */


/*
 *  free list of exclude elements
 */

void FreeExcludeList(Exclude_Type *List)
{
  Exclude_Type            *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer */

    /* free data */
    if (List->Name) free(List->Name);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new exclude element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddExcludeElement(char *Name)
{
  _Bool               Flag = False;        /* return value */
  Exclude_Type        *Element;            /* new element */

  /* sanity check */
  if (Name == NULL) return Flag;  

  Element = malloc(sizeof(Exclude_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Next = NULL;

    /* copy data */
    Element->Name = CopyString(Name);

    /* add new element to list */
    if (Env->LastExclude) Env->LastExclude->Next = Element;  /* just link */
    else Env->ExcludeList = Element;                         /* start list */
    Env->LastExclude = Element;                              /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/*
 *  check if filename matches any exlude pattern
 *
 *  returns:
 *  - 1 on match
 *  - 0 on error or mismatch
 */

_Bool MatchExcludeList(char *Name)
{
  _Bool                  Flag = False;        /* return value */
  Exclude_Type          *List;                /* linked list */  

  /* sanity check */
  if (Name == NULL) return Flag;

  List = Env->ExcludeList;      /* start of list */

  while (List)                  /* run through list */
  {
    if (MatchPattern(Name, List->Name))     /* got match */
    {
      Flag = True;        /* signal match */
      List = NULL;        /* end loop */
    }
    else                                    /* no match */
    {
      List = List->Next;   /* go to next element */
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

#undef INDEX_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
