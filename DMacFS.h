#ifndef DLIB_DMACFS_H
#define DLIB_DMACFS_H


#include "DDiskImageMem.h"


#define DMAC_FORMAT_UNKNOWN   0
#define DMAC_FORMAT_RAW       1
#define DMAC_FORMAT_MFS_DISK  2
#define DMAC_FORMAT_HFS_DISK  3


class DMacFS : public DDiskImageMem {
public:
  DMacFS(const char *fname);
  virtual ~DMacFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DMacFS"; }
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
  bool diskCopy;
};


#endif


