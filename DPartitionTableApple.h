#ifndef DLIB_DPARTITIONTABLEAPPLE_H
#define DLIB_DPARTITIONTABLEAPPLE_H


#include "DPartitionTable.h"


class DPartitionTableApple : public DPartitionTable
{
public:
  DPartitionTableApple(const char *name);
  virtual ~DPartitionTableApple();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *getName() { return "DPartitionTableApple"; }
  //
protected:
  //
  virtual void init();
  //
  virtual bool readGeometry();
  virtual bool readDirectory();
  //
};


#endif

