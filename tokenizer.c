/* ************************************************************************
 *
 *   tokenizer
 *
 *   (c) 1994-2012 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define TOKENIZER_C       1


/*
 *  include header files
 */

/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external functions */



/* ************************************************************************
 *   memory management
 * ************************************************************************ */


/*
 *  free token list
 */

void FreeTokenlist(Token_Type *List)
{
  Token_Type         *Token;

  while (List != NULL)          /* run through list */
  {
    Token = List->Next;         /* remember next token in list */

    /* free memory */
    if (List->String != NULL) free(List->String);
    free(List);

    List = Token;               /* next token element */
  }
}



/* ************************************************************************
 *   tokenizers
 * ************************************************************************ */


/*
 *  tokenize line into linked list of tokens
 *  - doesn't modify input string
 *  - separator is whitespace (one or multiple spaces and/or tabs)
 *  - supports quotations ("")
 *  - quoted special chars (\ quotes " and \)
 *  - supports comma separated strings outside quotations (comma will be kept as own token)
 */

Token_Type *Tokenize(char *Line)
{
  Token_Type        *List = NULL;       /* list of tokens */
  Token_Type        *Last = NULL;       /* last token in list */
  Token_Type        *Token;             /* new token */
  char              *Data;              /* start of token inside line */
  char              *TempData = NULL;   /* start of token inside line and new token flag */
  char              *Temp;              /* support pointer */
  _Bool             RunFlag = TRUE;     /* loop control */
  _Bool             InQuotes = FALSE;   /* token in quotation */
  _Bool             ErrFlag = FALSE;    /* signals problem */
  _Bool             Quoted = FALSE;     /* quoted special char */
  unsigned int      VirtCounter = 0;    /* virtual char counter (inside a token) */
  int               RealCounter = 0;    /* virtual char counter (inside a token) */

  if (Line == NULL) return List;       /* sanity check */

  Data = Line;          /* start of line is also first token's data */  


  /*
   *  process char by char
   */

  while (RunFlag)
  {
    /* move chars inside token if required */

    if (VirtCounter != RealCounter)
    {
      /* get rid of special chars by moving the real chars */
      Data[VirtCounter] = Data[RealCounter];
    }

    /* if line's end is reached */

    if (Line[0] == 0)
    {
      if (Quoted || InQuotes)              /* got open quote  */
      {
        /* if a quotation flag is set at the line's end
           then there is an unclosed quotation */
        ErrFlag = TRUE;
      }
      else if (RealCounter != 0)           /* end after token */
      {
        /* the line's end marks the end of the token */
        TempData = Data;                   /* flag to add last token */
      } 
      /* (RealCounter == 0) is end after whitespace */

      RunFlag = FALSE;                     /* end loop anyway */
    }

    /* if char is space or tab */

    else if ((Line[0] == ' ') || (Line[0] == '\t'))
    {
      /* a whitespace marks the end of the last token and the start of a new one
         but only outside quotations */

      if (InQuotes)                        /* inside a quotation */
      {
        VirtCounter++;                       /* it's just a standard char */
      }
      else if (Quoted)                     /* quoted special char expected */
      {
        ErrFlag = TRUE;                      /* this is a syntax error */
        RunFlag = FALSE;                     /* end loop */
      }
      else                                 /* we got a true whitespace */
      {
        /* if it's not the first char we got a new token */
        if (Data != Line)
        {
          TempData = Data;                 /* flag to create new token */
        }

        /* find the next non-whitespace char of the next token */
        Line++;                 /* move to next char */

        while ((Line[0] == ' ') || (Line[0] == '\t')) Line++;

        Data = Line;            /* set start of next token */
        Line--;                 /* because we increase Line at end of loop */
        RealCounter = -1;       /* reset char counter for new token (-1 to fix increase at loop end) */
      }
    }

    /* if char is a comma */

    else if (Line[0] == ',')
    {
      if (InQuotes)          /* in quotation mode */
      {
        VirtCounter++;         /* it's just a standard char */
      }
      else if (Quoted)       /* quoted special char expected */
      {
        ErrFlag = TRUE;        /* this is a syntax error */
        RunFlag = FALSE;       /* end loop */
      }
      else                   /* otherwise it's a separator */
      {
        if (VirtCounter > 0)     /* trailing comma */
        {
          /* create token for heading string and goto this comma again in the next loop run */
          TempData = Data;         /* flag to create new token */
          Data = Line;             /* set start of next token (this comma) */
          Line--;                  /* because we increase Line at end of loop */
        }
        else                     /* heading comma */
        {
          VirtCounter++;           /* this comma will be a token */
          TempData = Data;         /* flag to create new token */
          Data = Line;             /* set start of next token */
          Data++;                  /* char after comma will be next token */
        }
      }
    }

    /* if char is a hash */

    else if ((RealCounter == 0) && (Line[0] == '#'))
    {
      /* hash char after whitespace means the rest of the line is a remark */
      RunFlag = FALSE;                                /* end loop */      
    }

    /* if char is a backslash */

    else if (Line[0] == 92)
    {
      if (Quoted)                                     /* quoted backslash */
      {
        /* a quoted backslash is a normal char */
        Quoted = FALSE;                               /* stop quote */
        VirtCounter++;
      }
      else                                            /* quoted special char will follow */
      {
        Quoted = TRUE;
      }
    }

    /* if char is a quotation mark */

    else if (Line[0] == 34)
    {
      if (Quoted)                                     /* quoted quotation mark */
      {
        /* a quoted quotation is a normal char */
        Quoted = FALSE;                               /* stop quote */
        VirtCounter++;        
      }
      else                                            /* quotation mark */
      {
        if (InQuotes)                                 /* in quotation mode */
        {
          /* quotation ends with a second quotation mark */
          InQuotes = FALSE;                           /* stop quotation */
        }
        else                                          /* start quotation */
        {
          InQuotes = TRUE;
        }
      }
    }

    /* otherwise char is a standard char */

    else
    {
      if (Quoted)                            /* quoted special char expected */
      {
        ErrFlag = TRUE;                        /* this is a syntax error */
        RunFlag = FALSE;                       /* end loop */
      }
      else                                   /* standard char */
      {
        VirtCounter++;
      }
    }

    /* if we have to save a token */

    if (TempData != NULL)
    {
      /* create new token element and link it to token list */
      Token = calloc(1, sizeof(Token_Type));     /* create new token element */
      Temp = malloc(VirtCounter + 1);         /* create buffer for token string */

      if ((Token == NULL) || (Temp == NULL))
        Log(L_ERR, "Could not allocate memory!");

      strncpy(Temp, TempData, VirtCounter);   /* copy token data */
      Temp[VirtCounter] = 0;                  /* add missing 0 */
      Token->String = Temp;                   /* move data pointer */

      if (List == NULL) List = Token;         /* very first token */
      Token->Next = NULL;                     /* default end */
      if (Last != NULL) Last->Next = Token;   /* link new token */
      Last = Token;                           /* update pointer to last token */

      TempData = NULL;                        /* reset pointer/flag */
      VirtCounter = 0;                        /* reset char counter for new token */
      RealCounter = -1;        /* reset char counter for new token (-1 to fix increase at loop end) */
    }

    Line++;           /* move to line's next char */
    RealCounter++;    /* increment char counter for current token */
  }


  /*
   *  error handling
   */

  if (ErrFlag)
  {
    if (Quoted)            /* quoted special char */
    {
      Log(L_WARN, "Quote error: %s", Line);
    }
    else if (InQuotes)     /* unclosed quotation */
    {
      Log(L_WARN, "Quotation error: %s", Line);
    }

    FreeTokenlist(List);   /* free tokens */
    List = NULL;
  }

  return List;
}



/* ************************************************************************
 *   untokenize
 * ************************************************************************ */


/*
 *  untokenize token list
 *
 *  returns:
 *  - string pointer on success
 *  - NULL on any problem
 */

char *UnTokenize(Token_Type *List)
{
  char              *String = NULL;     /* return value */
  char              *Buffer;
  Token_Type        *Token;
  size_t            Size;
  int               Counter;

  /* sanity checks */
  if (List == NULL) return String;

  /* add all token string sizes */
  Token = List;

  while(Token)                /* run through list */
  {
    if (Token->String)        /* sanity check */
    {
      Size += strlen(Token->String) + 1;     /* 1 for space/0 */
    }

    Token = Token->Next;        /* next token */
  }

  /* allocate string */
  String = malloc(Size);

  if (String == NULL)           /* error */
  {
    Log(L_ERR, "No memory!\n");
  }
  else                          /* copy tokens into string */
  {
    Token = List;
    Counter = 0;
    Buffer = String;

    while(Token)                /* run through list */
    {
      if (Token->String)        /* sanity check */
      {
        Counter++;
        if (Counter > 1)        /* add space */
        {
          Buffer[0] = ' ';        /* set space */
          Buffer++;               /* move pointer by one char */
          Buffer[0] = 0;          /* set new string end */
        }

        strcpy(Buffer, Token->String);    /* copy token string */
        Size = strlen(Buffer);            /* get length */
        Buffer = &Buffer[Size];           /* move pointer to new end */
      }

      Token = Token->Next;        /* next token */
    }
  }

  return String;
}



/* ************************************************************************
 *   clean up of local definitions
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef TOKENIZER_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
