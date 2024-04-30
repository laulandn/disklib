#ifndef DLIB_DPARTITIONTABLEMBR_H
#define DLIB_DPARTITIONTABLEMBR_H


#include "DPartitionTable.h"


class DPartitionMBR {
public:
  //
  DPartitionMBR(unsigned char *b,UINT offset);
  //
  bool createDiskFromPart(unsigned char *b,UINT offset);
  //
  void debugDump();
  //
  UINT bootFlag;
  UINT cFirst,hFirst,sFirst;
  UINT partType;
  UINT cLast,hLast,sLast;
  UINT lbaFirst;
  UINT numSec;
  UINT cNum,hNum,sNum;
  bool lba;
  DDiskImageMem *disk;
};


class DPartitionTableMbr : public DPartitionTable
{
public:
  DPartitionTableMbr(const char *name);
  virtual ~DPartitionTableMbr();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *getName() { return "DPartitionTableMbr"; }
  virtual DDiskImageMem *getDisk();
  //
protected:
  //
  virtual void init();
  //
  virtual bool readGeometry();
  virtual bool readDirectory();
  //
  DPartitionMBR *part[4];
  UINT bootPartNum;
};


class DBootBlockMbr : public DBootBlock
{
public:
  DBootBlockMbr(DDisk *newDisk);
  virtual ~DBootBlockMbr();
  //
protected:
  //
  virtual void init();
  //
};


#endif

