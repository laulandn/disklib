#ifndef DLIB_DATARIFS_H
#define DLIB_DATARIFS_H


#include "DDiskImageMem.h"


#define DATARI_FORMAT_UNKNOWN 0
#define DATARI_FORMAT_RAW     1
#define DATARI_FORMAT_ATARI   2


class DAtariFS : public DDiskImageMem {
public:
  DAtariFS(const char *fname);
  virtual ~DAtariFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DAtariFS"; }
  virtual bool sanityCheck();
  //
  virtual bool readBlock(UINT blk);
  virtual bool writeBlock(UINT blk);
  //
protected:
  //
  virtual bool detectSubFormat();
  //
  virtual void init();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool readDirectory();
  virtual bool cacheFile(void *fp);
  //
  virtual bool readDirEntry(long offset);
  //
  bool atrHeader;
  uint8_t vtoc[128];  // NOTE: Row Block...and this is 128 bytes on single density...larger for double+ maybe?
};


#endif

