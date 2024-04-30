#ifndef DLIB_DPARTITIONTABLE_H
#define DLIB_DPARTITIONTABLE_H


#include "DDiskImageMem.h"


#define DLIB_DEFAULT_BOOTBLOCK_SIZE 2048


class DPartitionTable : public DDiskImageMem
{
public:
  DPartitionTable(const char *fname);
  virtual ~DPartitionTable();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *getName() { return "DPartitionTable"; }
  virtual AList_disk *getPartitionList() { return &partitionList; }
  virtual DDiskImageMem *getDisk();
  //
protected:
  //
  virtual void init();
  //
  virtual bool readDirectory();
  virtual bool readGeometry();
  //
  AList_disk partitionList;
};


class DBootBlock
{
public:
  DBootBlock(DDisk *newDisk);
  virtual ~DBootBlock();
  //
protected:
  //
  virtual void init();
  //
  DDisk *theDisk;
  //
};


#endif // DLIB_DPARTITIONTABLE_H

