
#define DEBUG_OUT std::cout
#define ERR_OUT std::cerr
#define CONS_IN std::cin
#define CONS_OUT std::cout


// View contents of a disk image file


#ifdef __linux__
//#define DEFAULT_FILE "/home/nick/src/nemu/zork1_apple2.dsk"
#define DEFAULT_FILE "/home/nick/src/nemu/zork1_c64.d64"
#endif // __linux__

#ifdef __APPLE__
#define DEFAULT_FILE "/Users/nick/src/nemu/zork1_apple2.dsk"
//#define DEFAULT_FILE "/Users/nick/src/nemu/zork1_c64.d64"
#endif // __APPLE__

#include <iostream>

#include "DSmartDisk.h"


const char *programName=NULL;
const char *theFilename=NULL;
const char *theCommand=NULL;


int doDir();

int doDir(DDisk *myDisk)
{
  console_msg("**** Going to get theDir...\n");
  DDirEntry *theFile=NULL;
  AList_disk *theDir=myDisk->getTheDir();
  if(!theDir) { 
    console_msg(programName); console_msg(": theDir was NULL!\n"); 
    return EXIT_FAILURE;
  }
  theDir->jumpToHead();
  // Check if first entry is good...
  theFile=(DDirEntry *)theDir->info();
  if(!theFile) {
    console_msg(programName); console_msg(": first dir entry was NULL!\n"); 
  }
  //
  int n=0;
  bool working=true;
  while(working) {
    theFile=(DDirEntry *)theDir->info();
    if(theFile) {
      if(!theFile->sanityCheck())  {
        console_msg(programName); console_msg(": theFile failed sanity check!\n"); 
        return EXIT_FAILURE;
      }
      if(n<10) console_msg(" ");
      console_int(n);
      console_msg(" '");
      if(theFile->name[0]) console_msg(theFile->name); else console_msg("???No filename???");
      console_msg("' ");
      console_long(theFile->size);
      console_nl();
    }
    else {
      // NOTE: This currently always happens after the last file...
      //console_msg(programName); console_msg(": theDir->info() was NULL!\n"); 
    }
    n++;
    if(theDir->atEnd()) working=false;
    else theDir->advance();
  }
  return EXIT_SUCCESS;
}


int main(int argc, char **argv)
{
  //
  //AWindow w;
  //AWindowOutputStream wos(&w);
  //
  programName=argv[0];
  //
  if(argc!=2) {
#ifdef DEFAULT_FILE
    theFilename=(char *)DEFAULT_FILE;
#else
    console_msg(programName); console_msg(": Usage: "); console_msg(programName); console_msg(" disk_image_file\n");
    return 0;
#endif // DEFAULT_FILE
  }
  else {
    theFilename=argv[1];
  }
  if(!theFilename) { console_msg(programName); console_msg(": Usage: "); console_msg(programName); console_msg(" disk_image_file\n"); return EXIT_SUCCESS; }
  //
  console_msg("**** Going to get smartDisk...\n");
  DSmartDisk *smartDisk=new DSmartDisk(theFilename);
  if(smartDisk->err.getError()) { console_msg(""); console_msg(argv[0]); console_msg(": error creating smartDisk!\n"); return EXIT_FAILURE; }
  //
  console_msg("**** Going to get myDisk...\n");
  DDisk *myDisk=smartDisk->getTheDisk();
  if(!myDisk) { console_msg(programName); console_msg(": couldn't get myDisk!\n"); return 0; }
  if(myDisk->err.getError()) { console_msg(programName); console_msg(": error getting myDisk!\n"); return EXIT_FAILURE; }
  if(!myDisk->sanityCheck())  { console_msg(programName); console_msg(": myDisk failed sanity check!\n"); return EXIT_FAILURE; }
  console_msg("Filesystem is '"); console_msg(myDisk->getName()); console_msg("'\n");
  console_msg("Disk name is '"); console_msg(myDisk->getDiskName()); console_msg("'\n");
  //
  //console_msg("**** Going to debugDump...\n");
  //myDisk->debugDump();
  //
  theCommand="dir"; // For now...
  //
  if(!strcmp(theCommand,"dir")) return doDir(myDisk);
  //
  return EXIT_FAILURE;
}
