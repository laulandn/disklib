
//#define DEBUG_OUT std::cerr
#define DEBUG_OUT dBug
#define ERR_OUT std::cerr
//#define DEBUG_OUT *aNullStream
//#define CONS_OUT std::cout
#define CONS_OUT *diskLibOutputStream
#define CONS_IN std::cin
//#define CONS_IN *inStream


#ifdef DEBUG_VERBOSE
//#undef DEBUG_VERBOSE
#endif


#include "DPartitionTable.h"


////////////////////////////////////////////////////////////////////////////////
//  DPartitionTable Class
////////////////////////////////////////////////////////////////////////////////

DPartitionTable::DPartitionTable(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DPartitionTable::DPartitionTable(...)\n");
}


DPartitionTable::~DPartitionTable()
{
}


void DPartitionTable::init()
{
 #ifdef DEBUG_VERBOSE
  debug_msg("DPartitionTable::init()\n");
#endif
 hasPartitions=true;
}


bool DPartitionTable::recognize(const char *fname)
{
  bool ret=false;
  debug_msg("DPartitionTable::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return ret;
}


bool DPartitionTable::readDirectory()
{
  debug_msg("DPartitionTable::readDirectory() "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DPartitionTable::readGeometry()
{
  debug_msg("DPartitionTable::readGeometry() "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


DDiskImageMem *DPartitionTable::getDisk()
{
  debug_msg("DPartitionTable::getDisk() "); debug_msg(DDISK_ERR_NO_IMPL);
  return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//  DBootBlock Class
////////////////////////////////////////////////////////////////////////////////

DBootBlock::DBootBlock(DDisk *newDisk)
{
  debug_msg("DBootBlock::DBootBlock(...)\n");
  init();
  theDisk=newDisk;
}


DBootBlock::~DBootBlock()
{
}


void DBootBlock::init()
{
  theDisk=NULL;
}


