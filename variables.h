/* ************************************************************************
 *
 *   global variables
 *
 *   (c) 1994-2012 by Markus Reschke
 *
 * ************************************************************************ */


/* ************************************************************************
 *   internal variables
 * ************************************************************************ */

/*
 *  only included by
 *  - mfreq-index.c
 *  - mfreq-list.c 
 *  - mfreq-srif.c
 */

#if defined ( MFREQ_INDEX_C ) || defined ( MFREQ_LIST_C ) || defined( MFREQ_SRIF_C )

  /* environment */
  Env_Type                *Env = NULL;

  /* string buffers */
  char                    *LogBuffer = NULL;      /* for errors/warnings */
  char                    *InBuffer = NULL;       /* for file reading */
  char                    *OutBuffer = NULL;      /* for file writing */
  char                    *TempBuffer = NULL;     /* for misc stuff */
  char                    *TempBuffer2 = NULL;    /* for misc stuff */


/* ************************************************************************
 *   external variables
 * ************************************************************************ */

/*
 *  included by all other source files
 */

#else

  /* environment */
  extern Env_Type                *Env;

  /* string buffers */
  extern char                    *LogBuffer;
  extern char                    *InBuffer;
  extern char                    *OutBuffer;
  extern char                    *TempBuffer;
  extern char                    *TempBuffer2;

#endif


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
