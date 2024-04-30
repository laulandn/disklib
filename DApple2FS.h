#ifndef DLIB_DAPPLE2FS_H
#define DLIB_DAPPLE2FS_H


// FIXME: Could be made into a plug-in


#include "DDiskImageMem.h"

#include "DCpmFS.h"


#define DAPPLE2_FORMAT_UNKNOWN  0
#define DAPPLE2_FORMAT_RAW      1
#define DAPPLE2_FORMAT_NIB      2
#define DAPPLE2_FORMAT_WOZ      3
#define DAPPLE2_FORMAT_2MG      4


#define EDISK2_NUM_TRACKS  35
#define EDISK2_NUM_SECTORS  16
#define EDISK2_ENCODED_SECTOR_SIZE  416
#define EDISK2_DECODED_SECTOR_SIZE  256
#define EDISK2_NIB_TRACK_SIZE (EDISK2_NUM_SECTORS*EDISK2_ENCODED_SECTOR_SIZE)
#define EDISK2_TRACK_SIZE (EDISK2_NUM_SECTORS*EDISK2_DECODED_SECTOR_SIZE)
#define EDISK2_INTERLEAVE0 0
#define EDISK2_INTERLEAVE1 1
#define EDISK2_INTERLEAVE2 2


class DApple2FSNibbleHelper
{
public:
  static bool encodeNibs(UINT tnum,uint8_t *src,UINT ssize,uint8_t *dest,UINT dsize);
  static bool decodeNibs(UINT tnum,uint8_t *src,UINT ssize,uint8_t *dest,UINT dsize);
  static bool encodeNibSector(UINT tnum,UINT snum,uint8_t *src,uint8_t *dest);
  static bool decodeNibSector(UINT tnum,UINT snum,uint8_t *src,uint8_t *dest);
protected:
};


class DApple2FS : public DDiskImageMem {
public:
  DApple2FS(const char *fname);
  virtual ~DApple2FS();
  //
protected:
  //
  virtual void init();
  virtual bool detectCpm();
  virtual bool readDirectoryCpm();
  virtual bool readDirEntryCpm(long offset);
  //
  bool nibble;
  bool cpm;
  bool twoMG;
  UINT interleave;
  DApple2FSNibbleHelper nibbleHelper;
  DCpmFS *cpmDisk;  // Only if it is really cpm
};



class DApplePascalFS : public DApple2FS {
public:
  DApplePascalFS(const char *fname);
  virtual ~DApplePascalFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DApplePascalFS"; }
  virtual bool sanityCheck();
  //
protected:
  //
  virtual void init();
  //
  virtual bool detectSubFormat();
  //
  virtual bool readDirectory();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool cacheFile(void *fp);
  //
  virtual bool readApplePascalEntry(long offset);
  //
};


class DAppleCpmFS : public DApple2FS {
public:
  DAppleCpmFS(const char *fname);
  virtual ~DAppleCpmFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DApplePascalFS"; }
  virtual bool sanityCheck();
  //
protected:
  //
  virtual void init();
  //
  virtual bool detectSubFormat();
  //
  virtual bool readDirectory();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool cacheFile(void *fp);
  //
  virtual bool readAppleCpmEntry(long offset);
  //
};


#endif

