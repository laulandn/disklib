
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


#include <unistd.h>
#include <sys/stat.h>


#include "Dabasics.h"

#include "DPartitionTable.h"
#include "DDiskImageFile.h"
#include "DDiskImageMem.h"

#include "DLocalDrive.h"


#define ALIB_HAVE_DIRENT_H 1
//#define ALIB_HAVE_DIRECT_H 1
//#define ALIB_HAVE_STAT_H 1
#define ALIB_HAVE_UNISTD_H 1
#define ALIB_HAVE_SYS_DIR_H 1
#define ALIB_HAVE_SYS_STAT_H 1
//#define ALIB_STRUCT_DIRECT_NOT_DIRENT 1
#define ALIB_HAVE_ALPHASORT 1
#define ALIB_HAVE_DIRENT 1
//#define ALIB_HAVE_DIRECT 1
#define ALIB_HAVE_SCANDIR 1
#define ALIB_HAVE_STAT 1
#define ALIB_HAVE_DIRENT_D_TYPE 1
#define ALIB_HAVE_RENAME 1


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
//  DLocalDrive Class
////////////////////////////////////////////////////////////////////////////////

// NOTE: fname isn't really any use...
DLocalDrive::DLocalDrive(const char *fname) : DDisk(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::DHarddisk(...)\n");
#endif
  init();
  //myVFS=parent;
  //if(!parent) debug_msg("NOTE: DLocalDrive without an DVfs for parent...\n");
#ifdef ASYS_WIN32CE
  chdir("/My Documents"));
#endif // ASYS_WIN32CE
}


DLocalDrive::~DLocalDrive()
{
}


/* STATIC */
bool DLocalDrive::recognize(const char *fname)
{
  debug_msg("DLocalDrive::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


/* STATIC */
bool DLocalDrive::recognizeFileExtension(const char *fname)
{
  debug_msg("DLocalDrive::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


const char *DLocalDrive::guessSystem(const char *fname)
{
  debug_msg("DLocalDrive::guessSystem('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return NULL;
}


void DLocalDrive::init()
{
  curLoc=0;
  track=0; sector=0; side=0;
  diskError=false;
  // in all caps for those lowercase handicapped old OS's
  const char *tname="HOST DIRECTORY";
  for(UINT t=0;t<strlen(tname);t++) diskName[t]=tname[t];
#ifdef USE_FINDER_DAT
  // TODO: ignoreFinderDat is always false at this point...
  if(!ignoreFinderDat) {
#ifdef DEBUG_VERBOSE
    debug_msg("Going to look for a finder.dat...\n");
#endif
    gotFinderDat=false;
    FILE *finderDatfd=(FILE *)fopen("FINDER.DAT","rb");
    if(!finderDatfd) finderDatfd=(FILE *)fopen("finder.dat","rb");
    if(finderDatfd) {
#ifdef DEBUG_VERBOSE
      debug_msg("(Opened finder.dat, reading it...)\n");
#endif
      gotFinderDat=true;
      readFinderDat(finderDatfd);
      fclose(finderDatfd);
    }
  }
#endif // USE_FINDER_DAT
  cachesDir=true;
  if(cachesDir) readDirectory();
}


#ifdef USE_FINDER_DAT
// TODO: this reads past the end of the real data in fidner.dat
// may be a problem with the working flag
void DLocalDrive::readFinderDat(FILE *f)
{
  if(!f) { debug_msg("DLocalDrive::NULL f!\n"); return; }
  bool working=true;
  char *buf=(char *)malloc(92*sizeof(char));
  DFinderDatEntry *en=(DFinderDatEntry *)NULL;
  if(!buf) return;
#ifdef DEBUG_VERBOSE
  debug_msg("(DLocalDrive found a finder.dat, reading it in and using it)\n");
#endif
  while(working) {
    size_t ret=fread(buf,1,92,f);
    if(ret!=92) working=false;
    if(buf[0]&&working) {
      en=new DFinderDatEntry;
      if(en) {
        UINT t=0;
        size_t len=buf[0];
        if(len>92) len=92;
        for(t=0;t<len;t++) en->longName[t]=buf[t+1];
        en->longName[len]=0;
        for(t=0;t<4;t++) en->type[t]=buf[t+32];
        for(t=0;t<4;t++) en->creator[t]=buf[t+36];
        for(t=0;t<8;t++) en->shortName[t]=buf[t+80];
        en->shortName[8]='.';
        for(t=0;t<3;t++) en->shortName[t+9]=buf[t+88];
        en->shortName[12]=0;
        // get rid of any trailing spaces or dots
        bool nonSpace=false;
        for(t=11;t>8;t--) {
          if(en->shortName[t]==' ') en->shortName[t]=0;
          else nonSpace=true;
        }
        if(!nonSpace) en->shortName[8]=0;
        finderDat.append((void *)en);
        //debug_msg("'"); debug_msg(en->longName); debug_msg("' -> '"); debug_msg(en->shortName); debug_msg("'\n");
      }
    }
  }
}
#endif // USE_FINDER_DAT


// TODO: Since the finder.dat is only read from the current dir, it
// doesn't work for any full pathnames, only files in the current dir!
const char *DLocalDrive::getShortName(const char *longName)
{
  if(!longName) { debug_msg("DLocalDrive::NULL longName!\n"); return longName; }
#ifdef USE_FINDER_DAT
  if(!gotFinderDat) return longName;
#endif // USE_FINDER_DAT
  char tlongName[DDISK_MAX_NAMELEN];
  tlongName[0]=0;
  DSHELPER_disk.fillInNameFromFullName(tlongName,longName);
#ifdef USE_FINDER_DAT
  if(!ignoreFinderDat) {
#ifdef DEBUG_VERBOSE
    debug_msg("Looking in finder.dat for "); debug_msg(tlongName); debug_msg("...\n");
#endif
    finderDat.jumpToHead();
    if(finderDat.atEnd()) {
      // Nothing in finderDat...could use AApp::getShortName here...
      return longName;
    }
    DFinderDatEntry *en=(DFinderDatEntry *)NULL;
    bool working=true;
    while(working) {
      if(finderDat.atEnd()) working=false;
      else {
        en=(DFinderDatEntry *)finderDat.info();
        if(en->longName[0]) {
          if(!strcmp(tlongName,en->longName)) return en->shortName;
        }
        finderDat.advance();
      }
    }
  }
#endif // USE_FINDER_DAT
  debug_msg("(Didn't find it)\n");
  return longName;
}


void *DLocalDrive::Fopen(const char *filename,const char *mode)
{
  if(!filename) { debug_msg("DLocalDrive::NULL fname!\n"); return NULL; }
  if(!mode) { debug_msg("DLocalDrive::NULL mode!\n"); return NULL; }
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::fopen("); debug_msg(filename); debug_msg(","); debug_msg(mode); debug_msg(")\n");
#endif
  void *ret=NULL;
  void *fd=NULL;
#ifdef USE_FAKE_CHDIR
#endif // USE_FAKE_CHDIR
  fd=(void *)fopen(filename,mode);
#ifdef USE_FINDER_DAT
  if(!fd) {
    // Look and see if we can figure out the "short name" by using a finder.dat file...
    // NOTE: This lets you use long filenames even on 16-bit dos if you've got a finder.dat
    if(!gotFinderDat) return NULL;
#ifdef USE_FAKE_CHDIR
#endif // USE_FAKE_CHDIR
    fd=(void *)fopen(getShortName(filename),mode);
  }
#endif // USE_FINDER_DAT
  if(fd) {
    ret=fd;
  }
  return ret;
}


UINT DLocalDrive::Fclose(void *fp)
{
  if(!fp) { debug_msg("DLocalDrive::NULL fp!\n"); return 0; }
  return fclose((FILE *)fp);
}


size_t DLocalDrive::Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::Fread(ptr,"); debug_int(sizelem); debug_msg(","); debug_int(n); debug_msg(",fp)\n");
#endif
  return fread(ptr,sizelem,n,(FILE *)fp);
}


size_t DLocalDrive::Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::Fwrite(ptr,"); debug_int(sizelem); debug_msg(","); debug_int(n); debug_msg(",fp)\n");
#endif
  return fwrite(ptr,sizelem,n,(FILE *)fp);
}


int DLocalDrive::Fseek(void *fp,long offset,int origin)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::Fseek(fp,"); debug_int(offset); debug_msg(","); debug_int(origin); debug_msg(")\n");
#endif
#ifdef ASYS_PALM
  return fseek((FILE *)fp,offset,(FileOriginEnum)origin);
#else
  return fseek((FILE *)fp,offset,origin);
#endif // ASYS_PALM
}


long DLocalDrive::Ftell(void *fp)
{
  return ftell((FILE *)fp);
}


int DLocalDrive::Fstat(int fd,struct stat *st)
{
  // Because some compilers (mwerks) don't have it...
  return fstat(fd,st);
}


int DLocalDrive::Stat(const char *filename,struct stat *st)
{
  return stat(filename,st);
}


#ifdef ASYS_MAC
#ifdef ALIB_MAC_MOREFILES
// prototype...
pascal void myIterateFilterProc(const CInfoPBRec * const cpbPtr, Boolean *quitFlag,
  void *yourDataPtr);
// the actual function...
pascal void myIterateFilterProc(const CInfoPBRec * const cpbPtr, Boolean *quitFlag,
  void *yourDataPtr)
{
  AList_disk *theDir=(AList_disk *)yourDataPtr;
  UINT fType=DDISK_TYPE_NONE;
  UINT theSize=0;
  DDirEntry *de=new DDirEntry;
  StringPtr theName=(StringPtr)NULL;
  if(cpbPtr->hFileInfo.ioFlAttrib&0x10) {
    // a folder
    theName=cpbPtr->dirInfo.ioNamePtr;
    fType=DDISK_TYPE_DIR;
  }
  else
  {
    // a file
    theName=cpbPtr->hFileInfo.ioNamePtr;
    fType=DDISK_TYPE_NONE;
    theSize=cpbPtr->hFileInfo.ioFlLgLen+cpbPtr->hFileInfo.ioFlRLgLen;
  }
  // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN bytes.
  for(UINT t=0;t<theName[0];t++)
    de->name[t]=theName[t+1];
  de->name[theName[0]]=0;
  de->type=fType;
  de->nativeType=cpbPtr->hFileInfo.ioFlAttrib;
  de->size=theSize;
  //debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
  theDir->insert(de);
}
#endif // ALIB_MAC_MOREFILES
#endif // ASYS_MAC


bool DLocalDrive::readDirectory()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DLocalDrive::readDirectory()...\n");
  debug_msg("cwd is "); debug_msg(Getcwd(NULL,0));  debug_msg("\n");
#endif
  freeDirCache();
  if(!cachesDir) {
    debug_msg("(cachesDir is false)\n");
	return false;
  }
  const char *sname=Getcwd(NULL,0);
  for(int t=0;t<strlen(sname);t++) diskName[t]=sname[t];
  diskName[strlen(sname)]=0;
  bool done=false;
  bool warnEm=true;
  DDirEntry *de=(DDirEntry *)NULL;
  UINT fType=DDISK_TYPE_NONE;
#ifdef ASYS_MAC
    debug_msg("Going to try mac style directory calls...\n");
#ifdef ALIB_MAC_MOREFILES
    debug_msg("Going to try morefiles...\n");
  OSErr ferr=IterateDirectory((short)0,(long)0,(unsigned char *)nil,(unsigned short)1,myIterateFilterProc,(void *)&theDir);
  if(err) debug_msg("IterateDirectory returned "); debug_msg(err); debug_msg("\n");
  done=true;
#endif // ALIB_MAC_MOREFILES
#endif // ASYS_MAC
#ifdef ASYS_PC_BASED
    debug_msg("Going to try pc style directory calls...\n");
#ifdef __SC__
  // TODO: with Symantec C++, findfirst makes a DOS call, so we'll only get the
  // short name if we use this on Windows...should special case Win32...
  FIND *theSearchFor=findfirst("*.*",_A_HIDDEN|_A_SYSTEM|_A_SUBDIR);
  while(theSearchFor) {
    //printf("%s\n",theSearchFor->name);
    de=new DDirEntry;
    // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN bytes.
    strcpy(de->name,theSearchFor->name);
    fType=DDISK_TYPE_NONE;
    if(theSearchFor->attribute&_A_SUBDIR) fType=DDISK_TYPE_DIR;
    de->type=fType;
    de->nativeType=theSearchFor->attribute;
    de->size=theSearchFor->size;
    //
    //debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->size); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
    //debug_msg("File: "); debug_msg(theSearchFor->name); debug_msg(" "); debug_msg((int)theSearchFor->attribute); debug_msg("\n");
    //
    theDir.insert(de);
    theSearchFor=findnext();
  }
  done=true;
#endif // __SC__
#ifdef _MSC_VER
    debug_msg("Going to try microsoft style directory calls...\n");
#ifndef ASYS_WIN32CE
  _finddata_t theSearchFor;
  uint32_t fhandle=_findfirst("*.*",&theSearchFor);
  bool more=true;
  while(more) {
    //printf("%s\n",theSearchFor.name);
    de=new DDirEntry;
    // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN bytes.
    strcpy(de->name,theSearchFor.name);
    fType=DDISK_TYPE_NONE;
    if(theSearchFor.attrib&_A_SUBDIR) fType=DDISK_TYPE_DIR;
    de->type=fType;
    de->nativeType=theSearchFor.attrib;
    de->size=theSearchFor.size;
    //debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->size); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
    //debug_msg("File: "); debug_msg(theSearchFor.name); debug_msg(" "); debug_msg((int)theSearchFor.attrib); debug_msg("\n");
    theDir.insert(de);
    UINT ret=_findnext(fhandle,&theSearchFor);
        if(ret) more=false;
  }
  done=true;
#endif // ASYS_WIN32CE
#endif // _MSC_VER
#ifdef ASYS_WIN32
  if(!done) {
    debug_msg("Going to try windows style directory calls...\n");
    WIN32_FIND_DATA theSearchFor;
    HANDLE fhandle=FindFirstFile("*.*"),&theSearchFor);
    bool more=true;
    while(more) {
      //printf("%s\n",theSearchFor.name);
      de=new DDirEntry;
      // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN bytes.
      strcpy(de->name,theSearchFor.cFileName);
      fType=DDISK_TYPE_NONE;
      if(theSearchFor.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
        fType=DDISK_TYPE_DIR;
      de->type=fType;
      de->nativeType=theSearchFor.dwFileAttributes;
      de->size=theSearchFor.nFileSizeLow;  // TODO: Lost high 32 bits here
      debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->size); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
      //debug_msg("File: "); debug_msg(theSearchFor.name); debug_msg(" "); debug_msg((int)theSearchFor.attrib); debug_msg("\n");
      theDir.insert(de);
      UINT ret=FindNextFile(fhandle,&theSearchFor);
      if(!ret) more=false;
    }
    done=true;
  }
#endif // ASYS_WIN32
#endif // ASYS_PC_BASED
#ifdef ASYS_PALM
    debug_msg("Going to try palm style directory calls...\n");
  UINT numdb=DmNumDatabases(0);  // 0 is the card number...
  // How can I tell how many cards there are?
  for(UINT t=0;t<numdb;t++) {
    LocalID lid=DmGetDatabase(0,t);
    Err err=0;
    if(lid) {
      de=new DDirEntry;
      UInt attr=0;
      err=DmDatabaseInfo(0,lid,(CharPtr)&(de->name),&attr,
        NULL,NULL,
        NULL,NULL,
        NULL,NULL,
        NULL,NULL,
        NULL);
      // TODO: check err
      fType=DDISK_TYPE_NONE;
      de->type=fType;
      de->nativeType=attr;
      ULong dsize=0;
      err=DmDatabaseSize(0,lid,NULL,NULL,&dsize);
      // TODO: check err
      de->size=dsize;
      //debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->size); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
      theDir.insert(de);
    }
  }
  done=true;
#endif // ASYS_PALM
  if(!done) {
#ifdef DEBUG_VERBOSE
    debug_msg("Going to try POSIX style directory calls...\n");
#endif
#ifdef ALIB_HAVE_DIRENT_H
#ifdef ALIB_STRUCT_DIRECT_NOT_DIRENT
    direct **theDirents=(direct **)NULL;
#else
    dirent **theDirents=(dirent **)NULL;
#endif // ALIB_STRUCT_DIRECT_NOT_DIRENT
#endif // ALIB_HAVE_DIRENT_H
#ifdef ALIB_HAVE_DIRECT_H
#ifdef ALIB_HAVE_DIRENT
    dirent **theDirents=(dirent **)NULL;
#endif // ALIB_HAVE_DIRENT
#ifdef ALIB_HAVE_DIRECT
    direct **theDirents=(direct **)NULL;
#endif // ALIB_HAVE_DIRECT
#endif // ALIB_HAVE_DIRECT_H
    int n=0;
#ifdef ALIB_HAVE_SCANDIR
#ifdef DEBUG_VERBOSE
    debug_msg("About to scandir...\n");
#endif
    n=scandir(".",&theDirents,/*(int (*)(...))*/NULL,
#ifdef ALIB_HAVE_ALPHASORT
      /*(int(*)(...))*/alphasort
#else
      NULL
#endif // ALIB_HAVE_ALPHASORT
    );
#endif // ALIB_HAVE_SCANDIR
    if(n) {
#ifdef DEBUG_VERBOSE
      debug_msg("Got "); debug_int(n); debug_msg(" from dir...\n");
      if(n==-1) n=0;
#endif
      while(n) {
        n--;
        //printf("%s\n",theDirents[n]->d_name);
        de=new DDirEntry;
#ifdef ALIB_HAVE_DIRENT
        // TODO: problem if filename is longer than DDISK_MAX_NAMELEN bytes.
        strcpy(de->name,theDirents[n]->d_name);
#endif // ALIB_HAVE_DIRENT
        fType=DDISK_TYPE_NONE;
        int r=1;
#ifdef ALIB_HAVE_STAT
        struct stat s;
        //debug_msg("Gonna awstat...\n");
        r=stat(de->name,&s);
        //debug_msg("Back from awstat.\n");
        if(!r) {
          if(s.st_mode&S_IFDIR) fType=DDISK_TYPE_DIR;
          //if(S_ISDIR(s.st_mode)) fType=DDISK_TYPE_DIR;
          if(s.st_mode&S_IFCHR) fType=DDISK_TYPE_DEVICE;
          //if(S_ISCHR(s.st_mode)) fType=DDISK_TYPE_DEVICE;
                  //if(S_ISBLK(s.st_mode)) fType=DDISK_TYPE_DEVICE;
        }
#endif // ALIB_HAVE_STAT
        de->type=fType;
#ifdef ALIB_HAVE_DIRENT
#ifdef ALIB_HAVE_DIRENT_D_TYPE
        de->nativeType=theDirents[n]->d_type;
#else
        de->nativeType=0;
#endif // ALIB_HAVE_DIRENT_D_TYPE
#endif // ALIB_HAVE_DIRENT
        de->size=getFileSize(de->name);
        //debug_msg("File: "); debug_msg(de->name); debug_msg(" "); debug_msg(de->size); debug_msg(" "); debug_msg(de->type); debug_msg("\n");
        //debug_msg("File: "); debug_msg(theDirents[n]->d_name); debug_msg(" ";
        //debug_msg(theDirents[n]->d_type); debug_msg("\n");
        theDir.insert(de);
      }
      done=true;
    }
  }
  if(!done) {
    char msg[256];
#ifdef UNICODE
        sprintf(msg,"%hs %d: Need to implement readDirectory!\n",__FILE__,__LINE__);
#else
        sprintf(msg,"%s %d: Need to implement readDirectory!\n",__FILE__,__LINE__);
#endif // UNICODE
    if(warnEm) debug_msg(msg);
    else debug_msg(msg); debug_msg("\n");
    cachesDir=false;
    return false;
  }
#ifdef DEBUG_VERBOSE
  debug_msg("Done.\n");
#endif
  return true;
}


void *DLocalDrive::Open(const char *name,UINT mode)
{
#ifdef DDISK_GLOBAL_UNIX_FILEIO
  void *ret=NULL;
  char *sname=strdups(name);
  int f=::open(sname,mode);
  if(f!=-1) ret=(void *)f;
  return ret;
#else
  const char *cmode="rb";
  return Fopen(name,cmode);
#endif // DDISK_GLOBAL_UNIX_FILEIO
}


UINT DLocalDrive::Close(void *fp)
{
#ifdef DDISK_GLOBAL_UNIX_FILEIO
  return ::close((int)fp);
#else
  return Fclose(fp);
#endif // DDISK_GLOBAL_UNIX_FILEIO
}


size_t DLocalDrive::Read(void *fp,uint8_t *ptr,size_t n)
{
#ifdef DDISK_GLOBAL_UNIX_FILEIO
  return ::read((int)fp,(char *)ptr,n);
#else
  return Fread(ptr,n,1,fp);
#endif // DDISK_GLOBAL_UNIX_FILEIO
}


size_t DLocalDrive::Write(void *fp,uint8_t *ptr,size_t n)
{
#ifdef DDISK_GLOBAL_UNIX_FILEIO
  return ::write((int)fp,(char *)ptr,n);
#else
  return Fwrite(ptr,n,1,fp);
#endif // DDISK_GLOBAL_UNIX_FILEIO
}


int DLocalDrive::Lseek(void *fp,long offset,int origin)
{
#ifdef DDISK_GLOBAL_UNIX_FILEIO
  return ::lseek((int)fp,offset,origin);
#else
  return Fseek(fp,offset,origin);
#endif // DDISK_GLOBAL_UNIX_FILEIO
}


bool DLocalDrive::Chdir(const char *dname)
{
  if(!dname) return false;
  // TODO: update curDir...
#ifdef USE_FAKE_CHDIR
  debug_msg("(Using fake chdir)\n");
  return DDisk::Chdir(dname);
#else
  const char *sname=strdup(dname);
  bool ret=false;
  //debug_msg("Going to chdir to '");  debug_msg(sname); debug_msg("'...\n");
  chdir(sname);
  // TODO: Check return val of chdir!!!
  ret=true; // assumed for now...
  for(int t=0;t<strlen(sname);t++) diskName[t]=sname[t];
  diskName[strlen(sname)]=0;
  refreshBuffers();
  return ret;
#endif // USE_FAKE_CHDIR
}


const char *DLocalDrive::Getcwd(char *buf,size_t len)
{
  //debug_msg("(DLocalDrive::Getcwd()...)\n");
#ifdef USE_FAKE_CHDIR
  return DDisk::Getcwd(buf,len);
#else
  return getcwd(buf,len);
#endif // USE_FAKE_CHDIR
}


bool DLocalDrive::myCopyFile(const char *oldname,const char *newname)
{
  if(!oldname) return false;
  if(!newname) return false;
  bool retVal=false;
  debug_msg("myCopyFile "); debug_msg(oldname); debug_msg(" to "); debug_msg(newname); debug_msg("\n");
  return retVal;
}


int DLocalDrive::Rename(const char *oldname,const char *newname)
{
  if(!oldname) return -1;
  if(!newname) return -1;
  bool retVal=-1;
#ifdef DEBUG_VERBOSE
  debug_msg("Rename "); debug_msg(oldname); debug_msg(" to "); debug_msg(newname); debug_msg("\n");
#endif
#ifdef ASYS_WIN32
  retVal=MoveFile(oldname,newname);
#endif // ASYS_WIN32
#ifdef __unix__
#ifdef ALIB_HAVE_RENAME
  if(::rename(oldname,newname)) retVal=0;
#endif // ALIB_HAVE_RENAME
#endif // __unix__
#ifdef ASYS_PALM
  // TODO:
#endif // ASYS_PALM
  return retVal;
}


bool DLocalDrive::Unlink(const char *filename)
{
  if(!filename) return false;
  bool retVal=false;
#ifdef DEBUG_VERBOSE
  debug_msg("Unlink "); debug_msg(filename); debug_msg("\n");
#endif
#ifdef ASYS_WIN32
  if(isDir(filename)) {
    removeAllFilesinFolder(filename);
    retVal=RemoveDirectory(filename);
  }
  else retVal=DeleteFile(filename);
#endif // ASYS_WIN32
#ifdef __unix__
  if(!unlink(filename)) retVal=true;
#endif // __unix__
#ifdef ASYS_PALM
  // TODO:
#endif // ASYS_PALM
  return retVal;
}


const char *DLocalDrive::getDiskName()
{
  const char *sname=Getcwd(NULL,0);
  for(int t=0;t<strlen(sname);t++) diskName[t]=sname[t];
  diskName[strlen(sname)]=0;
  return diskName;
}



