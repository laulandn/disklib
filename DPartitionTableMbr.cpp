
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


#include "DPartitionTableMbr.h"


////////////////////////////////////////////////////////////////////////////////
//  DPartitionTableMbr Class
////////////////////////////////////////////////////////////////////////////////

DPartitionTableMbr::DPartitionTableMbr(const char *fname) : DPartitionTable(fname)
{
  debug_msg("DPartitionTableMbr::DPartitionTableMbr()\n");
  init();
  //myVFS=parent;
  Mount(fname);
}


DPartitionTableMbr::~DPartitionTableMbr()
{
}


void DPartitionTableMbr::init()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DPartitionTableMbr::init()\n");
#endif
  DPartitionTable::init();
  bootPartNum=0;
}


/* STATIC */
bool DPartitionTableMbr::recognize(const char *fname)
{
  bool ret=false;
  char *buffer=(char *)malloc(64*1024);
  FILE *f=fopen(fname,"rb");
  if(!f) {
    debug_msg("DPartitionTableMbr::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
    return ret;
  }
  size_t nread=::fread(buffer,1,512,f);
  if(nread!=512) {
    debug_msg("DPartitionTableMbr::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_READ_ERROR);
    return ret;
  }
  fclose(f);
  //
  //dumpBufHex((uint8_t *)buffer,512);
  if(!((buffer[0x1fe]==(char)0x55)&&(buffer[0x1ff]==(char)0xaa))) {
#ifdef DEBUG_VERBOSE
    debug_msg("mbr signature not found\n");
    debug_hex2(buffer[0x1fe]); debug_hex2(buffer[0x1ff]); debug_msg("\n");
#endif
    free(buffer);
    return false;
  }
  ret=true;  // Let's just say that's enough for now...
  //
  free(buffer);
  return ret;
}


bool DPartitionTableMbr::readDirectory()
{
  debug_msg("DPartitionTableMbr::readDirectory()...\n");
  bool ret=false;
  //
  return ret;
}


bool DPartitionTableMbr::readGeometry()
{
  debug_msg("DPartitionTableMbr::readGeometry()\n");
  unsigned offset=0;
  //
  blockSize=512;
  bool rret=readBlock(0);
  if(!rret) { debug_msg("readBlock failed!\n"); return false; }
  dumpBufHex((uint8_t *)buf,512);
  offset=0x1be;
  //
  part[0]=new DPartitionMBR(buf,offset);
  if(part[0]->bootFlag==0x80) bootPartNum=0;
  debug_msg("Partition 0:\n");
  part[0]->debugDump();
  offset+=16;
  part[1]=new DPartitionMBR(buf,offset);
  if(part[1]->bootFlag==0x80) bootPartNum=0;
  debug_msg("Partition 1:\n");
  part[1]->debugDump();
  offset+=16;
  part[2]=new DPartitionMBR(buf,offset);
  if(part[2]->bootFlag==0x80) bootPartNum=0;
  debug_msg("Partition 2:\n");
  part[2]->debugDump();
  offset+=16;
  part[3]=new DPartitionMBR(buf,offset);
  if(part[3]->bootFlag==0x80) bootPartNum=0;
  debug_msg("Partition 3:\n");
  part[3]->debugDump();
  offset+=16;
  //
  return false;
}


DDiskImageMem *DPartitionTableMbr::getDisk()
{
  debug_msg("DPartitionTableMbr::getDisk()\n");
  if(part[bootPartNum]) {
    return part[bootPartNum]->disk;
  }
  else {
    debug_msg("part[bootPartNum] was null!\n");
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//  DBootBlockMbr Class
////////////////////////////////////////////////////////////////////////////////

DBootBlockMbr::DBootBlockMbr(DDisk *newDisk) : DBootBlock(newDisk)
{
  debug_msg("DBootBlockMbr::DBootBlockMbr(...)\n");
  init();
  theDisk=newDisk;
  // NOTE Only first 446 bytes of sector
  theDisk->readBlock(0);
}


DBootBlockMbr::~DBootBlockMbr()
{
}


void DBootBlockMbr::init()
{
  theDisk=NULL;
}


////////////////////////////////////////////////////////////////////////////////
//  DPartitionMBR Class
////////////////////////////////////////////////////////////////////////////////


// NOTE: CHS sectors start at 1, not 0...but LBA start at 0
DPartitionMBR::DPartitionMBR(unsigned char *b,UINT offset)
{
  debug_msg("DPartitionMBR::DPartitionMBR(...)\n");
  UINT val;
  bool empty=true;
  bootFlag=b[offset]; offset++;
  hFirst=b[offset]; offset++;
  val=b[offset]; offset++;
  sFirst=val&0x3f;
  cFirst=b[offset]+((val&0xc0)<<2); offset++;
  partType=b[offset]; offset++;
  hLast=b[offset]; offset++;
  val=b[offset]; offset++;
  sLast=val&0x3f;
  cLast=b[offset]+((val&0xc0)<<2); offset++;
  lbaFirst=(b[offset+3]<<24)+(b[offset+2]<<16)+(b[offset+1]<<8)+b[offset+0]; offset+=4;
  numSec=(b[offset+3]<<24)+(b[offset+2]<<16)+(b[offset+1]<<8)+b[offset+0]; offset+=4;
  // These are only valid for CHS
  cNum=cLast-cFirst;
  hNum=hLast-hFirst;
  sNum=sLast-sFirst;
  if((cFirst==1023)&&(hFirst==254)&&(sFirst==63)) lba=true; else lba=false;
  // TODO: If using LBA, the CHS is pretty much unused.  Need to calc fake geometry.
  if(numSec) createDiskFromPart(b,offset);
}


bool DPartitionMBR::createDiskFromPart(unsigned char *b,UINT offset)
{
  debug_msg("DPartitionMBR::createDiskFromPart(...)\n");
  disk=new DDiskImageMem(b,offset);
  return false;
}

void DPartitionMBR::debugDump() {
  debug_msg(" bootFlag="); debug_int(bootFlag);
  if(bootFlag&0x80) debug_msg(" (bootable)");
  debug_msg("\n");
  debug_msg(" cFirst="); debug_int(cFirst); debug_msg(" ");
  debug_msg("hFirst="); debug_int(hFirst); debug_msg(" ");
  debug_msg("sFirst="); debug_int(sFirst); debug_msg("\n");
  debug_msg(" partType="); debug_int(partType); debug_msg("\n");
  debug_msg(" cLast="); debug_int(cLast); debug_msg(" ");
  debug_msg("hLast="); debug_int(hLast); debug_msg(" ");
  debug_msg("sLast="); debug_int(sLast); debug_msg("\n");
  debug_msg(" lbaFirst="); debug_int(lbaFirst); debug_msg("\n");
  debug_msg(" numSec="); debug_int(numSec); debug_msg("\n");
  debug_msg(" lba="); debug_int(lba); debug_msg("\n");
}


////////////////////////////////////////////////////////////////////////////////
//  Notes
////////////////////////////////////////////////////////////////////////////////

/*

Some well-known resulting limits are the 502 MB and the 8.4 GB barriers.

*/
