
//#define DEBUG_OUT std::cerr
#define DEBUG_OUT dBug
#define ERR_OUT std::cerr
//#define DEBUG_OUT *aNullStream
//#define CONS_OUT std::cout
#define CONS_OUT *diskLibOutputStream
#define CONS_IN std::cin
//#define CONS_IN *inStream


#ifdef DEBUG_VERBOSE
#undef DEBUG_VERBOSE
#endif


#include "DPartitionTableApple.h"


////////////////////////////////////////////////////////////////////////////////
//  DPartitionTableApple Class
////////////////////////////////////////////////////////////////////////////////

DPartitionTableApple::DPartitionTableApple(const char *fname) : DPartitionTable(fname)
{
  debug_msg("DPartitionTableApple::DPartitionTableApple()\n");
  init();
  //myVFS=parent;
  Mount(fname);
}


DPartitionTableApple::~DPartitionTableApple()
{
}


void DPartitionTableApple::init()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DPartitionTableApple::init()\n");
#endif
  DPartitionTable::init();
}


/* STATIC */
bool DPartitionTableApple::recognize(const char *fname)
{
  bool ret=false;
  //debug_msg("DPartitionTableApple::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return ret;
}


bool DPartitionTableApple::readDirectory()
{
  debug_msg("DPartitionTableApple::readDirectory() "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DPartitionTableApple::readGeometry()
{
  debug_msg("DPartitionTableApple::readGeometry() "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}
