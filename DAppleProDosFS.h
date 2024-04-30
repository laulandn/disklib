#ifndef DLIB_DAPPLEPRODOSFS_H
#define DLIB_DAPPLEPRODOSFS_H


#include "DApple2FS.h"


class DAppleProDosFS : public DApple2FS {
public:
  DAppleProDosFS(const char *fname);
  virtual ~DAppleProDosFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DAppleProDosFS"; }
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
  virtual bool readAppleProDosEntry(long offset);
  //
};


#endif
