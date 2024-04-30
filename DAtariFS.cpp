
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


#include <string.h>


#include "DAtariFS.h"


////////////////////////////////////////////////////////////////////////////////
//  DAtariFS Class
////////////////////////////////////////////////////////////////////////////////

DAtariFS::DAtariFS(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DAtariFS::DAtariFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


DAtariFS::~DAtariFS()
{
  freeDirCache();
  diskMounted=false;
}


const char *DAtariFS::guessSystem(const char *fname)
{
  return "atari800";
}


/* STATIC */
bool DAtariFS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  FILE *f=fopen(fname,"rb");
  if(!f) { debug_msg("DAtariFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN); return ret; }
  size_t size=getSize(fname);
  if((size!=92176)&&(size!=92160)) {
    //::fclose(f); f=NULL;
    //debug_msg("DAtariFS::recognize: size was wrong for image\n");
    return ret;
  }
  char *tbuf=(char *)malloc(size);
  if(!tbuf) { /*::fclose(f);*/ debug_msg("DAtariFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret; }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(f) ::fclose(f);
  // Just about anything is okay with us...we'll check for errors later
  //debug_msg("DCbmFS::recognize was successful!\n");
  free(tbuf);
  ret=true;
  return ret;
}


/* STATIC */
bool DAtariFS::recognizeFileExtension(const char *fname)
{
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DAtariFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,"atr")) return true;
  return false;
}


void DAtariFS::init()
{
  noFileSystem=false;
  allTracksSameSectors=true;
  atrHeader=false;
  maxTrack=40;
  maxSector=18;
  maxSide=1;
  blockSize=128;
  for(UINT t=0;t<128;t++) { vtoc[t]=0; }
}


bool DAtariFS::readDirectory()
{
  debug_msg("DAtariFS::readDirectory...\n");
  freeDirCache();
  UINT t=0;
  for(t=0;t<256;t++) diskName[t]=0;
  strcpy(diskName,"NO NAME");
  readBlock(0);  // NOTE: This will get the ATR header if its there before we set skipOffset!
  switch(imageDataSize)
  {
    case 92176:
      // Let's guess an atr image with 16 byte header...
      skipOffset=16;
      atrHeader=true;
      debug_msg("DAtariFS::readDirectory guessing an atr image with header...\n");
      break;
    case 92160:
      // Let's guess an atr image w/o header...
      debug_msg("DAtariFS::readDirectory guessing an atr image w/o header...\n");
      break;
    default:
      if((buf[0]==0x96)&&(buf[1]==0x02)) {
        // ATR sig found!
        skipOffset=16;
        atrHeader=true;
        UINT wPars=(buf[3]*256)+buf[2];
        UINT wSecSize=(buf[5]*256)+buf[4];
        UINT btParsHigh=buf[6];
        debug_msg("wPars="); debug_int(wPars); debug_msg("("); debug_int(wPars*16); debug_msg(" bytes) wSecSize="); debug_int(wSecSize); debug_msg(" btParsHigh="); debug_int(btParsHigh); debug_msg("\n");
        blockSize=wSecSize;
        // NOTE: We aren't told what the # of tracks or sectors is, so we just make up something that'd fit the size we've got.
        // (The size values used are mostly arbitrary to fit between actual sizes)
        //
        // So...standard disks are 90k, we only change the geometry if the image is larger...
        if(imageDataSize>100000) {
          if(blockSize==128) { maxSector=26;  debug_msg("guessing 'enhanced density' for now, around 127k\n"); }
          else { debug_msg("NOTE: larger than ~100k, but 256 byte blocks...\n"); }
        }
        if(imageDataSize>150000) {
          if(blockSize==128) { maxSector=32;  debug_msg("guessing '1050' for now, around 160k\n"); }
          else { debug_msg("NOTE: larger than ~150k, but 256 byte blocks...\n"); }
        }
        if(imageDataSize>170000) {
          if(blockSize==256) { maxSector=26;  debug_msg("guessing 'double density' for now, around 180k\n"); }
          else { debug_msg("NOTE: larger than ~170k, but 128 byte blocks...\n"); }
        }
        if(imageDataSize>320000) {
          if(blockSize==256) { maxSector=26; maxSide=2; debug_msg("guessing 'quad density' for now, around 360k\n"); }
          else { debug_msg("NOTE: larger than ~320k, but 128 byte blocks...\n"); }
        }
        debug_msg("Guessing ATR image geometry is...\n");
        debug_msg("maxTrack="); debug_int(maxTrack); debug_msg(" maxSector="); debug_int(maxSector); debug_msg(" maxSide="); debug_int(maxSide); debug_msg(" blockSize="); debug_int(blockSize); debug_msg("\n");
      }
      else {
        debug_msg("DAtariFS::readDirectory can't figure out image format!\n"); diskError=true; diskStatus=DDISK_STATUS_BAD_FORMAT; return false;
      }
      break;
  }
  // NOTE: We are one sector off for some reason...for now...
  //
  // Read VTOC
  debug_msg("DAtariFS::readDirectory reading VTOC...\n");
  readBlock(359);
  for(t=0;t<128;t++) vtoc[t]=buf[t];
  // TODO: Use info here...
  //
  // Now read directory entries
  bool theResult=false;
  for(t=360;t<368;t++) {
    readBlock(t);
    // 8 entries per block
    for(UINT i=0;i<8;i++) {
      theResult=readDirEntry(i*16);
      if(!theResult) return true;
    }
  }
  return true;
}


// Is any of this right?
bool DAtariFS::readDirEntry(long off)
{
  debug_msg("DAtariFS::readDirEntry("); debug_long(off); debug_msg(")...\n");
  UINT firstBlock=(buf[off+4]*256)+buf[off+3];
  if(firstBlock) {
    UINT t=0;
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<DDISK_MAX_NAMELEN;t++) theFile->name[t]=0;
      theFile->name[DDISK_MAX_NAMELEN-1]=0;
      for(t=5;t<13;t++) theFile->name[t-5]=buf[off+t];
      theFile->name[8]='.';
      for(t=13;t<16;t++) theFile->name[t-4]=buf[off+t];
      debug_msg("name is '"); debug_msg(theFile->name); debug_msg("'\n");
      theFile->sizeBlocks=(buf[off+2]*256)+buf[off+1];
      debug_msg("size is "); debug_long(theFile->sizeBlocks); debug_msg(" blocks\n");
      theFile->nativeType=buf[0];
      debug_msg("nativeType="); debug_int(theFile->nativeType); debug_msg("\n");
      theFile->type=DDISK_TYPE_BINARY; // TODO
      UINT firstTrack=firstBlock/maxSector;
      UINT firstSector=firstBlock-(firstTrack*maxSector);
      debug_msg("firstBlock="); debug_int(firstBlock); debug_msg(" firstTrack="); debug_int(firstTrack); debug_msg(" firstSector="); debug_int(firstSector); debug_msg("\n");
      debug_msg("(sanity check: block is "); debug_int((firstTrack*maxSector)+firstSector); debug_msg(")\n");
      theFile->track=firstTrack;
      theFile->sector=firstSector;
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*blockSize;
      theFile->block=0;
      theFile->curSeek=0;
      theDir.append((void *)theFile);
    }
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  else { debug_msg("DAtariFS at "); debug_long(off); debug_msg(" "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
  debug_msg("DAtariFS at "); debug_long(off); debug_msg(" good file entry!\n");
  return true;
}


// NOTE: We override this because our first 3 sectors are ALWAYS 128 bytes long...
// TODO: What if there's no block 0?!?
bool DAtariFS::readBlock(UINT blk)
{
  diskError=true;
  if(!imageData) { debug_msg("No imageData for disk image!\n"); return false;}
  UINT tBlockSize=blockSize;
  UINT tBlk=blk;
  if(blk<4) {
    tBlockSize=128;
    curLoc=blk*tBlockSize+skipOffset;
  }
  else {
    tBlk=blk-3;
    curLoc=3*128;
    curLoc+=tBlk*blockSize+skipOffset;
  }
  if(curLoc>imageDataSize) { debug_msg("Past end of imageData!\n"); return false; }
  debug_msg("DAtariFS::readBlock("); debug_int(blk); debug_msg(") at offset "); debug_long(curLoc); debug_msg(" tBlk="); debug_int(tBlk); debug_msg(" tBlockSize="); debug_int(tBlockSize); debug_msg("\n");
  //debug_msg((int)0); debug_msg(": ";
  for(UINT t=0;t<tBlockSize;t++) {
    buf[t]=imageData[curLoc+t];
    //debug_msg((int)buf[t]); debug_msg(",";
    //if(t&&(!(t&0xf))) debug_msg("\n"); debug_msg((int)t); debug_msg(": ";
  }
  //debug_msg("\n");
  diskError=false;
  return true;
}


bool DAtariFS::writeBlock(UINT blk)
{
  debug_msg("DAtariFS::writeBlock not implemented!\n");
  diskError=true;
  diskStatus=DDISK_STATUS_WRITE_ERROR;
  return false;
}


bool DAtariFS::cacheFile(void *fp)
{
  debug_msg("DAtariFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DAtariFS::readGeometry()
{
  debug_msg("DAtariFS::readGeometry()...\n");
  bool ret=false;
  blockSize=0;
  maxTrack=0;
  maxSector=0;
  maxSide=0;
  return ret;
}


bool DAtariFS::readBootBlock()
{
  debug_msg("DAtariFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DAtariFS::detectSubFormat()
{
  // By default
  return false;
}


bool DAtariFS::sanityCheck()
{
  debug_msg("DAtariFS::sanityCheck()...\n");
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//  Docs
////////////////////////////////////////////////////////////////////////////////

/*

ATR image file format:

----------------------------------------------------------------------------------------------

ATR file format is invented by Nick Kennedy for his SIO2PC program. It is
also used by other emulators. It consists of a 16-byte header followed by the
raw dump of disk sectors. Despite being a de facto standard as the Atari
disk image file format, some parts of the header are redefined in an
incompatible way by different emulators. The first definition of the header
by Nick Kennedy was as follows:

WORD Magic;
WORD Size;
WORD SectorSize;
BYTE Unused[10];

Note: In this file, BYTE means an 8-bit data, WORD means a 16-bit data and
LONG means a 32-bit data. WORD and LONG are in the LSB-first format.

The explanation of individual fields are as follows:

Magic: This is a special code to identify an ATR file. Its value should be
be equal to the byte of the sum of ASCII values of the characters of
the string "NICKATARI", which is 0x0296 (662). If a different value is
encountered in the first two bytes of a file, then this is not an ATR
image file.

Size: Size of the image, excluding this header, in paragraphes, that is, in
16-byte blocks. Note that this allows a maximum size of 0xFFFF0 bytes,
which is not enough to create larger sizes. See SIO2PC rev. 3.0
extensions for the solution of this problem. When

SectorSize: This is the size, in bytes, of one sector of the image. It should
be either 128 (for single density and enhanced density disks) or
256 (for double density disks). Note that regardless the value of
this field, the first three sectors of all images are always
128-byte long. However there are some broken images where these
sectors are 256-byte long for double density disks.

Unused: These bytes are not used by the first definition of the format and
should be zero.

----------------------------------------------------------------------------------------------

In revision 3.0 of the SIO2PC program, Nick Kennedy has extended the format
as follows:

WORD Magic;
WORD Size;
WORD SectorSize;
WORD SizeHi;
BYTE Unused[8];

The newly added SizeHi field holds the high word of the image size in
paragraphes. This allows 0xFFFFFFFF0 bytes as the maximum size. But the
maximum size of a real atari disk can't exceed 0xFFFD80 (65535 sectors,
double density). So only the first byte of this field should be used.

----------------------------------------------------------------------------------------------

In revision 4.05 of SIO2PC, Nick Kennedy has further extended his format to
cover some disk protection schemes. But please note that these extensions are
rarely used. The header's structure has became as follows:

WORD Magic;
WORD Size;
WORD SectorSize;
WORD SizeHi;
BYTE Flags;
WORD ProtectionInfo;
BYTE Unused[5];

The newly added fields are defined as follows:

Flags: This byte holds two flags. If bit 4 is set, the image has bad sector
information. If bit 5 is set, the image is write protected. If the
image have bad sector information, the sectors starting with
0x1E3D1CC2 will be treated as bad sectors. See below.

ProtectionInfo: If the bit 4 of the Flags byte is set, this field holds the
bad sector with copy protection information in it. This
sector contains, in addition to bad sector information, some
information about the good sectors and I/O timing, in order to
emulate some copy protection schemes. See below:

A bad sector has the following information, instead of sector data:

LONG Magic;
BYTE BadSectorStatus[4];
BYTE AckResponse;
BYTE CptResponse;
BYTE ChecksumStatus;

The rest of the sector is unused.

Magic: This is a special code to identify sectors with bad sector information
instead of sector data. It should be 0x1E3D1CC2.

BadSectorStatus: This is the 4-byte data, in reverse order, to be returned to
Atari in response to a get status (0x53, 'S') SIO command
after an attempt to read this bad sector.

AckResponse: This is the acknowledgement code to be returned to Atati, instead
of ACK [0x41('A')] byte, when an attempt is made to read this
bad sector. It can be 'A' for ACK, 'N' for NAK or 'T' for
timeout. If it is 'T', the acknowledgement code will not be
sent.

CptResponse: This is the operation complete code to be returned to Atari,
instead of CPT [0x43('C')] byte, when an attempt is made to read
this bad sector. It can be 'C' for CPT, 'E' for ERR, 'T' for
timeout or '_' for not available. If it is 'T', the complete
code will not be sent. If the AckResponse is 'N' or 'T', this
field should be '_'.

ChecksumStatus: This byte, although not used by SIO2PC, is 'G' for good
checksum and 'B' for bad checksum.

The protection info sector, which is the bad sector pointed by the
ProtectionInfo field of the header, has the following information in addition
to the ordinary bad sector information:

BYTE GoodSectorTiming;
BYTE BadSectorTiming;
BYTE GoodSectorStatus[4];

GoodSectorTiming: This is the amount of delay, in jiffies (1/18.2 seconds),
after responding to a good sector read/write command. This
information is not used by SIO2PC.

BadSectorTiming: This is the amount of delay in jiffies, after responding to
a bad sector read/write command. This information is not
used by SIO2PC.

GoodSectorStatus: This is the 4-byte data, in reverse order, to be returned
to Atari in response to a get status (0x53, 'S') SIO
command, after a good sector read/write. Note that all
good sectors of an image returns the same code.

----------------------------------------------------------------------------------------------

Unfortunately, Steven Tucker, the author of APE, has extended the ATR header
structure in an incompatible way with SIO2PC rev. 4.05. These extension are
as follows:

WORD Magic;
WORD Size;
WORD SectorSize;
WORD SizeHi;
LONG CRC32;
BYTE Unused[3];
BYTE Status;

CRC32: This is the CRC32 of the image. It is calculated as if the second
8-byte part of the header contains all zeroes.

Status: This byte holds some information about disk status. If bit 0 is set,
then the image should be threated as write protected. If bit 1 is
set, then the image is sealed and the CRC32 code is valid.

*/


/*

As an add on, these are some standard image sizes:

Atari 810, Atari Dos 2.0s compatible: (almost universal)
Single Sided, Single Density, 40 tracks per side, 18 sectors per track, 128 bytes per sector. 720 sectors, 92160 bytes total.

Atari 1050, Atari Dos 2.5 compatible:
Single Sided, Enhanced* Density, 40 tracks per side, 26 sectors per track, 128 bytes per sector. 1040 sectors, 133120 bytes total.

* Atari calls it "dual" density. Prone to confusion with real "double" density.

Indus GT, compatible with some newer DOSes:
Single Sided, Double Density, 40 tracks per side, 18 sectors per track, 256 bytes per sector (except first 3 sectors that are 128 bytes). 720 sectors, 183936 bytes total.

Atari XF551, compatible with some newer DOSes:
Double Sided, Double Density, 40 tracks per side, 18 sectors per track, 256 bytes per sector (except first 3 sectors that are 128 bytes). 1440 sectors, 368256 bytes total.

Percom single sided, compatible with some newer DOSes:
Single Sided, Double Density, 80 tracks per side, 18 sectors per track, 256 bytes per sector (except first 3 sectors that are 128 bytes). 1440 sectors, 368256 bytes total.

Percom double sided, compatible with some newer DOSes:
Double Sided, Double Density, 80 tracks per side, 18 sectors per track, 256 bytes per sector (except first 3 sectors that are 128 bytes). 2880 sectors, 736896 bytes total.

Hard disks should report their sizes as single sided single track. They can be double or single density.

Here are some definitions:

Number of sides:
1 -> ss (single sided)
2 -> ds (double sided)
Tracks per side:
1 -> Hard disks
35 -> Rare
40 -> Most common format
77 -> Rare
80 -> Sometimes called "quad" density.
Sectors per track:
18 -> sd/dd (single density or double density)
26 -> ed (enhanced density, also called dual density)
Bytes per sector
128 -> single or enhanced density, all sectors are 128 bytes long
256- -> double density, first 3 sectors are 128 bytes long, rest is 256 bytes long.

So, basically, you read bytes per sector info from an ATR file and you decide if it's a sd/ed or dd disk. Then you check the size and calculate the number of sectors. You look up a table to determine other disk geometry parameters, e.g. to use in an emulator to supply get geometry and set geometry functions. (SIO commands 0x4E and 0x4F).

However some geometries cannot be determined with table lookup. Percom SS and Atari XF551 have the exact same size and sectors per track. It seems that another extension to ATR format is necessary :)

The full list:
ss, sd, 35: 1*35*80= 630 * 128 = 80640
ss, sd, 40: 1*18*40= 720 * 128 = 92160 (810)
ss, ed, 35: 1*26*35= 910 * 128 = 116480
ss, ed, 40: 1*26*40=1040 * 128 = 133120 (1050)
ss, dd, 35: 1*18*35= 630 . 256 = 160896
ds, sd, 35: 2*18*35=1260 * 128 = 161280
ss, sd, 77: 1*18*77=1386 * 128 = 177408
ss, dd, 40: 1*18*40= 720 . 256 = 183936 (indus)
ss, sd, 80: 1*18*80=1440 * 128 = 184320 -|
ds, sd, 40: 2*18*40=1440 * 128 = 184320 -|
ds, ed, 35: 2*26*35=1820 * 128 = 232960
ss, ed, 35: 1*26*80=2080 * 128 = 266240 -|
ds, ed, 35: 2*26*40=2080 * 128 = 266240 -|
ds, dd, 35: 2*18*35=1260 . 256 = 322176
ss, dd, 77: 1*18*77=1386 . 256 = 354432
ds, sd, 77: 2*18*77=2772 * 128 = 354816
ss, dd, 80: 1*18*80=1440 . 256 = 368256 -| (percom single sided)
ds, dd, 40: 2*18*40=1440 . 256 = 368256 -| (xf551)
ds, sd, 80: 2*18*80=2880 * 128 = 368640
ds, ed, 35: 2*26*80=4160 * 128 = 532480
ds, dd, 77: 2*18*77=2772 . 256 = 709248
ds, dd, 80: 2*18*80=2880 . 256 = 736896 (percom double sided)

Well, there are also broken double density image files which store 256 bytes for the first 3 sectors instead of 128:

ss, bd, 35: 1*18*35= 630 * 256 = 161280
ss, bd, 40: 1*18*40= 720 * 256 = 184320 (broken indus)
ds, bd, 35: 2*18*35=1260 * 256 = 322560
ss, bd, 77: 1*18*77=1386 * 256 = 354816
ss, bd, 80: 1*18*80=1440 * 256 = 368640 -| (broken percom single sided)
ds, bd, 40: 2*18*40=1440 * 256 = 368640 -| (broken xf551)
ds, bd, 77: 2*18*77=2772 * 256 = 709632 (broken)
ds, bd, 80: 2*18*80=2880 * 256 = 737280 (broken percom double sided)

bd means "broken density" :)

*/


/*
NOTES:

First 3 blocks are boot, and always single density regardless of rest of disk

"density" is not always what it means for other disk! (more a rule of thumb)

standard format:
720 sectors, 40 tracks
dos leaves 707 sectors free
128 btes per sector
last 3 bytes of each sector: bytes used, file number, next sector
(leaving 125 bytes for data)

single-sides, single density:
40 tracks, 18 sectors per track, 128 bytes per sector, 90k capacity.

single-sides, double density:
40 tracks, 18 sectors per track, 256 bytes per sector, 180k capacity.

single-sides, enhanced density:
40 tracks, 26 sectors per track, 128 bytes per sector, 127k capacity.

double-sides, double density: (called quad density sometimes)
80 tracks (40 per side), 18 sectors per track, 256 bytes per sector, 360k capacity.

1050 disks:
40 tracks 32 sectors each 128 bytes per sector total 160k space.

Percom, astral, micromainframe 3rd party disks:
40 tracks, 18 sectors each 256 bytes per sector ttal 180k

SD - 1050/XF551:
18 sectors per track
Interleave factor: 9
1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25,
2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26.

ED - 1050/XF551:
26 sectors per track
Interleave factor 13
1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25,
2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26.

I discovered two different interleave factors for DD (the interleave factor is sometimes revered to as 'sector skew'). The first one is 'fast' for 1050 read/write operations and the second makes things faster for the XF551 FD.

DD - 1050:
18 sectors per track
Interleave factor 15
6, 12, 18, 5, 11, 17, 4, 10, 16,
3, 9, 15, 2, 8, 14, 1, 7, 13.

DD - XF551:
18 sectors per track
Interleave factor 9
1, 3, 5, 7, 9, 11, 13, 15, 17,
2, 4, 6, 8, 10, 12, 14, 16, 18.


The 'standard' ATR file format is: (first byte listed as 1, not 0)

    * 01: NICKATARI Signature
    * 02: "
    ($0296)
    * 03: Paragraphs, low
    (size/$10)
    * 04: Paragraphs, high
    * 05: Sector Size, low
    * 06: Sector Size, high
    * 07: Extended paragraphs
    (extra hi byte)
    * 08..16: Unused
    * Remainder of image contains Atari data.


Current APE changes to the ATR header:

    * Byte 8 : 32bit Authentication CRC
    * 9 : 32bit Authentication CRC
    * 10: 32bit Authentication CRC
    * 11: 32bit Authentication CRC

    * 16: Bit 0: Write Protect
          o Bit 1: Sealed/Authenticated Atr
          o Bits 2..7: Unused


XFD format has no header and is just raw sectors...


directory is blocks 361-368
vtoc is 360
dir entries are 16 bytes
8 per sector

*/
