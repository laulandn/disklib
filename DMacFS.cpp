
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


#include <string.h>


#include "DMacFS.h"


////////////////////////////////////////////////////////////////////////////////
//  DMacFS Class
////////////////////////////////////////////////////////////////////////////////

DMacFS::DMacFS(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DMacFS::DMacFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


void DMacFS::init()
{
  noFileSystem=false;
  allTracksSameSectors=true;
  // Init for standard original disk
  maxTrack=40;
  maxSector=10;
  maxSide=1;
  blockSize=256;
  diskCopy=false;
}


DMacFS::~DMacFS()
{
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
bool DMacFS::recognize(const char *fname)
{
  //if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  if(!fname) return ret;
  FILE *f=fopen(fname,"rb");
  if(!f) {
#ifdef DEBUG_VERBOSE
    debug_msg("DMacFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
#endif
    return false;
  }
  size_t size=getSize(fname);
  //
  char *tbuf=(char *)malloc(size);
  if(!tbuf) {
    /*fclose(f); f=NULL;*/ debug_msg("DMacFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret;
  }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  //
  if(f) fclose(f);
  //
  char *tstring=NULL;
  // I think these are for "blessed" disks only...
  tstring=(char *)"LK`";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) ret=true;
  if(!strncmp((char *)tbuf+0x54,tstring,strlen(tstring))) ret=true;
  if(ret) {
    debug_msg("Found what looks like mac boot block...\n");
  }
  //
  // This is a hack to find diskcopy images...and probably not right...
  bool check=false;
  if(((tbuf[0x18]==(char)0x00)&&(tbuf[0x19]==(char)0x50)&&(tbuf[0x18]==(char)0x00))) check=true;
  if(check) {
    check=false;
    if(((tbuf[0x20]==(char)0x09)&&(tbuf[0x21]==(char)0x98)&&(tbuf[0x22]==(char)0x00)&&(tbuf[0x23]==(char)0x8c))) check=true;
  }
  if(check) {
    check=false;
    if(((tbuf[0x24]==(char)0x00)&&(tbuf[0x25]==(char)0xaa)&&(tbuf[0x26]==(char)0x00)&&(tbuf[027]==(char)0x9b))) check=true;
  }
  if(check) {
    // going to guess diskcopy image...
    debug_msg("Found what looks like diskcopy image signature...\n");
    ret=true;
  }
  //
  return ret;
}


/* STATIC */
bool DMacFS::recognizeFileExtension(const char *fname)
{
  //debug_msg("DMacFS::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
/*
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DMacFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  //if(!strcmp(ext,"atr")) return true;
*/
  // actually anything is ok...
  return false;
}


const char *DMacFS::guessSystem(const char *fname)
{
  return "acorn";
}


bool DMacFS::readDirectory()
{
  debug_msg("DMacFS::readDirectory...\n");
  freeDirCache();
  //
  return false;
}


bool DMacFS::cacheFile(void *fp)
{
  debug_msg("DMacFS::cacheFile not implemented!\n");
  diskError=true;
  return false;
}


bool DMacFS::readDirEntry(long off)
{
  debug_msg("DMacFS::readDirEntry("); debug_long(off); debug_msg(")...\n");
  return false;
}


bool DMacFS::readGeometry()
{
  debug_msg("DMacFS::readGeometry()...\n");
  bool ret=false;
  //
  return ret;
}


bool DMacFS::readBootBlock()
{
  debug_msg("DMacFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DMacFS::detectSubFormat()
{
  // By default
  return false;
}


bool DMacFS::sanityCheck()
{
  debug_msg("DMacFS::sanityCheck()...\n");
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default
  return false;
}


/*

diskcopy 4.2 images:
84 byte header
pascal string name at start
16 bit tag size at 0x46
format (2?) at byte 81?


twiggy disks:
Each physical sector stores 512 data bytes and 20 tag bytes. Each side of the disk had 46 tracks, and the number of sectors per track varies from 15 to 22. This results in 851 sectors per side, or a total capacity of 871,424 bytes.[5] The Lisa Hardware Manual doesn't explicitly state the total number of sectors, but page 173 states that there are 4 tracks of 22 sectors, 7x21, 6x20, 6x19, 6x18, 6x17, 7x16, and 4x15. The controller uses similar circuitry to the Disk II controller, but runs at twice the clock rate. The controller uses a dedicated MOS 6504 microprocessor; in the Lisa this is on the system I/O card, and for the UniFile/DuoFile products, it is on an interface card that plugs into a peripheral expansion slot. The Lisa 2/10 and Macintosh XL I/O card use the IWM controller chip to replace the TTL chips of the earlier design.

 */
