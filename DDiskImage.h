#ifndef DLIB_DDISKIMAGE_H
#define DLIB_DDISKIMAGE_H


#include "DDisk.h"


// Some useful macros for reading the disk buffer...
// TODO: These may be implemented as functions at some point...
#define alibReadBuf(x) (buf[x])
#define alibReadBuf16l(x) (buf[x]+(buf[x+1]*256))
#define alibReadBuf16b(x) ((buf[x]*256)+buf[x+1])
#define alibReadBuf32l(x) (alibReadBuf16l(x)+(alibReadBuf16l(x+2)*65536))
#define alibReadBuf32b(x) ((alibReadBuf16b(x)*65536)+alibReadBuf16b(x+2))


class DDiskImage : public DDisk
{
public:
  DDiskImage(const char *fname);
  virtual ~DDiskImage();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "generic DDiskImage"; }
  //
  // NOTE: State of unmounted disk is undefined, don't try and do anything with it!
  virtual bool Mount(const char *fname);
  virtual bool Unmount();
  //
  // These should be (almost) exact copies of the real functions
  virtual void *Fopen(const char *filename,const char *mode);  // return value is opaque!
  virtual UINT Fclose(void *fp);
  virtual size_t Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual size_t Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual int Ferror(void *fp);
  virtual int Feof(void *fp);
  virtual int Fseek(void *fp,long offset,int origin);
  virtual long Ftell(void *fp);
  virtual int Fstat(int fd,struct awstat_struct *st);
  //
  //virtual int Stat(const char *filename,struct awstat_struct *st);
  //virtual void *Open(const char *filename,UINT mode);  // return value is opaque!
  //virtual UINT Close(void *fp);
  //virtual size_t n Read(void *fp,uint8_t *ptr,size_t n n);
  //virtual size_t n Write(void *fp,uint8_t *ptr,size_t n n);
  //virtual int Lseek(void *fp,long offset,int origin);
  //
  //virtual bool Unlink(const char *filename);
  //virtual bool Chdir(const char *dname);
  //virtual const char *Getcwd(char *buf,size_t len);
  //
  virtual bool readTrackSector(UINT tt,UINT ss);
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
  bool diskMounted;
  long skipOffset; // Number of bytes (probably a header) to skip
  size_t imageDataSize;
  const char *theFileName;
  //
  UINT *trackOffset;  // An array, for each track, the offset in bytes to its start in the image.
  UINT *numSectors;  // An array, for each track, the number of sectors. (for formats that vary)
};


#endif // DLIB_DDISKIMAGE_H

