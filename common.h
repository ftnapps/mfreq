/* ************************************************************************
 *
 *   common header file
 *
 *   (c) 1994-2017 by Markus Reschke
 *
 * ************************************************************************ */


/* ************************************************************************
 *   common includes
 * ************************************************************************ */


/* basic stuff */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

/* time */
#include <time.h>



/* ************************************************************************
 *   constants
 * ************************************************************************ */


/*
 *  defaults for this program
 */

/* about */
#define PROJECT          "mfreq"
#define VERSION          "v3.16"
#define VERSION_MAJOR    3
#define VERSION_MINOR    16
#define COPYRIGHT        "(c) 1994-2017 by Markus Reschke"

/* default paths */
#define DEFAULT_CFG_PATH      "/etc/fido/mfreq"
#define DEFAULT_TMP_PATH      "/var/tmp"

/* buffers (must not exceed maximum size of an Integer */
#define DEFAULT_BUFFER_SIZE   4096           /* standard buffer size */

/* fileindex */
#define SUFFIX_DATA      "data"
#define SUFFIX_LOOKUP    "lookup"
#define SUFFIX_ALIAS     "alias"
#define SUFFIX_OFFSET    "offset"


/*
 *  just to be sure :-)
 */

/* boolean */
#define FALSE    0
#define False    0
#define TRUE     1
#define True     1


/*
 *  global values
 */

/* log types */
#define L_INFO           1    /* information */
#define L_DEBUG          2    /* debugging output */
#define L_WARN           3    /* warning */
#define L_ERR            4    /* error */

/* cfg switches (bitmask, 16 bits) */
/* common */
#define SW_NONE               0b0000000000000000  /* no switch set */
#define SW_BINARY_SEARCH      0b0000000000000001  /* binary search */
#define SW_ANY_CASE           0b0000000000000010  /* case-insensitive file matching */
#define SW_SI_UNITS           0b0000000000000100  /* enable SI units (input) */
#define SW_IEC_UNITS          0b0000000000001000  /* enable IEC units (output) */
/* index */
#define SW_PATH_ALIASES       0b0000000000010000  /* create path aliases */
/* frequest */
#define SW_DELETE_REQUEST     0b0000000100000000  /* delete request file (.req) */
#define SW_SEND_NETMAIL       0b0000001000000000  /* send respone netmail */
#define SW_TYPE_2             0b0000010000000000  /* packet type-2 */
#define SW_TYPE_2PLUS         0b0000100000000000  /* packet type-2+ */
#define SW_SEND_TEXT          0b0001000000000000  /* send response text file */
#define SW_LOG_REQUEST        0b0010000000000000  /* extensive logging */

/* file flags (bitmask, 16 bits) */
#define FILE_NONE             0b0000000000000000  /* no flag set */
#define FILE_SKIP             0b0000000000000001  /* skip file */

/* frequest flags (bitmask, 16 bits) */
#define REQ_NONE              0b0000000000000000  /* no flag set */
#define REQ_PROTECTED         0b0000000000000001  /* protected FTS session */
#define REQ_UNPROTECTED       0b0000000000000010  /* unprotected FTS session */
#define REQ_LISTED            0b0000000000000100  /* listed system */
#define REQ_UNLISTED          0b0000000000001000  /* unlisted system */

/* file info status (bitmask, 16 bits) */
#define FINFO_NONE            0b0000000000000000  /* no status */
#define FINFO_OK              0b0000000000000001  /* ok to list */
#define FINFO_NOT_FOUND       0b0000000000000010  /* file deleted / not found */
#define FINFO_EXCLUDED        0b0000000000000100  /* file excluded */
#define FINFO_CHANGED         0b0000000000001000  /* some value changed */

/* file response status (bitmask, 16 bits) */
#define RESP_NONE             0b0000000000000000  /* no status */
#define RESP_OK               0b0000000000000001  /* ok to send */
#define RESP_PWERROR          0b0000000000000010  /* password error */
#define RESP_OFFLINE          0b0000000000000100  /* not available right now */
#define RESP_DUPE             0b0000000000001000  /* duplicate file */
#define RESP_INTDUPE          0b0000000000010000  /* internal dupe */

/* file request status (bitmask, 16 bits) */
#define FREQ_NONE             0b0000000000000000  /* no status */
#define FREQ_NO_FILE          0b0000000000000001  /* no file found */
#define FREQ_FOUND_FILE       0b0000000000000010  /* found file(s) */
#define FREQ_LIMIT            0b0000000000010000  /* any limits exceeded */
#define FREQ_FILELIMIT        0b0000000000100000  /* file limit exceeded */
#define FREQ_BYTELIMIT        0b0000000001000000  /* byte limit exceeded */
#define FREQ_PWLIMIT          0b0000000010000000  /* bad PW limit exceeded */
#define FREQ_FREQLIMIT        0b0000000100000000  /* frequests (filename patterns) */

/* file info modes (bitmask, 16 bits) */
#define INFO_NONE             0b0000000000000000  /* no mode */
#define INFO_UPDATE           0b0000000000000001  /* update description file */
#define INFO_STRICT           0b0000000000000010  /* strict syntax checking */
#define INFO_SKIPS            0b0000000000000100  /* allow missing data fields */
#define INFO_RELAX            0b0000000000001000  /* ignore syntax errors */
#define INFO_DIR_BBS          0b0000000000010000  /* dir.bbs */
#define INFO_FILES_BBS        0b0000000000100000  /* files.bbs */
#define INFO_DESCRIPT_ION     0b0000000001000000  /* descript.ion */

/* file info fields */
#define FIELD_NONE            0    /* no type */
#define FIELD_NAME            1    /* name */
#define FIELD_SIZE            2    /* size */
#define FIELD_DATE            3    /* date */
#define FIELD_COUNTER         4    /* download counter */
#define FIELD_DESC            5    /* description */

/* field alignments */
#define ALIGN_NONE            0    /* unset */
#define ALIGN_LEFT            1    /* align left */
#define ALIGN_RIGHT           2    /* align right */

/* field formats */
#define FIELD_FORM_NONE       0    /* unset */
/* name */
#define FIELD_FORM_DOS        1    /* dos style 8.3 */
#define FIELD_FORM_LONG       2    /* long filename */
/* size */
#define FIELD_FORM_BYTES      1    /* number of bytes */
#define FIELD_FORM_UNIT       2    /* number and unit */
#define FIELD_FORM_SHORT      3    /* number and short unit */
/* date */
#define FIELD_FORM_US         1    /* MM-DD-YY */
#define FIELD_FORM_ISO        2    /* YYYY-MM-DD */
/* download counter */
#define FIELD_FORM_SQUARE     1    /* [...] */
/* description */
#define FIELD_FORM_SINGLE     1    /* single line */
#define FIELD_FORM_MULTI      2    /* multiple lines */



/* ************************************************************************
 *   types
 * ************************************************************************ */


/* tokens (linked list) */
typedef struct token_type
{
  char                   *String;       /* contains single token */
  struct token_type      *Next;         /* pointer to next element */
} Token_Type;


/* file details (linked list) */
typedef struct file_type
{
  char                   *Name;         /* filename */
  time_t                 Time;          /* time of last file modifaction */
  unsigned short         Flags;         /* additional flags/conditions */
  struct file_type       *Next;         /* pointer to next element */
} File_Type;


/* file data for file index (linked list) */
typedef struct index_data
{
  char                   *Name;         /* frequest name */
  char                   *Filepath;     /* file path */
  char                   *PW;           /* frequest password */
  struct index_data      *Next;         /* pointer to next element */
} IndexData_Type;


/* lookup data for file index (linked list) */
typedef struct index_lookup
{
  char                   Letter;        /* initial letter */
  off_t                  Offset;        /* data file offset */
  unsigned int           Start;         /* start line# */
  unsigned int           Stop;          /* stop line# */
  struct index_lookup    *Next;         /* pointer to next element */
} IndexLookup_Type;


/* path aliases for file index (linked list) */
typedef struct index_alias
{
  unsigned int           Number;        /* alias number */
  char                   *Path;         /* path */
  off_t                  Offset;        /* file offset */
  struct index_alias     *Next;         /* pointer to next element */
} IndexAlias_Type;


/* files to exclude (linked list) */
typedef struct exclude
{
  char                   *Name;         /* file name / pattern */
  /* todo: add conditions */
  struct exclude         *Next;         /* pointer to next element */
} Exclude_Type;


/* FTS address / AKA (linked list) */
/* adressing based on FRL-1002 and FSC-0039 */
typedef struct aka
{
  char              *Address;           /* address string */
  unsigned short    Zone;               /* zone number (1 - 65535) */
  unsigned short    Net;                /* net number (1 - 65535) */
  unsigned short    Node;               /* node number (0 - 65535) */
  unsigned short    Point;              /* point number (0 - 65535) */
  char              *Domain;            /* domain (max. 8 characters) */
  struct aka        *Next;              /* pointer to next element */
} AKA_Type;


/* index definition (linked list) */
typedef struct index
{
  char              *Filepath;          /* filepath of file index */
  char              *MountingPoint;     /* mounting point */
  struct index      *Next;              /* pointer to next element */
} Index_Type;


/* request limit (linked list) */
typedef struct limit
{
  char              *Address;           /* FTS address pattern */
  long              Files;              /* number of files */
  long long         Bytes;              /* sum of bytes (twice as long as off_t) */
  int               BadPWs;             /* number of bad passwords */
  int               Freqs;              /* number of frequests (file patterns) */ 
  unsigned short    Flags;              /* additional flags/conditions */
  struct limit      *Next;              /* pointer to next element */
} Limit_Type;


/* file found (linked list) */
typedef struct response
{
  char              *Filepath;          /* filepath */
  off_t             Size;               /* filesize */
  unsigned short    Status;             /* file status */
  struct response   *Next;              /* pointer to next element */
} Response_Type;


/* requested file (linked list) */
typedef struct request
{
  char              *Name;              /* file name or pattern */
  char              *PW;                /* password */
  unsigned short    Status;             /* request status */
  Response_Type     *Files;             /* files to send (linked list) */
  Response_Type     *LastFile;          /* pointer to last element in list */
  struct request    *Next;              /* pointer to next element */
} Request_Type;


/* file information (linked list) */
typedef struct info
{
  char              *Name;              /* file name */
  off_t             Size;               /* file size */
  time_t            Time;               /* time of last file modifaction */
  unsigned short    Counter;            /* download counter */
  unsigned short    Status;             /* file status */
  Token_Type        *Infos;             /* file decription (linked list) */
  Token_Type        *LastInfo;          /* pointer to last element in list */
  struct info       *Next;              /* pointer to next element */ 
} Info_Type;


/* data field for file description file (linked list) */
typedef struct field
{
  unsigned short    Type;               /* type ID */
  unsigned short    Line;               /* line number */
  unsigned short    Pos;                /* position */
  unsigned short    Width;              /* width (max. size) */
  unsigned short    Align;              /* alignment */
  unsigned short    Format;             /* format ID */
  struct field      *Next;              /* pointer to next element */
} Field_Type;


/* global environment */
typedef struct
{
  /* basic filepaths */
  char              *CfgFilepath;       /* configuration filepath */
  char              *LogFilepath;       /* log filepath */

  /* process */
  char              *CWD;               /* current working directory */
  pid_t             PID;                /* process ID */

  /* global time */
  time_t            UnixTime;
  struct tm         DateTime;

  /* file streams */
  FILE              *Log;               /* log file */
  FILE              *List;              /* filelist */

  /* program control */
  _Bool             Run;                /* stop/keep running */
  unsigned short    ConfigDepth;        /* depth of cfg files (includes) */
  char              *CfgInUse;          /* cfg currently parsed */
  unsigned int      CfgLinenumber;      /* line number currently parsed */

  /* common configuration */
  unsigned short    CfgSwitches;        /* several cfg switches */

  /* file index */
  IndexData_Type    *DataList;          /* index data (linked list) */
  IndexData_Type    *LastData;          /* pointer to last element in list */
  IndexLookup_Type  *LookupList;        /* index lookup (linked list) */
  IndexLookup_Type  *LastLookup;        /* pointer to last element in list */
  IndexAlias_Type   *AliasList;         /* index lookup (linked list) */
  IndexAlias_Type   *LastAlias;         /* pointer to last element in list */
  Exclude_Type      *ExcludeList;       /* file exclusion (linked list) */
  Exclude_Type      *LastExclude;       /* pointer to last element in list */
  File_Type         *FileList;          /* file details (linked list) */
  File_Type         *LastFile;          /* pointer to last element in list */

  /* frequest configuration */
  char              *MailPath;          /* path of response mail */
  Token_Type        *MailHeader;        /* mail header (linked list) */
  Token_Type        *LastHeader;        /* pointer to last element in list */
  Token_Type        *MailFooter;        /* mail footer (linked list) */
  Token_Type        *LastFooter;        /* pointer to last element in list */
  AKA_Type          *AKA;               /* my AKAs (linked list) */
  AKA_Type          *LastAKA;           /* pointer to last element in list */
  Index_Type        *IndexList;         /* index files (linked list) */
  Index_Type        *LastIndex;         /* pointer to last element in list */
  Limit_Type        *LimitList;         /* request limits (linked list) */
  Limit_Type        *LastLimit;         /* pointer to last element in list */

  /* frequest filepaths */
  char              *SRIF_Filepath;     /* filepath of SRIF file */
  char              *RequestFilepath;   /* filepath of request */
  char              *ResponseFilepath;  /* filepath of response */
  char              *MailFilepath;      /* filepath of netmail */
  char              *TextFilepath;      /* filepath of textmail */
  
  /* frequest(er) details */
  char              *Sysop;             /* sysop name */
  AKA_Type          *ReqAKA;            /* requesters AKAs (linked list) */
  AKA_Type          *ReqLastAKA;        /* pointer to last element in list */
  AKA_Type          *CalledAKA;         /* which AKA is called */
  long              BPS;                /* speed (in bps) of connection */
  long              ReqTime;            /* time (in minutes) allowed */
  unsigned short    ReqFlags;           /* session/system flags */
  Request_Type      *RequestList;       /* request list (linked list) */
  Request_Type      *LastRequest;       /* pointer to last element in list */  

  /* frequest runtime stuff */
  Limit_Type        *ActiveLimit;       /* limits to use for requester */
  long              Files;              /* file counter */
  long long         Bytes;              /* byte counter (twice as long as off_t) */
  unsigned short    FreqStatus;         /* status of frequest */
  AKA_Type          *ActiveLocalAKA;    /* local AKA to be used */
  AKA_Type          *ActiveRemoteAKA;   /* remote AKA to be used */

  /* filelist */
  char              *ListFilepath;      /* filepath of filelist */
  unsigned short    InfoMode;           /* file info mode */
  Info_Type         *InfoList;          /* file information list (linked list) */
  Info_Type         *LastInfo;          /* pointer to last element in list */
  /* file description fields (linked list) */
  Field_Type        *Fields_filelist;   /* for filelist */
  Field_Type        *Fields_files_bbs;  /* for files.bbs */

} Env_Type;



/* ************************************************************************
 *   EOF
 * ************************************************************************ */
