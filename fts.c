/* ************************************************************************
 *
 *   FTS stuff
 *
 *   (c) 1994-2013 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  local constants
 */

#define FTS_C       1



/*
 *  include header files
 */

/* local header files */
#include "common.h"           /* common stuff */
#include "variables.h"        /* global variables */
#include "functions.h"        /* external funtions */

/* extra */
#include <endian.h>



/*
 *  constants
 */

/* message attributes (bitmask, 16 bits) */
#define ATTR_PRIVATE                    0b0000000000000001
#define ATTR_CRASH                      0b0000000000000010
#define ATTR_RECD                       0b0000000000000100
#define ATTR_SENT                       0b0000000000001000
#define ATTR_FILE_ATTACHED              0b0000000000010000
#define ATTR_IN_TRANSIT                 0b0000000000100000
#define ATTR_ORPHAN                     0b0000000001000000
#define ATTR_KILL_SENT                  0b0000000010000000
#define ATTR_LOCAL                      0b0000000100000000
#define ATTR_HOLD_FOR_PICKUP            0b0000001000000000
#define ATTR_UNUSED                     0b0000010000000000
#define ATTR_FILE_REQUEST               0b0000100000000000
#define ATTR_RETURN_RECEIPT_REQUEST     0b0001000000000000
#define ATTR_IS_RETURN_RECEIPT          0b0010000000000000
#define ATTR_AUDIT_REQUEST              0b0100000000000000
#define ATTR_FILE_UPDATE_REQ            0b1000000000000000

/* capability word / supported bundle types (bitmask, 16 bits) */
#define CW_STONEAGE           0b0000000000000000
#define CW_2                  0b0000000000000001
#define CW_2PLUS              0b0000000000000001



/*
 *  local types
 */


/* type-2 packet header (FTS-0001) */
typedef struct
{
  /* byte order for all 16 bit values: LSB MSB */
  unsigned short    origNode;      /* origin node */
  unsigned short    destNode;      /* destination node */
  unsigned short    year;          /* year of packet creation */
                                   /* value range: 1-65535 */
  unsigned short    month;         /* month of packet creation */
                                   /* value range: 0-11 for Jan-Dec */
  unsigned short    day;           /* day of packet creation */
                                   /* value range: 1-31 */
  unsigned short    hour;          /* hour of packet creation */
                                   /* value range: 0-23 */
  unsigned short    minute;        /* minute of packet creation */
                                   /* value range: 0-59 */
  unsigned short    second;        /* second of packet creation */
                                   /* value range: 0-59 */
  unsigned short    baud;          /* max. bps rate */
  unsigned short    packetType;    /* packet type */
                                   /* value: 2 (0x02 0x00) */
  unsigned short    origNet;       /* origin net */
  unsigned short    destNet;       /* destination net */
  unsigned char     prodCode;      /* product code */
  unsigned char     serialNo;      /* serial number or null */
  unsigned char     password[8];   /* password, null padded */
                                   /* value range: A-Z, 0-9 */
  unsigned short    origZone;      /* origin zone */
  unsigned short    destZone;      /* destination zone */
  unsigned char     fill[20];      /* padding, null padded */
} PacketHeader2_Type;


/* type-2 extended packet header (FSC-0039) */
typedef struct
{
  /* byte order for all 16 bit values: LSB MSB */
  unsigned short    OrgNode;       /* origin node */
  unsigned short    DstNode;       /* destination node */
  unsigned short    Year;          /* year of packet creation */
                                   /* value range: 1-65535 */
  unsigned short    Month;         /* month of packet creation */
                                   /* value range: 0-11 for Jan-Dec */
  unsigned short    Day;           /* day of packet creation */
                                   /* value range: 1-31 */
  unsigned short    Hour;          /* hour of packet creation */
                                   /* value range: 0-23 */
  unsigned short    Min;           /* minute  of packet creation */
                                   /* value range: 0-59 */
  unsigned short    Sec;           /* second of packet creation */
                                   /* value range: 0-59 */
  unsigned short    Baud;          /* max. bps rate */
  unsigned short    PktVer;        /* packet version */
                                   /* value: 2 (0x02 0x00) */
  unsigned short    OrgNet;        /* origin net */
  unsigned short    DstNet;        /* destination net */
  unsigned char     PrdCod_L;      /* FTSC product code LSB */
  unsigned char     PVMajor;       /* product revision MSB */
  unsigned char     Password[8];   /* password, null padded */
  unsigned short    QOrgZone;      /* origin zone (ZMailQ,QMail) */
  unsigned short    QDestZone;     /* destination zone (ZMailQ,QMail) */
  unsigned char     Filler[4];     /* spare */
  unsigned char     PrdCod_H;      /* FTSC product code MSB */
  unsigned char     PVMinor;       /* FTSC product revision LSB */
  unsigned short    CapWord;       /* capability word */
  unsigned short    OrigZone;      /* origination zone */
  unsigned short    DestZone;      /* destination zone */
  unsigned short    OrigPoint;     /* origination point */
  unsigned short    DestPoint;     /* destination point */
  unsigned char     ProdData[4];   /* product specific data */
} PacketHeader3ext_Type;


/* type-2+ packet header (FSC-0048) */
typedef struct
{
  /* byte order for all 16 bit values: LSB MSB */
  unsigned short    origNode;      /* origin node */
  unsigned short    destNode;      /* destination node */
  unsigned short    year;          /* year of packet creation */
                                   /* value range: 1-65535 */
  unsigned short    month;         /* month of packet creation */
                                   /* value range: 0-11 for Jan-Dec */
  unsigned short    day;           /* day of packet creation */
                                   /* value range: 1-31 */
  unsigned short    hour;          /* hour of packet creation */
                                   /* value range: 0-23 */
  unsigned short    minute;        /* minute  of packet creation */
                                   /* value range: 0-59 */
  unsigned short    second;        /* second of packet creation */
                                   /* value range: 0-59 */
  unsigned short    baud;          /* max. bps rate of orig and dest */
  unsigned short    packetType;    /* packet version */
                                   /* value: 2 (0x02 0x00) */
  unsigned short    origNet;       /* origin net */
                                   /* -1 if from point */
  unsigned short    destNet;       /* destination net */
  unsigned char     ProductCode_L; /* FTSC product code LSB */
  unsigned char     Revision_H;    /* product major revision */
  unsigned char     password[8];   /* password, null padded */
                                   /* valid chars: A-Z, 0-9 */
  unsigned short    QorigZone;     /* origin zone (as in QMail) */
  unsigned short    QdestZone;     /* destination zone (as in QMail) */
  unsigned short    AuxNet;        /* origin net if origin is a point */
  unsigned short    CWvalidationCopy;  /* copy of capability word */
                                       /* in reversed byte order (MSB LSB) */
  unsigned char     ProductCode_H; /* FTSC product code MSB */
  unsigned char     Revision_L;    /* product minor revision */
  unsigned short    CapabilWord;   /* capability word */
  unsigned short    origZone;      /* origination zone (as in FD etc) */
  unsigned short    destZone;      /* destination zone (as in FD etc) */
  unsigned short    origPoint;     /* origination point (as in FD etc) */
  unsigned short    destPoint;     /* destination point (as in FD etc) */
  unsigned char     ProdData[4];   /* product specific data */
} PacketHeader2plus_Type;


/* message header (FTS-0001) */
typedef struct
{
  /* byte order for all 16 bit values: LSB MSB */
  unsigned short    messageType;   /* version/type */
                                   /* value: 2 (0x02 0x00) */
  unsigned short    origNode;      /* origin node */
  unsigned short    destNode;      /* destination node */
  unsigned short    origNet;       /* origin net */
  unsigned short    destNet;       /* destination net */
  unsigned short    Attribute;     /* message attributes */
  unsigned short    cost;          /* cost of origin */
} MessageHeader_Type;




/* ************************************************************************
 *   AKAs
 * ************************************************************************ */


/*
 *  free list of AKA elements
 */

void FreeAKAlist(AKA_Type *List)
{
  AKA_Type                *Next;

  /* sanity check */
  if (List == NULL) return;

  while (List)
  {
    Next = List->Next;           /* save pointer to next element */

    /* free data */
    if (List->Address) free (List->Address);
    if (List->Domain) free (List->Domain);

    /* free structure */
    free(List);

    List = Next;                 /* move to next element */
  }
}



/*
 *  create new AKA element
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

AKA_Type *NewAKA(char *Address)
{
  _Bool              Flag = False;            /* control flag */
  AKA_Type           *Element = NULL;         /* return value */
  char               *Help1 = NULL, *Help2 = NULL;
  size_t             Length;
  long               Value;

  /* sanity check */
  if (Address == NULL) return Element;

  Element = malloc(sizeof(AKA_Type));    /* allocate memory */

  if (Element)          /* success */
  {
    /* set default values */
    Element->Address = NULL;
    Element->Zone = 0;
    Element->Net = 0;
    Element->Node = 0;
    Element->Point = 0;
    Element->Domain = NULL;
    Element->Next = NULL;

    /*
     *  extract details of address
     *  - may not include a point number (if node)
     *  - domain is optional
     */

    /* init address parser */
    Length = strlen(Address);
    if (Length < DEFAULT_BUFFER_SIZE - 1)
    {
      strcpy(TempBuffer, Address);
      Flag = True;
    }


    /* get zone */
    if (Flag)
    {
      Flag = False;                           /* reset flag */
      Help1 = TempBuffer;                     /* set start */
      Help2 = Help1;

      while ((Help2[0] != 0) && (Help2[0] != ':')) Help2++;   /* find ":" */

      if (Help2[0] != 0)                      /* got it */
      {
        Help2[0] = 0;                           /* create substring */
        Help2++;
        Value = Str2Long(Help1);                /* convert */
        if ((Value > 0) && (Value <= 65535))    /* check value */
        {
          Element->Zone = Value;                  /* save value */
          Flag = True;                            /* ok to proceed */
        }
      }
    }

    /* get net */
    if (Flag)
    {
      Flag = False;                           /* reset flag */
      Help1 = Help2;                          /* move start */

      while ((Help2[0] != 0) && (Help2[0] != '/')) Help2++;   /* find "/" */

      if (Help2[0] != 0)                      /* got it */
      {
        Help2[0] = 0;                           /* create substring */
        Help2++;
        Value = Str2Long(Help1);                /* convert */
        if ((Value > 0) && (Value <= 65535))    /* check value */
        {
          Element->Net = Value;                   /* save value */
          Flag = True;                            /* ok to proceed */
        }
      }
    }

    /* get domain (optional) */
    if (Flag)
    {
      Flag = False;                           /* reset flag */
      Help1 = Help2;                          /* move start */

      while ((Help2[0] != 0) && (Help2[0] != '@')) Help2++;   /* find "@" */

      if (Help2[0] != 0)                      /* got it */
      {
        Help2[0] = 0;                           /* create substring */
        Help2++;
        Length = strlen(Help2);
        if ((Length > 0) && (Length <= 8))      /* up to 8 chars */
        {
          Element->Domain = CopyString(Help2);    /* save string */
          Flag = True;                            /* ok to proceed */
        }
      }
      else                                    /* no domain */
      {
        Flag = True;                            /* ok to proceed */
      }
    }

    /* get point (optional) */
    if (Flag)
    {
      Flag = False;                           /* reset flag */
      Help2 = Help1;                          /* move back to start */

      while ((Help2[0] != 0) && (Help2[0] != '.')) Help2++;   /* find "." */

      if (Help2[0] != 0)                      /* got it */
      {
        Help2[0] = 0;                           /* create substring */
        Help2++;
        Value = Str2Long(Help2);                /* convert */
        if ((Value >= 0) && (Value <= 65535))   /* check value */
        {
          Element->Point = Value;                 /* save value */
          Flag = True;                            /* ok to proceed */
        }
      }
      else                                    /* no point */
      {
        Flag = True;                            /* ok to proceed */
      }
    }

    /* get node */
    if (Flag)
    {
      Flag = False;                           /* reset flag */

      /* the remaining substring contains the node number */
      Value = Str2Long(Help1);                /* convert */
      if ((Value >= 0) && (Value <= 65535))   /* check value */
      {
        Element->Node = Value;                  /* save value */
        Flag = True;                            /* ok to proceed */
      }        
    }


    if (Flag)          /* got details */
    {
      /* to be able to compare address strings we have to normalize them */

      /* basic address */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1, "%d:%d/%d.%d",
        Element->Zone, Element->Net, Element->Node, Element->Point);

      if (Element->Domain)        /* add domain */
      {
        Length = strlen(TempBuffer);
        snprintf(&TempBuffer[Length], DEFAULT_BUFFER_SIZE - 1 - Length,
          "@%s", Element->Domain);
      }

      /* copy data */
      Element->Address = CopyString(TempBuffer);
    }
    else               /* error */
    {
      FreeAKAlist(Element);           /* free element */
      Element = NULL;                 /* reset pointer */
    }
  }
  else                                 /* error */
  {
    Log(L_ERR, "Couldn't allocate memory!");
  }

  return Element;
}



/*
 *  check if two AKAs match somehow :-)
 *
 *  returns:
 *  - 1 on match
 *  - 0 on error or mismatch
 */

_Bool MatchAKAs(AKA_Type *AKA1, AKA_Type *AKA2)
{
  _Bool                  Flag = False;        /* return value */

  /* sanity checks */
  if ((AKA1 == NULL) || (AKA2 == NULL)) return Flag;

  /* first check domains if available */
  if (AKA1->Domain && AKA2->Domain)
  {
    if (strcmp(AKA1->Domain, AKA2->Domain) == 0) Flag = True;
  }

  /* if no match yet, check zones */
  if (Flag == False)
  {
    if (AKA1->Zone == AKA2->Zone) Flag = True;
  }

  /* Nothing more to check, since we don't know which zones belong
     to which network. For Fidonet we know, but not for all the others. */

  return Flag;
}



/* ************************************************************************
 *   netmail & packet
 * ************************************************************************ */


/*
 *  convert number of month (0-11) to string
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool Month2String(unsigned int Number, char *String, unsigned int Length)
{
  _Bool             Flag = False;        /* return value */
  char              *Months[12] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  /* sanity checks */
  if ((String == NULL) || (Length < 4)) return Flag;

  if (Number <= 11)
  {
    strcpy(String, Months[Number]);
    Flag = True;              /* signal success */
  }

  return Flag;
}



/*
 *  create FTS packet header (FTS-0001)
 *
 *  todo: support FSC-0039
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WritePacketHeader(FILE *File)
{
  _Bool                  Flag = False;        /* return value */
  PacketHeader2_Type     *Packet2;
  PacketHeader2plus_Type *Packet2plus;
  unsigned short         n;

  /* sanity check */
  if (File == NULL) return Flag;

  /* all packet header types got the same size */
  Packet2 = malloc(sizeof(PacketHeader2_Type));   /* allocate memory */

  if (Packet2)
  {
    /*
     *  fill in common data
     */

    Packet2->origNode = htole16(Env->ActiveLocalAKA->Node);
    Packet2->destNode = htole16(Env->ActiveRemoteAKA->Node);
    Packet2->year = htole16(Env->DateTime.tm_year + 1900);
    Packet2->month = htole16(Env->DateTime.tm_mon);
    Packet2->day = htole16(Env->DateTime.tm_mday);
    Packet2->hour = htole16(Env->DateTime.tm_hour);
    Packet2->minute = htole16(Env->DateTime.tm_min);
    Packet2->second = htole16(Env->DateTime.tm_sec);
    Packet2->baud = 0;
    Packet2->packetType = htole16(2);
    Packet2->origNet = htole16(Env->ActiveLocalAKA->Net);
    Packet2->destNet = htole16(Env->ActiveRemoteAKA->Net);
    Packet2->prodCode = 0xFE;      /* no allocated product ID */
    Packet2->serialNo = 0;

    /* empty password (null padded) */
    for (n = 0; n < 8; n++) Packet2->password[n] = 0;

    Packet2->origZone = htole16(Env->ActiveLocalAKA->Zone);
    Packet2->destZone = htole16(Env->ActiveRemoteAKA->Zone);


    /*
     *  fill in packet type specific data
     */

    if (Env->CfgSwitches & SW_TYPE_2PLUS)    /* type-2+ */
    {
      Packet2plus = (PacketHeader2plus_Type *)Packet2;

      Packet2plus->Revision_H = VERSION_MAJOR;
      Packet2plus->Revision_L = VERSION_MINOR;

      /* special treatment if origin is a point */
      if (Env->ActiveLocalAKA->Point > 0)
      {
        Packet2plus->AuxNet = Packet2plus->origNet;
        Packet2plus->origNet = htole16(-1);
      }

      Packet2plus->ProductCode_L = 0xFE;     /* no allocated product ID */
      Packet2plus->ProductCode_H = 0xFE;     /* request one from FTSC? */

      Packet2plus->CapabilWord = htole16(CW_2PLUS);
      /* copy of Capability Word in reversed byte order (MSB LSB) */
      Packet2plus->CWvalidationCopy = htobe16(CW_2PLUS);

      Packet2plus->origZone = htole16(Env->ActiveLocalAKA->Zone);
      Packet2plus->destZone = htole16(Env->ActiveRemoteAKA->Zone);
      Packet2plus->origPoint = htole16(Env->ActiveLocalAKA->Point);
      Packet2plus->destPoint = htole16(Env->ActiveRemoteAKA->Point);

      /* product data (null padded) */
      for (n = 0; n < 4; n++) Packet2plus->ProdData[n] = 0;
    }
    else                                     /* type-2 (default) */
    {
      /* empty data (null padded) */
      for (n = 0; n < 20; n++) Packet2->fill[n] = 0;
    }


    /* write packet header */
    if (fwrite(Packet2, sizeof(PacketHeader2_Type), 1, File) == 1)
    {
      Flag = True;            /* signal success */
    }

    free(Packet2);            /* free structure */
  }

  return Flag;
}



/*
 *  create FTS message header (FTS-0001)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteNetmailHeader(FILE *File)
{
  _Bool                  Flag = False;        /* return value */
  MessageHeader_Type     *NetMail;
  char                   *Buffer;
  char                   Month[4];
  size_t                 Size = 0;

  /* sanity checks */
  if ((File == NULL) || (DEFAULT_BUFFER_SIZE < 170)) return Flag;

  NetMail = malloc(sizeof(MessageHeader_Type));    /* allocate memory */

  if (NetMail)
  {
    /* fill in data */
    NetMail->messageType = htole16(2);
    NetMail->origNode = htole16(Env->ActiveLocalAKA->Node);
    NetMail->destNode = htole16(Env->ActiveRemoteAKA->Node);
    NetMail->origNet = htole16(Env->ActiveLocalAKA->Net);
    NetMail->destNet = htole16(Env->ActiveRemoteAKA->Net);
    NetMail->Attribute = htole16(ATTR_PRIVATE);
    NetMail->cost = 0;

    /* write message header */
    if (fwrite(NetMail, sizeof(MessageHeader_Type), 1, File) == 1)
    {
      Flag = True;             /* signal success */
    }

    free(NetMail);       /* free structure */
  }

  /* datetime and variable length fields */
  if (Flag)
  {
    Flag = False;        /* reset flag */
    Buffer = OutBuffer;

    /* DateTime, 20 bytes, null terminated */
    /* format: "DD MMM YY  HH:MM:SS" (MMM = Jan-Dec) */
    Month[0] = 0;
    Month2String(Env->DateTime.tm_mon, &(Month[0]), 4);
    snprintf(Buffer, 20,
      "%02d %s %02d  %02d:%02d:%02d", 
      Env->DateTime.tm_mday,
      Month,
      (Env->DateTime.tm_year + 1900) % 100,
      Env->DateTime.tm_hour,
      Env->DateTime.tm_min,
      Env->DateTime.tm_sec);
    Size += strlen(Buffer) + 1;
    Buffer = &(OutBuffer[Size]);     /* advance to first char behind 0 */
 
    /* toUserName, max. 36 bytes, null terminated */
    snprintf(Buffer, 36,
      "%s", Env->Sysop);
    Size += strlen(Buffer) + 1;
    Buffer = &(OutBuffer[Size]);     /* advance to first char behind 0 */

    /* fromUserName , max. 36 bytes, null terminated */
    snprintf(Buffer, 36, "mfreq");
    Size += strlen(Buffer) + 1;
    Buffer = &(OutBuffer[Size]);     /* advance to first char behind 0 */

    /* subject, max. 72 bytes, null terminated */
    snprintf(Buffer, 72, "Your Frequest");
    Size += strlen(Buffer) + 1;

    /* write fields */
    if (fwrite(OutBuffer, Size, 1, File) == 1)
    {
      Flag = True;             /* signal success */
    }
  }

  return Flag;
}



/*
 *  create message kludges
 *  - FTS-0001 FMPT, TOPT, INTL
 *  - FSC-0005 INTL 
 *  - FTS-0009 MSGID
 *  - FSC-0054 CHRS
 *  - FSP-1013 CHRS
 *  - FSC-0046 PID
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteNetmailKludges(FILE *File)
{
  _Bool                  Flag = False;        /* return value */
  char                   *Buffer;
  size_t                 Size = 0;

  /* sanity checks */
  if ((File == NULL) || (DEFAULT_BUFFER_SIZE < 200)) return Flag;

  /*
   *  hints:
   *  - each kludge is started by SOH (0x01)
   *  - each line is terminated by CR and LF (0x0D 0x0A)
   */

  Buffer = OutBuffer;


  /*
   *  INTL <dest zone>:<dest net>/<dest node> <orig zone>:<orig net>/<orig node>
   */

  snprintf(Buffer, 50,
    "\001INTL %u:%u/%u %u:%u/%u\r\n",
     Env->ActiveRemoteAKA->Zone,
     Env->ActiveRemoteAKA->Net,
     Env->ActiveRemoteAKA->Node, 
     Env->ActiveLocalAKA->Zone,
     Env->ActiveLocalAKA->Net,
     Env->ActiveLocalAKA->Node);
  Size += strlen(Buffer);
  Buffer = &(OutBuffer[Size]);


  /*
   *  FMPT <orig point>
   */

  if (Env->ActiveLocalAKA->Point > 0)
  {
    snprintf(Buffer, 15, "\001FMPT %u\r\n", Env->ActiveLocalAKA->Point);
    Size += strlen(Buffer);
    Buffer = &(OutBuffer[Size]);
  }


  /*
   *  TOPT <dest point>
   */

  if (Env->ActiveRemoteAKA->Point > 0)
  {
    snprintf(Buffer, 15, "\001TOPT %u\r\n", Env->ActiveRemoteAKA->Point);
    Size += strlen(Buffer);
    Buffer = &(OutBuffer[Size]);
  }


  /*
   *  MSGID: <FTS address> <serial number>
   *  serial number: 8 digits, hexadecimal, lower case
   */

  snprintf(Buffer, 50, "\001MSGID: %s %08x\r\n",
    Env->ActiveLocalAKA->Address, (unsigned int)Env->UnixTime);
  Size += strlen(Buffer);
  Buffer = &(OutBuffer[Size]);  


  /*
   *  CHRS: <identifier> <level>
   *  identifiers for level 2: LATIN-1, ASCII, IBMPC, ... 
   */

  snprintf(Buffer, 20, "\001CHRS: LATIN-1 2\r\n");
  Size += strlen(Buffer);
  Buffer = &(OutBuffer[Size]);


  /*
   *  PID: <pID> <version>[ <serial#>]
   */

  snprintf(Buffer, 30, "\001PID: "PROJECT" "VERSION"\r\n");
  Size += strlen(Buffer);
  Buffer = &(OutBuffer[Size]);

  /* write kludges */
  if (fwrite(OutBuffer, Size, 1, File) == 1)
  {
    Flag = True;             /* signal success */
  }

  return Flag;
}



/*
 *  create netmail/textmail content
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteMailContent(FILE *File)
{
  _Bool                  Flag = False;        /* return value */
  Token_Type             *Token;
  Request_Type           *Request;
  Response_Type          *Response;
  char                   *Help;

  /* sanity check */
  if (File == NULL) return Flag;

  /*
   *  hints:
   *  - each line is terminated by CR and LF (0x0D 0x0A)
   */

  Flag = True;

  /*
   *  header
   */

  if (Env->MailHeader)            /* when we got a header */
  {
    /* add empty line */
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1, "\r\n");
    fputs(OutBuffer, File);

    /* add header lines */
    Token = Env->MailHeader;
    while (Token)                  /* follow list */
    {
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s\r\n", Token->String);
      fputs(OutBuffer ,File);
      Token = Token->Next;           /* next one */
    }
  }

  /*
   *  results
   */

  Request = Env->RequestList;

  if (Request == NULL)             /* no request */
  {
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
      "\r\nNothing requested!\r\n");
    fputs(OutBuffer, File);
  }

  while (Request)                  /* follow list */
  {
    /* what's requested */
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
      "\r\nRequested \"%s\":\r\n", Request->Name);
    fputs(OutBuffer, File);

    Response = Request->Files;

    if (Response == NULL)        /* no match(es) */
    {
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "  - nothing found\r\n");
      fputs(OutBuffer, File);
    }
    else                         /* got match(es) */    
    {
      while (Response)             /* follow list */
      {
        Help = GetFilename(Response->Filepath);

        if (Help)       /* sanity check */
        {
          if (Response->Status & STAT_OK)       /* valid file */
          {
            if (LongLong2ByteString(Response->Size, TempBuffer, DEFAULT_BUFFER_SIZE - 1))
              snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
                "  - %s (%sytes)\r\n", Help, TempBuffer);
            else
              snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
                "  - %s (%ld Bytes)\r\n", Help, Response->Size);

            fputs(OutBuffer, File);
          }
          else if (Response->Status & STAT_FILELIMIT)
          {
            snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
              "  - next file would exceed file limit\r\n");
            fputs(OutBuffer, File);
          }
          else if (Response->Status & STAT_BYTELIMIT)
          {
            snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
              "  - next file would exceed byte limit\r\n");
            fputs(OutBuffer, File);
          }
          else if (Response->Status & STAT_OFFLINE)
          {
            snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
              "  - %s (currently not available)\r\n", Help);
            fputs(OutBuffer, File);
          }
        }

        Response = Response->Next;      /* next one */ 
      }
    }

    Request = Request->Next;       /* next one */
  }


  /*
   *  totals
   */

  snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
    "\r\nTotals:\r\n");
  fputs(OutBuffer, File);
  snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
    "  - %ld files\r\n", Env->Files);
  fputs(OutBuffer, File);
  if (LongLong2ByteString(Env->Bytes, TempBuffer, DEFAULT_BUFFER_SIZE - 1))
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
      "  - %sytes\r\n", TempBuffer);
  else
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
      "  - %lld bytes\r\n", Env->Bytes); 
  fputs(OutBuffer, File);


  /*
   *  footer
   */

  if (Env->MailFooter)            /* when we got a footer */
  {
    /* add empty line */
    snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1, "\r\n");
    fputs(OutBuffer, File);

    /* add footer lines */
    Token = Env->MailFooter;
    while (Token)                  /* follow list */
    {
      snprintf(OutBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s\r\n", Token->String);
      fputs(OutBuffer, File);
      Token = Token->Next;           /* next one */
    }
  }

  return Flag;
}



/*
 *  create FTS packet end (FTS-0001)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WriteNetmailEnd(FILE *File)
{
  _Bool                  Flag = False;        /* return value */

  /* sanity check */
  if (File == NULL) return Flag;

  /*
   *  hints:
   *  - message text is null terminated
   */

  Flag = True;

  /* terminate message text with 0 */
  OutBuffer[0] = 0;
  if (fwrite(OutBuffer, 1, 1, File) != 1) Flag = False;

  return Flag;
}




/*
 *  create FTS packet end (FTS-0001)
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool WritePacketEnd(FILE *File)
{
  _Bool                  Flag = False;        /* return value */
  unsigned short         Version = 0;

  /* sanity check */
  if (File == NULL) return Flag;

  /* it's just a packet version field with a value of 0 */

  /* write packet end */
  if (fwrite(&Version, 2, 1, File) == 1)
  {
    Flag = True;             /* signal success */
  }

  return Flag;
}



/*
 *  create response netmail
 *
 *  returns:
 *  - 1 on success
 *  - 0 on error
 */

_Bool NetMail()
{
  _Bool                  Flag = False;        /* return value */
  FILE                   *File;
  unsigned short         n = 0;
  unsigned int           Value;    

  /* create filepath for packet */
  if (Env->MailFilepath == NULL)
  {
    Value = (unsigned int)Env->UnixTime;

    while (n < 10)          /* try up to 10 times */
    {
      /* use unix time plus offset */
      snprintf(TempBuffer, DEFAULT_BUFFER_SIZE - 1,
        "%s/%08x.pkt", Env->MailPath, Value & 0xFFFFFFFF);

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
        Env->MailFilepath = CopyString(TempBuffer);    /* use this filepath */
        n = 10;                 /* end loop */
      }
    }

    if (Env->MailFilepath == NULL)
    {
      Log(L_WARN, "Couldn't create usable netmail filepath!");
      return Flag;
    }
  }

  /* create netmail packet */
  File = fopen(Env->MailFilepath, "w");       /* truncate & write mode  */
  if (File)                                   /* opened */
  {
    /* create netmail packet */
    if (WritePacketHeader(File) &&
        WriteNetmailHeader(File) &&
        WriteNetmailKludges(File) &&
        WriteMailContent(File) &&
        WriteNetmailEnd(File) &&
        WritePacketEnd(File))
    {
      Flag = True;           /* signal success */
    }

    fclose(File);                          /* close file */
  }
  else                                        /* file error */
  {
    Log(L_WARN, "Couldn't open netmail packet (%s)!", Env->MailFilepath);
  }

  /* clean up */
  if (!Flag)        /* on error */
  {
    /* try to remove broken netmail */
    if (File)
    {
      unlink(Env->MailFilepath);
    }

    /* reset netmail filepath to prevent sending of broken netmail */
    if (Env->MailFilepath)
    {
      free(Env->MailFilepath);
      Env->MailFilepath = NULL;
    }

    /* log problem */
    Log(L_WARN, "Couldn't create netmail packet!");
  }

  return Flag;
}



/* ************************************************************************
 *   clean up of local definitions
 * ************************************************************************ */


/*
 *  undo local constants
 */

#undef FTS_C


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
