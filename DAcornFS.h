#ifndef DLIB_DACORNFS_H
#define DLIB_DACORNFS_H


#include "DDiskImageMem.h"


#define DACORN_FORMAT_UNKNOWN 0
#define DACORN_FORMAT_RAW     1
#define DACORN_FORMAT_ACORN   2


class DAcornFS : public DDiskImageMem {
public:
  DAcornFS(const char *name);
  virtual ~DAcornFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DAcornFS"; }
  virtual bool sanityCheck();
  //
  static UINT checkFilesystem(unsigned char *tbuf,FILE *f,UINT fsize);
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
  virtual bool readDirEntry(long offset);
  //
  bool adfs;
};


#endif

