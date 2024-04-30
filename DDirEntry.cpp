
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


#include "DPartitionTable.h"
#include "DDiskImageFile.h"
#include "DDiskImageMem.h"
#include "DLocalDrive.h"


#ifdef ALIB_HAVE_UNISTD_H
#include <unistd.h>
#endif // ALIB_HAVE_UNISTD_H

#ifdef ALIB_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef ALIB_HAVE_STAT_H
#include <stat.h>
#endif

#ifdef ALIB_HAVE_DIRENT_H
#include <dirent.h>
#endif // ALIB_HAVE_DIRENT_H

#ifdef ALIB_HAVE_DIRECT_H
#include <direct.h>
#endif // ALIB_HAVE_DIRECT_H

#ifdef ALIB_HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif // ALIB_HAVE_SYS_DIR_H


#ifdef ASYS_MAC
#ifndef __GNUC__
#define ALIB_MAC_MOREFILES 1
#include <IterateDirectory.h>
#endif // __GNUC__
#endif // ASYS_MAC


////////////////////////////////////////////////////////////////////////////////
//  DDirEntry Class
////////////////////////////////////////////////////////////////////////////////

DDirEntry::DDirEntry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDirEntry created.\n");
#endif
  signature=0;
  for(UINT t=0;t<DDISK_MAX_NAMELEN;t++) name[t]=0;
  type=DDISK_TYPE_NONE;
  nativeType=0;
  sizeBlocks=0;
  size=0;
  track=0;  sector=0;
  block=0;
  data=NULL;
  curSeek=0;
  locked=false;
  //
  // Implementation dependent, or unused
  privateVal1=0;
  privateVal2=0;
}


DDirEntry::~DDirEntry()
{
  // Nothing to do really
}


bool DDirEntry::checkDiskSanity(DDiskImage *theDisk)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDirEntry::checkDiskSanity()...\n");
#endif
  if(!theDisk) return false;
  if(!sanityCheck()) return false;
  //
  //if(track>theDisk->getMaxTrack()) return false;
  //if(theDisk->getNoTrackZero()) { if(!track) return false; }
  //if(sector>theDisk->getMaxSector()) return false;
  //
  return true;
}


bool DDirEntry::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDirEntry::sanityCheck()...\n");
#endif
  if(!basicSanityCheck()) return false;
  // By default
  return true;
}


bool DDirEntry::basicSanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDirEntry::basicSanityCheck()...\n");
#endif
  // By default
  return true;
}


void DDirEntry::debugDump()
{
  debug_msg("DDirEntry::debugDump()...\n");
  if(!sanityCheck()) { debug_msg("DDirEntry failed sanityCheck!!!\n"); }
  debug_msg("signature: "); debug_hex(signature); debug_msg("\n");
  debug_msg("name: "); debug_msg(name); debug_msg("\n");
  debug_msg("nativeType: "); debug_int(nativeType); debug_msg("\n");
  debug_msg("type: "); debug_int(type); debug_msg("\n");
  debug_msg("file sizeBlocks: "); debug_long(sizeBlocks); debug_msg("\n");
  debug_msg("file size: "); debug_long(size); debug_msg("\n");
  debug_msg("track: "); debug_int(track); debug_msg("\n");
  debug_msg("sector: "); debug_int(sector); debug_msg("\n");
  debug_msg("block: "); debug_int(block); debug_msg("\n");
  if(data) debug_msg("data: present\n");
  else  debug_msg("data: NULL\n");
  debug_msg("curSeek: "); debug_long(curSeek); debug_msg("\n");
  debug_msg("locked: "); debug_int(locked); debug_msg("\n");
}
