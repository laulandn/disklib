#ifndef DLIB_DDIRENTRY_H
#define DLIB_DDIRENTRY_H


#include "Dabasics.h"


class DDisk;
class DDiskImage;


// for DDirEntry.type
// (Not all of these will be used)
#define DDISK_TYPE_NONE   0
#define DDISK_TYPE_DIR    1
#define DDISK_TYPE_DEVICE 2
#define DDISK_TYPE_BINARY 3
#define DDISK_TYPE_BASIC  4
#define DDISK_TYPE_TEXT   5
#define DDISK_TYPE_FS     6


// See alib/filedefs.h
// Max filename len we'll handle
#define DDISK_MAX_NAMELEN 1024
// Max pathname len we'll handle
#define DDISK_MAX_PATHLEN 1024


// NOTE: Any of these fields may be unused in any particular format
class DDirEntry : public AGenericListNode_disk
{
public:
  DDirEntry();
  virtual ~DDirEntry();
  //
  virtual void debugDump();
  //
  virtual bool checkDiskSanity(DDiskImage *theDisk);
  virtual bool sanityCheck();
  //
  virtual void setPrivateVal1(long val) { privateVal1=val; }
  virtual void setPrivateVal2(long val) { privateVal2=val; }
  //
  // NOTE All the below members SHOULD BE PROTECTED!
  UINT signature;  // MUST be 0xface to be valid
  char name[DDISK_MAX_NAMELEN];
  UINT nativeType;  // defined by the disk type
  UINT type; // as above...may not fit exactly...
  size_t sizeBlocks;  // In blocks (whatever size they may be)
  size_t size;  // In bytes (actual size or sizeBlocks*blockSize)
  UINT track,sector;  // If known and makes sense...
  UINT block;  // If known and makes sense...
  char *data;  // File buffer, if cached (May be larger than actual)
  long curSeek;
  bool locked;
  //
protected:
  //
  virtual bool basicSanityCheck();
  //
  // Implementation dependent, or unused
  long privateVal1,privateVal2;
};


#endif

