#ifndef DLIB_DAPPLEDOS3FS_H
#define DLIB_DAPPLEDOS3FS_H


#include "DApple2FS.h"


class DAppleDos3FS : public DApple2FS {
public:
  DAppleDos3FS(const char *fname);
  virtual ~DAppleDos3FS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DAppleDos3FS"; }
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
  virtual bool readAppleDos3Entry(long offset);
  //
  bool thirteenSector;
};


#endif
