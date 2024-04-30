
#define DEBUG_OUT dBug
#define ERR_OUT std::cerr
//#define DEBUG_OUT *aNullStream
#define CONS_OUT std::cout
#define CONS_IN std::cin


// NOTE: This version of the file is not dependant on any other libs such as portable/nickcpp


#include <stdio.h>
#include <stdlib.h>


#include "Dabasics.h"


////////////////////////////////////////////////////////////////////////////////
//  AError_disk Class
////////////////////////////////////////////////////////////////////////////////

AError_disk::AError_disk()
{
  errorFlag=false;
}


bool AError_disk::getError()
{
  return errorFlag;
}


void AError_disk::setError()
{
  errorFlag=true;
  //exit(EXIT_FAILURE);
}


////////////////////////////////////////////////////////////////////////////////
//  ADataSourceAbstract_disk Class
////////////////////////////////////////////////////////////////////////////////

ADataSourceAbstract_disk::ADataSourceAbstract_disk()
{
  init();
}


ADataSourceAbstract_disk::ADataSourceAbstract_disk(char *tname,char *ttable,char *thost,
  char *tuser,char *tpass)
{
  init();
  connected=false;
  name=tname;
  host=thost;
  user=tuser;
  table=ttable;
}


void ADataSourceAbstract_disk::init()
{
  connected=true;
  name=NULL;
  host=NULL;
  user=NULL;
  table=NULL;
}


ADataSourceAbstract_disk::~ADataSourceAbstract_disk()
{
  // Nothing to do
}


void ADataSourceAbstract_disk::kill()
{
  // Nothing to do
}


void ADataSourceAbstract_disk::insert(void *n)
{
  // Nothing to do
}


void ADataSourceAbstract_disk::append(void *n)
{
  // Nothing to do
}


void* ADataSourceAbstract_disk::remove()
{
  // Because there's nothing there...
  return NULL;
}


void* ADataSourceAbstract_disk::info()
{
  // Because there's nothing there...
  return NULL;
}


void ADataSourceAbstract_disk::advance()
{
  // Nothing to do
}


void ADataSourceAbstract_disk::retreat()
{
  // Nothing to do
}


void ADataSourceAbstract_disk::jumpToHead()
{
  // Nothing to do
}


void ADataSourceAbstract_disk::jumpToTail()
{
  // Nothing to do
}


bool ADataSourceAbstract_disk::sort()
{
  // Nothing to do
  return false;
}


bool ADataSourceAbstract_disk::atEnd()
{
  // Because there's nothing there...
  return true;
}


bool ADataSourceAbstract_disk::isEmpty()
{
  // Because there's nothing there...
  return true;
}


bool ADataSourceAbstract_disk::isSorted()
{
  // Because there's nothing there...
  return true;
}


bool ADataSourceAbstract_disk::placeBookmark()
{
  // Because there's nothing there...
  return false;
}


bool ADataSourceAbstract_disk::gotoBookmark()
{
  // Because there's nothing there...
  return false;
}


bool ADataSourceAbstract_disk::isConnected()
{
  // Because we're always "connected"...and always local...by default.
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//  AGenericListNode_disk Class
////////////////////////////////////////////////////////////////////////////////

AGenericListNode_disk::AGenericListNode_disk()
{
  // child classes might do something here...
}


AGenericListNode_disk::~AGenericListNode_disk()
{
  // child classes might do something here...
}


int AGenericListNode_disk::compare(AGenericListNode_disk *other)
{
  return A_COMPARE_UNKNOWN;
}


////////////////////////////////////////////////////////////////////////////////
//  AListNode_disk Class
////////////////////////////////////////////////////////////////////////////////

AListNode_disk::AListNode_disk()
{
  prev=(AListNode_disk *)NULL;
  next=(AListNode_disk *)NULL;
  info=(void *)NULL;
}


////////////////////////////////////////////////////////////////////////////////
//  AList_disk Class
////////////////////////////////////////////////////////////////////////////////

AList_disk::AList_disk()
{
  init();
}


void AList_disk::init()
{
  ADataSourceAbstract_disk::init();
  head=(AListNode_disk *)NULL;
  tail=(AListNode_disk *)NULL;
  cur=(AListNode_disk *)NULL;
  bookmark=(AListNode_disk *)NULL;
}


AList_disk::~AList_disk()
{
  kill();
}


void AList_disk::kill()
{
  if(head) {
  AListNode_disk *p=head;
    while(p) {
      AListNode_disk *q=p;
      if(p) p=p->next;
      if(q) delete q;
    }
  }
  head=(AListNode_disk *)NULL;
  tail=(AListNode_disk *)NULL;
  cur=(AListNode_disk *)NULL;
}


void AList_disk::insert( void *n )
{
  AListNode_disk *p=new AListNode_disk;
  if(!p) return;
  p->info=n;
  p->next=cur;
  if(cur) p->prev=cur->prev;
  if(p->next) p->next->prev=p;
  if(p->prev) p->prev->next=p;
  cur=p;
  if(!p->prev) head=p;
  if(!p->next) tail=p;
}


void AList_disk::append(void *n)
{
  if(!cur) cur=tail;
  AListNode_disk *p=new AListNode_disk;
  if(!p) return;
  p->info=n;
  p->prev=cur;
  if(cur) p->next=cur->next;
  if(p->next) p->next->prev=p;
  if(p->prev) p->prev->next=p;
  cur=p;
  if(!p->prev) head=p;
  if(!p->next) tail=p;
}


// FIXME: I'm still a little iffsety on this one...
void *AList_disk::remove()
{
  if(!head) return NULL;
  if(!tail) return NULL;
  if(!cur) cur=tail;
  AListNode_disk *p=cur;
  void *r=cur->info;
  if(p->next) p->next->prev=p->prev;
  if(p->prev) p->prev->next=p->next;
  if(p->next) cur=p->next;
  else cur=p->prev;
  //if(!head->prev) head=p->next;
  //if(!tail->next) tail=p->prev;
  if(!p->next) tail=p->prev;
  if(!p->prev) head=p->next;
  if(!cur) head=(AListNode_disk *)NULL;
  if(!cur) tail=(AListNode_disk *)NULL;
  delete p;
  return r;
}


void *AList_disk::info()
{
  void *ret=NULL;
  if(cur) ret=cur->info;
  return ret;
}


void AList_disk::advance()
{
  if(cur) {
     cur=cur->next;
  }
}


void AList_disk::retreat()
{
  if(cur) {
     cur=cur->prev;
  }
}


void AList_disk::jumpToHead()
{
  cur=head;
}


void AList_disk::jumpToTail()
{
  cur=tail;
}


bool AList_disk::atEnd()
{
  bool ret=false;
  if(!cur) {
    ret=true;
  }
  return ret;
}


bool AList_disk::placeBookmark()
{
  bookmark=cur;
  return true;
}


bool AList_disk::gotoBookmark()
{
  cur=bookmark;
  return true;
}


AStringHelper_disk DSHELPER_disk;


////////////////////////////////////////////////////////////////////////////////
//  AStringHelper_disk Class
////////////////////////////////////////////////////////////////////////////////


AStringHelper_disk::AStringHelper_disk()
{
  decMode=true;
  //hexPrefix="$";
  //hexPrefix="0x";
  hexPrefix="";
  // Assume 32 bits for now...
  numAddrDigits=8;
  numDataDigits=8;
  theArgc=0;
  for(unsigned int t=0;t<16;t++) theArgv[t]=NULL;
}


AStringHelper_disk::~AStringHelper_disk()
{
  //debug_msg("AStringHelper_disk::~AStringHelper_disk()\n");
}


void AStringHelper_disk::fatal(const char *msg)
{
  debug_msg(msg); debug_msg("\n");
  exit(EXIT_FAILURE);
}


unsigned int AStringHelper_disk::countSpaces(const char *s)
{
  // TODO: Count contiguous spaces as a single space...?
  unsigned int ret=0;
  unsigned long l=strlen(s);
  for(unsigned int t=0;t<l;t++)
    if(s[t]==' ') ret++;
  return ret;
}


char *AStringHelper_disk::removeQuotes(const char *s)
{
  if(!s) {
    tempStringBuf[0]=0;
    return tempStringBuf;
  }
  unsigned long len=strlen(s);
  unsigned int t=0,tt=0;
  while(t<len) {
    if(s[t]!='\"') {
      tempStringBuf[tt]=s[t];
      tt++;
    }
    t++;
  }
  return tempStringBuf;
}


char *AStringHelper_disk::flipSlashes(const char *s)
{
  if(!s) {
    tempStringBuf[0]=0;
    return tempStringBuf;
  }
  strcpy(tempStringBuf,s);
  unsigned long len=strlen(tempStringBuf);
  for(unsigned int t=0;t<len;t++) {
    char c=tempStringBuf[t];
    if(c=='\\') tempStringBuf[t]='/';
    if(c=='/') tempStringBuf[t]='\\';
  }
  return tempStringBuf;
}


bool AStringHelper_disk::extensionMatch(const char *filename, const char *extension)
{
  bool ret=false;
  unsigned long l=strlen(filename);
  unsigned long e=strlen(extension);
#ifdef WIN32
  if(!_strnicmp(filename+l-e,extension,e)) ret=true;
#else
  if(!strncasecmp(filename+l-e,extension,e)) ret=true;
#endif
  return ret;
}


// NOTE: This is designed to parse a single line (only one newline)
// TODO: Handle escapes
void AStringHelper_disk::parseString(const char *str, const char *cmd, bool ignoreSpaces,char treatThisAsASpace)
{
  // States:
  // false - base
  // true - in quote, waiting for close quote
  bool state=false;
  unsigned int t=0;
  theArgc=0;
  for(t=0;t<16;t++) theArgv[t]=NULL;
  //debug_msg("AStringHelper_disk::parseString()...\n");
  unsigned int n=0;
  if(cmd) { n++; theArgv[0]=(char *)cmd; theArgc=1; }
  if(!str) return;
  unsigned long arglen=strlen(str);
  if(!arglen) return;
  char *src=strdup(str);
  //debug_msg("AStringHelper_disk::parseString ("); debug_msg(arglen); debug_msg(" chars incl nl): '"); debug_msg(src); debug_msg("'\n");
  for(t=0;t<arglen;t++) {
    if(src[t]=='"') {
      //debug_msg("double quote!\n");
      state=!state;
      src[t]=0;
    }
    if((src[t]==treatThisAsASpace)&&treatThisAsASpace) {
      //debug_msg("treatThisAsASpace!\n");
      src[t]=0;
    }
    if(src[t]=='\t') {
      //debug_msg("tab!\n");
      src[t]=0;
    }
    if(src[t]==10) {
      //debug_msg("10!\n");
      src[t]=0;
    }
    if(src[t]==13) {
      //debug_msg("13!\n");
      src[t]=0;
    }
    if(src[t]==' ') {
      //debug_msg("space!\n");
      if(!state) {
        //debug_msg("!state!\n");
        if(!ignoreSpaces) {
          //debug_msg("!ignoreSpaces!\n");
          src[t]=0;
        }
        //else debug_msg("just ignored a space!\n");
      }
    }
  }
  if(strlen(src)) theArgv[n]=src; else n--;
  for(t=0;t<arglen;t++) {
    if(!src[t]) {
      //debug_msg(strlen(src+t+1)); debug_msg(" "); debug_msg(((char *)src+t+1)); debug_msg("\n");
      if(strlen(src+t+1)) { n++; theArgv[n]=src+t+1; }
    }
  }
  theArgc=n+1;
  //debug_msg("theArgc is "); debug_msg(theArgc); debug_msg("\n");
  for(t=0;t<theArgc;t++) {
    if(theArgv[t]) {
      //debug_msg(t); debug_msg(": '"); debug_msg(theArgv[t]); debug_msg("' ("); debug_msg(((long)strlen(theArgv[t]))); debug_msg(" chars)\n");
    }
  }
}


unsigned int AStringHelper_disk::parseNumber(const char *str)
{
  if(!str) return 0;
  //if(str[strlen(str)-1]=='h') str[strlen(str)-1]=0;
  unsigned int val=0,offset=0;
  char *format=NULL;
  bool tDecMode=decMode;
  if(str[0]=='#') { tDecMode=true;  offset=1; }
  if(str[0]=='$') { tDecMode=false;  offset=1; }
  if((str[0]=='0')&&(str[1]=='x')) { tDecMode=false;  offset=2; }
#ifdef DEBUG_VERBOSE
  debug_msg("Parsing number '"); debug_msg(str); debug_msg("' (tDecMode="); debug_int((int)tDecMode); debug_msg(") ");
#endif
  if(tDecMode) format=(char *)"%d"; else format=(char *)"%x";
  sscanf(str+offset,format,&val);
#ifdef DEBUG_VERBOSE
  debug_msg("is "); debug_int(val); debug_msg("\n");
#endif
  return val;
}


char *AStringHelper_disk::toLower(const char *us)
{
  if(!us) {
    tempStringBuf[0]=0;
    return tempStringBuf;
  }
  strcpy(tempStringBuf,us);
  unsigned long l=strlen(tempStringBuf);
  for(unsigned int t=0;t<l;t++) {
    if((tempStringBuf[t]>='A')&&(tempStringBuf[t]<='Z'))
      tempStringBuf[t]=(char)(tempStringBuf[t]+('a'-'A'));
  }
  return tempStringBuf;
}


char *AStringHelper_disk::toUpper(const char *ls)
{
  if(!ls) {
    tempStringBuf[0]=0;
    return tempStringBuf;
  }
  strcpy(tempStringBuf,ls);
  unsigned long l=strlen(tempStringBuf);
  for(unsigned int t=0;t<l;t++) {
    if((tempStringBuf[t]>='a')&&(tempStringBuf[t]<='z'))
      tempStringBuf[t]=(char)(tempStringBuf[t]-('a'-'A'));
  }
  return tempStringBuf;
}


char *AStringHelper_disk::removeSpaces(const char *s)
{
  if(!s) {
    tempStringBuf[0]=0;
    return tempStringBuf;
  }
  unsigned long l=strlen(s),i=0;
  for(unsigned int t=0;t<l;t++) {
    if(s[t]!=' ') {
      tempStringBuf[i]=s[t];  i++;
    }
  }
  tempStringBuf[i]=0;
  return tempStringBuf;
}


char *AStringHelper_disk::removeSpaces2(const char *s)
{
  if(!s) {
    tempStringBuf2[0]=0;
    return tempStringBuf2;
  }
  unsigned long l=strlen(s),i=0;
  for(unsigned int t=0;t<l;t++) {
    if(s[t]!=' ') {
      tempStringBuf2[i]=s[t];  i++;
    }
  }
  tempStringBuf2[i]=0;
  return tempStringBuf2;
}


/* STATIC */
bool AStringHelper_disk::getPathFromFullName(char *dest,const char *fullname)
{
  dest[0]=0;
  return fillInPathFromFullName(dest,fullname);
}


/* STATIC */
bool AStringHelper_disk::fillInPathFromFullName(char *dest,const char *fullname)
{
  if(!fullname) return false;
  if(!dest) return false;
  unsigned long slen=strlen(fullname);
  unsigned int n=0,t=0;
  for(t=0;t<slen;t++) {
    //debug_msg("Looking at :"); debug_msg(fullname[t]); debug_msg("\n");
    if(fullname[t]=='=') n=t; // hehe...
    if(fullname[t]==':') n=t;
    if(fullname[t]=='/') n=t;
    if(fullname[t]=='\\') n=t;
  }
  // Don't do anything if there isn't a leading path in the pathname...
  if(!n) return false;
  //debug_msg("Looks like the offset is "); debug_msg(n); debug_msg("\n");
  unsigned long dlen=strlen(dest);
  for(t=0;t<n;t++) dest[dlen+t]=fullname[t];
  dest[n+dlen]=0;
  /*
  char tc=fullname[n+1];
  fullname[n+1]=0;
  char *tname=fullname;
  //debug_msg("tname is '"); debug_msg(tname); debug_msg("'\n");
  fillInString(dest,tname);
  fullname[n+1]=tc;
  */
  return true;
}


/* STATIC */
void AStringHelper_disk::fillInString(char *dest,const char *str)
{
  if(!str) return;
  if(!dest) return;
  unsigned long dlen=strlen(dest);
  unsigned long slen=strlen(str);
  //debug_msg("Len of src is "); debug_msg(slen); debug_msg(", dest is "); debug_msg(dlen); debug_msg("\n");
  for(unsigned int t=0;t<slen;t++) dest[dlen+t]=str[t];
  dest[slen+dlen]=0;
}


/* STATIC */
bool AStringHelper_disk::getNameFromFullName(char *dest,const char *fullname)
{
  if(!fullname) return false;
  if(!dest) return false;
  dest[0]=0;
  return fillInNameFromFullName(dest,fullname);
}


/* STATIC */
bool AStringHelper_disk::fillInNameFromFullName(char *dest,const char *fullname)
{
  if(!fullname) return false;
  if(!dest) return false;
  unsigned long slen=strlen(fullname);
  unsigned int n=0;
  for(unsigned int t=0;t<slen;t++) {
    //debug_msg("Looking at :"); debug_msg(fullname[t]); debug_msg("\n");
    if(fullname[t]=='=') n=t; // hehe...
    if(fullname[t]==':') n=t;
    if(fullname[t]=='/') n=t;
    if(fullname[t]=='\\') n=t;
  }
  //debug_msg("Looks like the offset is "); debug_msg(n); debug_msg("\n");
  unsigned int p=1;
  // If there isn't a leading path in the pathname don't skip anything...
  if(!n) p=0;
  const char *tname=fullname+n+p;
  //debug_msg("tname is '"); debug_msg(tname); debug_msg("'\n");
  fillInString(dest,tname);
  return true;
}


char *AStringHelper_disk::myDec(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%ld",val);
  return tempStringBuf;
}


// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%lx",hexPrefix,val);
  return tempStringBuf;
}


// NOTE: number of digits (bits) will vary
// This is for converting an address to hex text
char *AStringHelper_disk::myHexAddr(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  switch(numAddrDigits)
  {
    case 4: sprintf(tempStringBuf,"%s%04lx",hexPrefix,val); break;
    case 8: sprintf(tempStringBuf,"%s%08lx",hexPrefix,val); break;
    case 16: sprintf(tempStringBuf,"%s%016lx",hexPrefix,val); break;
    default: sprintf(tempStringBuf,"%s%08lx",hexPrefix,val); break;
  }
  return tempStringBuf;
}


// NOTE: number of digits (bits) will vary
// This is for converting a data to hex text
char *AStringHelper_disk::myHexData(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  switch(numDataDigits)
  {
    case 4: sprintf(tempStringBuf,"%s%04lx",hexPrefix,val); break;
    case 8: sprintf(tempStringBuf,"%s%08lx",hexPrefix,val); break;
    case 16: sprintf(tempStringBuf,"%s%016lx",hexPrefix,val); break;
    default: sprintf(tempStringBuf,"%s%08lx",hexPrefix,val); break;
  }
  return tempStringBuf;
}


// NOTE: unsigned 64 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex16(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%016lx",hexPrefix,val);
  return tempStringBuf;
}


// TODO: Sign is not actually implemented yet!!!
// NOTE: signed 64 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::mySHex16(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%016lx",hexPrefix,val);
  return tempStringBuf;
}


// NOTE: unsigned 32 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex8(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%08lx",hexPrefix,val);
  return tempStringBuf;
}


// TODO: Sign is not actually implemented yet!!!
// NOTE: signed 32 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::mySHex8(unsigned long val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%08lx",hexPrefix,val);
  return tempStringBuf;
}


// NOTE: unsigned 16 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex4(unsigned short val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%04x",hexPrefix,val);
  return tempStringBuf;
}


// TODO: Sign is not actually implemented yet!!!
// NOTE: signed 16 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::mySHex4(unsigned short val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%04x",hexPrefix,val);
  return tempStringBuf;
}


// NOTE: unsigned 8 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex2(unsigned char val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%02x",hexPrefix,val);
  return tempStringBuf;
}


// TODO: Sign is not actually implemented yet!!!
// NOTE: signed 8 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::mySHex2(unsigned char val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%s%02x",hexPrefix,val);
  return tempStringBuf;
}


// NOTE: unsigned 8 bits
// This is for converting an arbitrary pointer to a hex string...
char *AStringHelper_disk::myHex2nox(unsigned char val)
{
  // NOTE: Since this is using a common buffseter, cache results
  // if you're going to use this multiple times with stdio before a flush!
  tempStringBuf[0]=0;
  sprintf(tempStringBuf,"%02x",val);
  return tempStringBuf;
}


#ifdef UNICODE
wchar_t *AStringHelper_disk::toWide(char *ss,unsigned int offset)
{
  if(!ss) {
    unicodew[0]=0;  unicodew[1]=0;
    return unicodew;
  }
  unsigned int len=strlen(ss);
  for(unsigned int t=0;t<len;t++) {
    unicodew[offset+t]=ss[t];
  }
  unicodew[offset+len]=0;
  return unicodew+offset;
}
#endif // UNICODE


#ifdef UNICODE
char *AStringHelper_disk::toNarrow(wchar_t *ws,unsigned int offset)
{
  if(!ws) {
    unicodes[0]=0;
    return unicodes;
  }
  unsigned int len=wcslen(ws);
  for(unsigned int t=0;t<len;t++) {
    unicodes[offset+t]=(char)ws[t];
  }
  unicodes[offset+len]=0;
  return unicodes+offset;
}
#endif // UNICODE


