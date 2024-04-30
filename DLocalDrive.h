#ifndef DLIB_DHARDDISK_H
#define DLIB_DHARDDISK_H


#include "DDisk.h"

#include "DFinderDatEntry.h"


class DLocalDrive : public DDisk
{
public:
  DLocalDrive(const char *fname);
  virtual ~DLocalDrive();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "DLocalDrive"; }
  virtual const char *getDiskName();
  //
  virtual void *Fopen(const char *filename,const char *mode);
  virtual UINT Fclose(void *fp);
  virtual size_t Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual size_t Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual int Fseek(void *fp,long offset,int origin);
  virtual long Ftell(void *fp);
  virtual int Fstat(int fd,struct stat *st);
  virtual int Stat(const char *filename,struct stat *st);
  virtual void *Open(const char *filename,UINT mode);
  virtual UINT Close(void *fp);
  virtual size_t Read(void *fp,uint8_t *ptr,size_t n);
  virtual size_t Write(void *fp,uint8_t *ptr,size_t n);
  virtual int Lseek(void *fp,long offset,int origin);
  virtual bool myCopyFile(const char *oldname,const char *newname);
  virtual int Rename(const char *oldname,const char *newname);
  virtual bool Unlink(const char *filename);
  virtual bool Chdir(const char *dname);
  virtual const char *Getcwd(char *buf,size_t len);
  //
  virtual const char *getShortName(const char *longName);
  //
protected:
  //
  virtual void init();
  virtual bool readDirectory();
  //
#ifdef USE_FINDER_DAT
  void readFinderDat(FILE *f);
  //
  bool gotFinderDat;
  AList_disk finderDat;
#endif // USE_FINDER_DAT
};


#endif // DLIB_DHARDDISK_H

