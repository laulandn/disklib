#ifndef DLIB_DFATFS_H
#define DLIB_DFATFS_H


#include "DDiskImageMem.h"


#define DFAT_FORMAT_UNKNOWN   0
#define DFAT_FORMAT_RAW       1
#define DFAT_FORMAT_MFM_DISK  2


class DFatFSHeader {
public:
  //
  DFatFSHeader(unsigned char *b,UINT offset);
  //
  void debugDump();
  //
  UINT fatBytesPerSector;
  UINT fatSectorsPerCluster;
  UINT fatReservedSectors;
  UINT fatTotalFats;
  UINT fatMaxRootEntries;
  UINT fatTotalSectors;
  UINT fatMediaDescriptor;
  UINT fatSectorsPerFat;
  UINT fatNumSectors;
  UINT fatNumSides;
  UINT fatHiddenSectors;
  UINT fatTotalSectorsLarge;
  UINT fatDriveNumber;
  UINT fatFlags;
  UINT fatSignature;
};


class DFatDirEntry {
public:
  //
  DFatDirEntry(unsigned char *b,UINT offset);
  //
  void debugDump();
  //
  char name[8];
  char ext[3];
  UINT createTimeFine;
  UINT createDateCoarse;
  UINT createDate;
  UINT lastAccess;
  UINT firstCluster;
  UINT size;
};


class DFatFS : public DDiskImageMem {
public:
  DFatFS(const char *fname);
  virtual ~DFatFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DFatFS"; }
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
  DFatFSHeader *header;
  UINT imageFormat;
  UINT dirOffSector;
  UINT dirOffBytes;
  UINT numDirSectors;
  //
};


#endif

