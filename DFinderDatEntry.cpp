
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


#include "DFinderDatEntry.h"


////////////////////////////////////////////////////////////////////////////////
//  DFinderDatEntry Class
////////////////////////////////////////////////////////////////////////////////

DFinderDatEntry::DFinderDatEntry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFinderDatEntry::DFinderDatEntry()\n");
#endif
}



void DFinderDatEntry::init()
{
  int t;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) longName[t]=0;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) shortName[t]=0;
  for(t=0;t<4;t++) type[t]=0;
  for(t=0;t<4;t++) creator[t]=0;
}

