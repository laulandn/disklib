#ifndef DLIB_DCPMFS_H
#define DLIB_DCPMFS_H


#include "DDiskImageMem.h"


// NOTE!  This includes various Spectrum, Amstrad and MSX formats (all Z80's)...


#define DCPM_FORMAT_UNKNOWN 0
#define DCPM_FORMAT_RAW     1
#define DCPM_FORMAT_EXTDSK  2
#define DCPM_FORMAT_CPCEMU  3
#define DCPM_FORMAT_IMD     4
#define DCPM_FORMAT_MSX     5
#define DCPM_FORMAT_TRS80   6
#define DCPM_FORMAT_CBM     7
#define DCPM_FORMAT_APPLE2  8
#define DCPM_FORMAT_CPM8000 9


class DCpmFS : public DDiskImageMem {
public:
  DCpmFS(const char *fname);
  virtual ~DCpmFS();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DCpmFS"; }
  virtual bool sanityCheck();
  //
  virtual bool readBlock(UINT blk);
  virtual bool writeBlock(UINT blk);
  //
  // NOTE: These next two are usually protected...
  virtual bool readDirectory();
  virtual bool readDirEntry(ULONG offset);
  //
protected:
  //
  virtual void init();
  virtual bool readBootBlock();
  virtual bool readGeometry();
  virtual bool cacheFile(void *fp);
  //
  virtual bool detectSubFormat();
  bool verifyApple2();
  //
  bool guessGeometry();
  bool deinterleave();
  bool unpackExtDsk();
  bool readExtDskHeader();
  bool unpackExtDskTrack(UINT t);
  bool unpackCpcEmu();
  bool readCpcEmuHeader();
  bool unpackCpcEmuTrack(UINT t);
  //
  unsigned char *unpackedData;
  size_t unpackedSize;
  size_t *trackDataSize;
  size_t cpcTrackSize;
  UINT imageFormat;
  UINT totalSectors;
  ULONG offset,dOffset;  // source and desk offsets for unpacking data
  UINT skippedLast;  // how many tracks we skip
  UINT dirTrack;
  UINT dirStartSect;
  UINT dirEndSect;
  UINT dirEntrySize;
  bool dirAlreadyRead;
  bool hardDisk;
};


#endif

