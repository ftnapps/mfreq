/* ************************************************************************
 *
 *   functions header file
 *
 *   (c) 1994-2014 by Markus Reschke
 *
 * ************************************************************************ */


/*
 *  In each source file we create a local ID definition. If the ID definition
 *  is unset we may import functions of that specific source file.
 */


/* ************************************************************************
 *   functions from log.c
 * ************************************************************************ */

#ifndef LOG_C

  extern void Log(unsigned short int Type, const char *Line, ...);

  extern void LogCfgError(void);

#endif


/* ************************************************************************
 *   functions from misc.c
 * ************************************************************************ */

#ifndef MISC_C

  extern char *CopyString(const char *Source);
  extern long Str2Long(const char *Token);
  extern _Bool Bytes2String(long long Bytes, char *Buffer, size_t Size);
  extern long long String2Bytes(char *Token);

  extern _Bool MatchPattern(char *String, char *Pattern);

  extern void UnlockFile(FILE *File);
  extern _Bool LockFile(FILE *File, char *Filepath);
  extern _Bool IsMountingPoint(char *Path);

  extern char *GetFilename(char *Filepath);

  extern unsigned short GetKeyword(char **Keywords, char *String);

#endif


/* ************************************************************************
 *   functions from tokenizer.c
 * ************************************************************************ */

#ifndef TOKENIZER_C

  extern void FreeTokenlist(Token_Type *List);

  extern Token_Type *Tokenize(char *Line);

  extern char *UnTokenize(Token_Type *List);

#endif


/* ************************************************************************
 *   functions from index.c
 * ************************************************************************ */

#ifndef INDEX_C

  extern void FreeDataList(IndexData_Type *List);
  extern _Bool AddDataElement(char *Name, char *Filepath, char *PW);

  extern void FreeLookupList(IndexLookup_Type *List);
  extern _Bool AddLookupElement(char Letter, off_t Offset,
    unsigned int Start, unsigned int Stop);

  extern void FreeAliasList(IndexAlias_Type *List);
  extern _Bool AddAliasElement(unsigned int Number, char *Path);

  extern void FreeExcludeList(Exclude_Type *List);
  extern _Bool AddExcludeElement(char *Name);
  extern _Bool MatchExcludeList(char *Name);

#endif


/* ************************************************************************
 *   functions from files.c
 * ************************************************************************ */

#ifndef FILES_C

  extern void FreeFileList(File_Type *List);
  extern _Bool AddFileElement(char *Name, time_t Time);

#endif


/* ************************************************************************
 *   functions from req.c
 * ************************************************************************ */

#ifndef REQ_C

  extern void FreeIndexList(Index_Type *List);
  extern _Bool AddIndexElement(char *Filepath, char *MountingPoint);

  extern void FreeLimitList(Limit_Type *List);
  extern _Bool AddLimitElement(char *Address, long Files, long long Bytes,
    unsigned int Flags);

  extern void FreeResponseList(Response_Type *List);
  extern Response_Type *CreateResponseElement(char *Filepath);
  extern _Bool DuplicateResponse(Response_Type *Response);

  extern void FreeRequestList(Request_Type *List);
  extern _Bool AddRequestElement(char *Name, char *Password);

#endif


/* ************************************************************************
 *   functions from fts.c
 * ************************************************************************ */

#ifndef FTS_C

  extern void FreeAKAlist(AKA_Type *List);
  extern AKA_Type *NewAKA(char *Address);
  extern _Bool MatchAKAs(AKA_Type *AKA1, AKA_Type *AKA2);

  extern _Bool WriteMailContent(FILE *File);
  extern _Bool NetMail();

#endif


/* ************************************************************************
 *   functions from list.c
 * ************************************************************************ */

#ifndef LIST_C

  extern void FreeInfoList(Info_Type *List);
  extern _Bool AddInfoElement(char *Name, off_t Size, time_t Time);
  extern Info_Type *SearchInfoList(Info_Type *List, char *Name);
  extern _Bool AddDesc2Info(Info_Type *Info, char *Data);

  extern void FreeFieldList(Field_Type *List);
  extern _Bool InsertField(Field_Type **Fields,
    unsigned short Type, unsigned short Line,
    unsigned short Pos, unsigned short Width,
    unsigned short Align, unsigned short Format);

  extern _Bool Strings2DateTime(char *Year, char *Month, char *Day,
    struct tm *DateTime);

  extern void FillString(char *String, char Char,
    unsigned short Number, unsigned short Max);
  extern long long LimitNumber(long long Number, unsigned short Digits);
  extern _Bool Bytes2StringN(long long Bytes, unsigned short Width,
    unsigned short Format, char *Buffer, size_t Size);

  extern _Bool CheckDosFilename(char* Name);

#endif


/* ************************************************************************
 *   EOF
 * ************************************************************************ */
