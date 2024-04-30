
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


#include "DSmartDisk.h"

#include "DLocalDrive.h"
#include "DAcornFS.h"
#include "DAtariFS.h"
#include "DApple2FS.h"
#include "DAppleDos3FS.h"
#include "DAppleProDosFS.h"
#include "DCbmFS.h"
#include "DCpmFS.h"
#include "DFatFS.h"
#include "DMacFS.h"
#include "DTrs80FS.h"

#include "DPartitionTableMbr.h"
#include "DPartitionTableApple.h"


//#define DVFS_INCLUDE_ALL_PLUGINS 1


////////////////////////////////////////////////////////////////////////////////
//  DSmartDisk Class
////////////////////////////////////////////////////////////////////////////////

DSmartDisk::DSmartDisk(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DSmartDisk created.\n");
#endif
  theDisk=NULL;
  Mount(fname);
}


DSmartDisk::~DSmartDisk()
{
}


DDisk *DSmartDisk::getTheDisk()
{
  if(theDisk) return theDisk;
#ifdef DEBUG_VERBOSE
  else { debug_msg("DSmartDisk::getTheDisk "); debug_msg(DDISK_ERR_NO_DISK); }
#endif
  return NULL;
}


// NOTE: fname can be NULL...
// NOTE: Be sure to add any other formats here...
bool DSmartDisk::Mount(const char *fname)
{
  if(theDisk) Unmount();
  if(!fname) {
#ifdef DEBUG_VERBOSE
    debug_msg("mount: Going to create new DLocalDrive...\n");
#endif
    theDisk=new DLocalDrive("Local Drive");
    return true;
  }
#ifdef DEBUG_VERBOSE
  debug_msg("DSmartDisk::mount('"); debug_msg(fname); debug_msg("')...\n");
#endif
  // FIXME: check if file exists before having recognize try...
  //
  // TODO: Remember to add new formats here!
  if(!theDisk) { if(DAcornFS::recognize(fname))  theDisk=new DAcornFS(fname); }
  if(!theDisk) { if(DApplePascalFS::recognize(fname))  theDisk=new DApplePascalFS(fname); }
  if(!theDisk) { if(DFatFS::recognize(fname))  theDisk=new DFatFS(fname); }
  if(!theDisk) { if(DCpmFS::recognize(fname))  theDisk=new DCpmFS(fname); }
  if(!theDisk) { if(DAppleDos3FS::recognize(fname))  theDisk=new DAppleDos3FS(fname); }
  if(!theDisk) { if(DAppleProDosFS::recognize(fname))  theDisk=new DAppleProDosFS(fname); }
  if(!theDisk) { if(DAppleCpmFS::recognize(fname))  theDisk=new DAppleCpmFS(fname); }
  if(!theDisk) { if(DAtariFS::recognize(fname))  theDisk=new DAtariFS(fname); }
  if(!theDisk) { if(DMacFS::recognize(fname))  theDisk=new DMacFS(fname); }
  if(!theDisk) { if(DCbmFS::recognize(fname))  theDisk=new DCbmFS(fname); }
  if(!theDisk) { if(DTrs80FS::recognize(fname))  theDisk=new DTrs80FS(fname); }
  //
  DPartitionTable *theTable=NULL;
  if(!theDisk) {
    if(DPartitionTableMbr::recognize(fname)) {
#ifdef DEBUG_VERBOSE
      debug_msg("DSmartDisk::Mount look like DPartitionTableMbr...\n");
#endif
      theTable=new DPartitionTableMbr(fname);
    }
  }
  if(!theDisk) {
    if(DPartitionTableApple::recognize(fname)) {
#ifdef DEBUG_VERBOSE
      debug_msg("DSmartDisk::Mount look like DPartitionTableApple...\n");
#endif
      theTable=new DPartitionTableApple(fname);
    }
  }
  if(theTable) {
    theDisk=theTable->getDisk();
    if(!theDisk) debug_msg("DSmartDisk::Mount found partitionTable, but didn't get disk!\n");
  }
  //
  /*
  // NOTE: CP/M must come last since there are mutant apple and cbm (to name a few) formats
  if(!theDisk) { if(DCpmFS::recognize(fname))  theDisk=new DCpmFS(fname); }
  */
  //
  if(theDisk) {
    if(theDisk->err.getError()) {
      debug_msg(theDisk->getName()); debug_msg("\n");
      debug_msg(DDISK_ERR_DISK);
      delete theDisk;
      theDisk=(DDisk *)NULL;
      err.setError();
      return false;
    }
  }
  else {
#ifdef DEBUG_VERBOSE
    debug_msg("DSmartDisk::mount disk format wasn't recognized!\n");
#endif
    return false;
  }
  return true;
}


bool DSmartDisk::Unmount()
{
  if(theDisk) {
    delete theDisk;
    theDisk=NULL;
#ifdef DEBUG_VERBOSE
    debug_msg("Unmounted disk.\n");
#endif
    /*
    if(num==0) {
#ifdef DEBUG_VERBOSE
      debug_msg("Unmount: Going to create new DLocalDrive...\n");
#endif
      disk[0]=new DLocalDrive(this);
    }
    */
  }
  else {
    debug_msg("unmount: No disk in drive!\n");
    return false;
  }
  return true;
}
