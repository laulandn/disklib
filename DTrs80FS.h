#ifndef DLIB_DTRS80FS_H
#define DLIB_DTRS80FS_H


#include "DDiskImageMem.h"


#define DTRS80_FORMAT_UNKNOWN 0
#define DTRS80_FORMAT_RAW     1
#define DTRS80_FORMAT_TRS80   2


class DTrs80FS : public DDiskImageMem {
public:
  DTrs80FS(const char *name);
  virtual ~DTrs80FS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DTrs80FS"; }
  virtual bool sanityCheck();
  //
protected:
  //
  virtual bool detectSubFormat();
  //
  virtual void init();
  virtual bool readDirectory();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool cacheFile(void *fp);
  //
  virtual bool readDirEntry(long offset);
  //
  bool dmkImage;
  uint8_t gat[256];  // NOTE: Raw block
  uint8_t hit[256];  // NOTE: Raw block
};


#endif

