#ifndef DLIB_DDISKIMAGEFILE_H
#define DLIB_DDISKIMAGEFILE_H


#include "DDiskImage.h"


class DDiskImageFile : public DDiskImage
{
public:
  DDiskImageFile(const char *fname);
  virtual ~DDiskImageFile();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "generic DDiskImageFile"; }
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
  virtual void init();
  //
  virtual bool openTheFile();
  virtual bool readGeometry();
  virtual bool readDirectory();
  virtual bool readDirEntry(long offset);
  //
  FILE *theFile;
};


#endif // DLIB_DDISKIMAGEFILE_H

