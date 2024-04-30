#ifndef DISK_ABASICS_DISK_H
#define DISK_ABASICS_DISK_H


// NOTE: This version of the file is not dependant on any other libs such as portable/nickcpp


// hodgepodge of useful string utilities


// Maybe I shouldn't depend on these here...
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern "C" {
#include "../../more_src/nlib/nlibdbug.h"
};


#define ATEMPSTRINGBUF_SIZE 256


////////////////////////////////////////////////////////////////////////////////
//  AError_disk Class
////////////////////////////////////////////////////////////////////////////////


// NOTE: You can't reset the flag, on purpose, because all errors are considered fatal.

class AError_disk
{
public:
  AError_disk();
  bool getError();
  void setError();
protected:
  bool errorFlag;
};


////////////////////////////////////////////////////////////////////////////////
//  ABaseClass_disk Class
////////////////////////////////////////////////////////////////////////////////


class ABaseClass_disk
{
public:
  ABaseClass_disk() { }
  virtual ~ABaseClass_disk() { }
  //
  AError_disk err;
};


// Just used to collect private classes under the same branch
class APrivateBase_disk : public ABaseClass_disk {
};


////////////////////////////////////////////////////////////////////////////////
//  AStringHelper_disk Class
////////////////////////////////////////////////////////////////////////////////


#define DSHELPER_disk aDefaultStringHelper_disk

#define DISK2HEX(a) DSHELPER_disk.myHex(a)
#define DISK2HEX2(a) DSHELPER_disk.myHex2(a)
#define DISK2HEX2NX(a) DSHELPER_disk.myHex2nox(a)
#define DISK2SHEX2(a) DSHELPER_disk.mySHex2(a)
#define DISK2HEX4(a) DSHELPER_disk.myHex4(a)
#define DISK2SHEX4(a) DSHELPER_disk.mySHex4(a)
#define DISK2HEX8(a) DSHELPER_disk.myHex8(a)
#define DISK2SHEX8(a) DSHELPER_disk.mySHex8(a)
#define DISK2HEX16(a) DSHELPER_disk.myHex16(a)
#define DISK2SHEX16(a) DSHELPER_disk.mySHex16(a)


class AStringHelper_disk
{
public:
  AStringHelper_disk();
  virtual ~AStringHelper_disk();
  //
  void fatal(const char *msg);
  //
  unsigned int getArgc() { return theArgc; }
  char *getArgv(unsigned int i) { return theArgv[i]; }
  char **getAllArgv() { return (char **)theArgv; }
  //
  bool extensionMatch(const char *filename, const char *extension);
  unsigned int countSpaces(const char *s);
  //
  // Quick convert to hex string
  char *myHex2(unsigned char val);
  char *mySHex2(unsigned char val);
  char *myHex2nox(unsigned char val);  // No prefix
  char *myHex4(unsigned short val);
  char *mySHex4(unsigned short val);
  char *myHex8(unsigned long val);
  char *mySHex8(unsigned long val);
  char *myHex16(unsigned long val);
  char *mySHex16(unsigned long val);
  char *myHexAddr(unsigned long val); // will vary in size
  char *myHexData(unsigned long val); // will vary in size
  char *myHex(unsigned long val);  // No leading zeros
  char *myDec(unsigned long val);
  //
  void setNumAddrDigits(unsigned int b) { numAddrDigits=b; }
  void setNumDataDigits(unsigned int b) { numDataDigits=b; }
  //
  // These all don't modify the source, but instead return a new modified string
  char *removeQuotes(const char *s);
  char *removeSpaces(const char *s);
  char *removeSpaces2(const char *s);
  char *flipSlashes(const char *s);
  char *toLower(const char *us);
  char *toUpper(const char *us);
  //
    //
  bool getDecMode() { return decMode; }
  void setDecMode(bool tMode=true) { decMode=tMode; }
  void setHexMode() { decMode=false; }
  //
  // Break up a string at whitespace...args are in theArgc and theArgv
  void parseString(const char *str, const char *cmd, bool ignoreSpaces=false, char treatThisAsASpace=' ');
  unsigned int parseNumber(const char *str);
  //
  // NOTE: These return false if they didn't find anything to do.
  static bool getPathFromFullName(char *dest,const char *fullname);
  static bool fillInPathFromFullName(char *dest,const char *fullname);
  static void fillInString(char *dest,const char *str);
  static bool getNameFromFullName(char *dest,const char *fullname);
  static bool fillInNameFromFullName(char *dest, const char *fullname);
  //
protected:
  // Private data members
  bool decMode;
  unsigned int theArgc;
  char *theArgv[16];
  char tempStringBuf[ATEMPSTRINGBUF_SIZE];
  char tempStringBuf2[ATEMPSTRINGBUF_SIZE];
  const char *hexPrefix;
  unsigned int numAddrDigits;
  unsigned int numDataDigits;
};


extern AStringHelper_disk DSHELPER_disk;


////////////////////////////////////////////////////////////////////////////////
//  ADataSourceAbstract_disk Class
////////////////////////////////////////////////////////////////////////////////


class ADataSourceAbstract_disk
{
public:
  ADataSourceAbstract_disk();
  ADataSourceAbstract_disk(char *name,char *table,char *host=NULL,char *user=NULL,char *password=NULL);
  virtual ~ADataSourceAbstract_disk();
  virtual void kill();
  virtual bool sort();
  virtual void jumpToHead();
  virtual void jumpToTail();
  virtual void insert(void *n);
  virtual void append(void *n);
  virtual void *remove();
  virtual void *info();
  virtual void advance();
  virtual void retreat();
  virtual bool placeBookmark();
  virtual bool gotoBookmark();
  virtual bool atEnd();
  virtual bool isEmpty();
  virtual bool isSorted();
  virtual bool isConnected();
  //
  AError_disk err;
  //
protected:
  virtual void init();
  //
  bool connected;
  char *name;
  char *table;
  char *host;
  char *user;
};


////////////////////////////////////////////////////////////////////////////////
//  AGenericListNode_disk Class
////////////////////////////////////////////////////////////////////////////////


// values returned by comparision functions
#define A_COMPARE_UNKNOWN  42
#define A_COMPARE_SAME     0
#define A_COMPARE_BEFORE   -1
#define A_COMPARE_AFTER    1


// The idea is that this is what *info points to...in
// practice this class is never used...
class AGenericListNode_disk
{
public:
  AGenericListNode_disk();
  ~AGenericListNode_disk();
  virtual int compare(AGenericListNode_disk *other);
protected:
};


////////////////////////////////////////////////////////////////////////////////
//  AListNode_disk Class
////////////////////////////////////////////////////////////////////////////////


// This is used internally by AList_disk to store nodes...
class AListNode_disk
{
friend class AList_disk;
public:
  //
  AListNode_disk();
  //
protected:
  //
  void *info;  // could be a pointer to AGenericListNode_disk or some such
  AListNode_disk *prev;
  AListNode_disk *next;
  //
};


////////////////////////////////////////////////////////////////////////////////
//  AList_disk Class
////////////////////////////////////////////////////////////////////////////////


class AList_disk : public ADataSourceAbstract_disk
{
public:
  //
  AList_disk();
  virtual ~AList_disk();
  virtual void kill();
  virtual void insert(void *n); // insert before current position
  virtual void append(void *n); // append after current position
  virtual void *remove(); // remove current position
  virtual void *info(); // info of current element
  virtual void advance(); // advance current position
  virtual void retreat(); // retreat current position
  virtual bool sort() { return false; /* list is always unsorted */ }
  virtual void jumpToHead();
  virtual void jumpToTail();
  virtual bool placeBookmark();
  virtual bool gotoBookmark();
  virtual bool atEnd(); // NOTE: true for either end of list (WHAT? NO!)
  virtual bool isEmpty() { return !head; }
  virtual bool isSorted() { return false; }
  //
protected:
  //
  virtual void init();
  //
  AListNode_disk *head;
  AListNode_disk *tail;
  AListNode_disk *cur; // current position
  AListNode_disk *bookmark;
  //
};


#endif


