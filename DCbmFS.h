#ifndef DLIB_DCBMFS_H
#define DLIB_DCBMFS_H


// FIXME: Could be made into a plug-in


#include "DDiskImageMem.h"
#include "DCpmFS.h"


#define DCBM_FORMAT_UNKNOWN 0
#define DCBM_FORMAT_1541 1
#define DCBM_FORMAT_PET1 2
#define DCBM_FORMAT_1581 3
#define DCBM_FORMAT_1571 4
#define DCBM_FORMAT_PET2 5


class DCbmFS : public DDiskImageMem {
public:
  DCbmFS(const char *fname);
  virtual ~DCbmFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DCbmFS"; }
  virtual bool sanityCheck();
  //
  //virtual size_t Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  //
protected:
  //
  virtual void init();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool readGeometryMore();
  virtual bool readDirectory();
  virtual bool readDirectoryCpm();
  virtual bool cacheFile(void *fp);
  //
  virtual bool detectSubFormat();
  virtual bool detectCpm();
  //
  virtual bool readDirEntry(long offset);
  virtual bool readDirEntryCpm(long offset);
  //
  char id1,id2,dos1,dos2;
  UINT blocksFree;
  UINT cbmDirTrack;
  bool geos;
  bool cpm;
  bool gcr; // raw
  DCpmFS *cpmDisk;  // Only if it is really cpm
  //
};


#endif

