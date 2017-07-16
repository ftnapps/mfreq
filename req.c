/* ************************************************************************
 *
 *   functions for request specific data management
 *
 *   (c) 1994-2017 by Markus Reschke
 *
 * ************************************************************************ */

/*
 *  local constants
 */

#define REQ_C


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
 *   indexes (linked list)
 * ************************************************************************ */


/*
 *  free list of index elements
 */

void FreeIndexList(Index_Type *List)
{
  Index_Type          *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Filepath) free(List->Filepath);
    if (List->MountingPoint) free(List->MountingPoint);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new index element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddIndexElement(char *Filepath, char *MountingPoint)
{
  _Bool               Flag = False;        /* return value */
  Index_Type          *Element;            /* new element */

  /* sanity check */
  if (Filepath == NULL) return Flag;

  Element = malloc(sizeof(Index_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Next = NULL;

    /* copy data */
    Element->Filepath = CopyString(Filepath);
    if (MountingPoint)
      Element->MountingPoint = CopyString(MountingPoint);
    else
      Element->MountingPoint = NULL;      

    /* add new element to list */
    if (Env->LastIndex) Env->LastIndex->Next = Element;      /* just link */
    else Env->IndexList = Element;                           /* start list */
    Env->LastIndex = Element;                                /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   frequest limits (linked list)
 * ************************************************************************ */


/*
 *  free list of limit elements
 */

void FreeLimitList(Limit_Type *List)
{
  Limit_Type          *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Address) free(List->Address);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new limit element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddLimitElement(char *Address, long Files, long long Bytes, int BadPWs,
  int Freqs, unsigned int Flags)
{
  _Bool               Flag = False;        /* return value */
  Limit_Type          *Element;            /* new element */

  /* sanity check */
  if (Address == NULL) return Flag;

  Element = malloc(sizeof(Limit_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Next = NULL;

    /* copy data */
    Element->Address = CopyString(Address);
    Element->Files = Files;
    Element->Bytes = Bytes;
    Element->BadPWs = BadPWs;
    Element->Freqs = Freqs;
    Element->Flags = Flags;

    /* add new element to list */
    if (Env->LastLimit) Env->LastLimit->Next = Element;      /* just link */
    else Env->LimitList = Element;                           /* start list */
    Env->LastLimit = Element;                                /* save new list end */

    Flag = True;            /* signal success */
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Flag;
}



/* ************************************************************************
 *   files to send (linked list)
 * ************************************************************************ */


/*
 *  free list of response elements
 */

void FreeResponseList(Response_Type *List)
{
  Response_Type            *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Filepath) free(List->Filepath);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create new Response element
 *
 *  returns:
 *  - pointer on success
 *  - NULL on error
 */

Response_Type *CreateResponseElement(char *Filepath)
{
  Response_Type             *Element = NULL;           /* return value */

  /* sanity check */
  if (Filepath == NULL) return Element; 

  Element = malloc(sizeof(Response_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->Size = -1;
    Element->Status = RESP_NONE;
    Element->Next = NULL;

    /* copy data */
    Element->Filepath = CopyString(Filepath);
  }
  else                  /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  } 

  return Element;
}



/*
 *  check for duplicate in list of files already found
 *  - for a specific request
 *
 *  requires:
 *  - request element to search
 *  - response element to check for
 *
 *  returns:
 *  - 0 if no duplicate is found
 *  - 1 if a duplicate is found
 */

_Bool DuplicateResponse(Request_Type *Request, Response_Type *Response)
{
  _Bool                  Flag = False;       /* return value */
  Response_Type          *File;

  /* sanity check */
  if ((Request == NULL) || (Response == NULL)) return Flag;

  File = Request->Files;           /* first element */

  while (File && !Flag)            /* loop through list */
  {
    if (File != Response)          /* not me */
    {
      /* check if both got the same filepath */
      if (File->Filepath && Response->Filepath)   /* sanity check */
      {
        if (strcmp(File->Filepath, Response->Filepath) == 0)
        {
          Flag = True;             /* signal duplicate */
        }
      }
    }

    File = File->Next;             /* next file */
  }

  return Flag;
}



/*
 *  check for duplicate in list of files already found
 *  - for all requested files
 *
 *  requires:
 *  - response element to check for
 *
 *  returns:
 *  - 0 if no duplicate is found
 *  - 1 if a duplicate is found
 */

_Bool AnyDuplicateResponse(Response_Type *Response)
{
  _Bool                  Flag = False;       /* return value */
  Request_Type           *Request = NULL;

  /* sanity check */
  if (Response == NULL) return Flag;

  if (Env)
  {
    Request = Env->RequestList;         /* first request element */
  }


  /*
   *  loop through all requested files
   *  and check for any duplicate filepath
   */

  while (Request && !Flag)    /* loop through requests */
  {
    /* check current request for duplicates */
    Flag = DuplicateResponse(Request, Response);

    Request = Request->Next;            /* next request */
  }

  return Flag;
}



/* ************************************************************************
 *   requested files (linked list)
 * ************************************************************************ */


/*
 *  free list of request elements
 */

void FreeRequestList(Request_Type *List)
{
  Request_Type           *Next;    /* pointer for next element */

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;             /* save pointer to next element */

    /* free data */
    if (List->Name) free(List->Name);
    if ((List->SearchName) && (List->SearchName != List->Name)) free(List->SearchName);
    if (List->PW) free(List->PW);
    if (List->Files) FreeResponseList(List->Files);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create and add new request element to global list
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool AddRequestElement(char *Name, char *Password)
{
  _Bool             Flag = False;       /* return value */
  Request_Type      *Element;           /* new element */
  char              *Help;              /* support pointer */

  /* sanity check */
  if (Name == NULL) return Flag;

  Element = malloc(sizeof(Request_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set defaults */
    Element->PW = NULL;
    Element->Files = NULL;
    Element->Status = FREQ_NONE;
    Element->LastFile = NULL;
    Element->Next = NULL;

    /* copy data */
    Element->Name = CopyString(Name);     /* original filename pattern */ 
    if (Password) Element->PW = CopyString(Password);

    if (Env->CfgSwitches & SW_ANY_CASE)   /* case insensitive search */
    {
      Element->SearchName = CopyString(Name);     /* copy filename pattern */

      if (Element->SearchName)
      {
        /* convert search pattern to upper case */
        Help = Element->SearchName;
        while (Help[0] != 0)
        {
          Help[0] = toupper(Help[0]);
          Help++;                         /* next char */
        }
      }
    }
    else                                  /* cases sensitive search */
    {
      Element->SearchName = Element->Name;        /* same filename pattern */
    }

    /* add new element to list */
    if (Env->LastRequest) Env->LastRequest->Next = Element;    /* just link */
    else Env->RequestList = Element;                           /* start list */
    Env->LastRequest = Element;                                /* save new list end */

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

#undef REQ_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
