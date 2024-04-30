#ifndef DLIB_DDISKIMAGEMEM_H
#define DLIB_DDISKIMAGEMEM_H


#include "DDiskImage.h"


class DDiskImageMem : public DDiskImage
{
public:
  DDiskImageMem(const char *fname);
  DDiskImageMem(unsigned char *b,UINT offset);
  virtual ~DDiskImageMem();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "generic DDiskImageMem"; }
  //
  // NOTE: State of unmounted disk is undefined, don't try and do anything with it!
  virtual bool Mount(const char *fname);
  virtual bool Unmount();
  //
  virtual bool readBlock(UINT blk);
  //
  virtual void debugDump();
  //
protected:
  //
  virtual void init();
  //
  virtual bool readGeometry();
  virtual bool readDirectory();
  virtual bool readDirEntry(long offset);
  //
  bool handleEmuFormat();
  static unsigned char *recognizeHelper(const char *fname);
  //
  unsigned char *imageData;
};


#endif // DLIB_DDISKIMAGEMEM_H

