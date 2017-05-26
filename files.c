/* ************************************************************************
 *
 *   functions for file specific data management
 *
 *   (c) 2015 by Markus Reschke
 *
 * ************************************************************************ */

/*
 *  local constants
 */

#define FILES_C


/*
 *  include header files
 */


/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */



/* ************************************************************************
 *   file data (linked list)
 * ************************************************************************ */


/*
 *  free list of file elements
 */

void FreeFileList(File_Type *List)
{
  File_Type            *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Name) free(List->Name);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new file element to list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddFileElement(char *Name, time_t Time)
{
  _Bool               Flag = False;        /* return value */
  File_Type           *Element;            /* new element */

  /* sanity check */
  if (Name == NULL) return Flag;  

  Element = malloc(sizeof(File_Type));     /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Flags = FILE_NONE;
    Element->Next = NULL;

    /* copy data */
    Element->Name = CopyString(Name);
    Element->Time = Time;

    /* add new element to list */
    if (Env->LastFile) Env->LastFile->Next = Element;  /* just link */
    else Env->FileList = Element;                      /* start list */
    Env->LastFile = Element;                           /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   clean-up of local constants
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef FILES_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
