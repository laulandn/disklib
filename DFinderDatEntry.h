#ifndef DLIB_DFINDERDATENTRY_H
#define DLIB_DFINDERDATENTRY_H


#define USE_FINDER_DAT 1


// Support for MacOS 7-9 PCExchange finder.dat filess
// TODO: Not all there yet


#include "DDisk.h"


#ifdef USE_FINDER_DAT
// Used for handling finder.dat files if we find them
class DFinderDatEntry : public AGenericListNode_disk
{
public:
  DFinderDatEntry();
  //
  char shortName[DDISK_MAX_NAMELEN];
  char longName[DDISK_MAX_NAMELEN];
  char type[4];
  char creator[4];
protected:
  void init();
};
#endif // USE_FINDER_DAT


#endif // DLIB_DFINDERDATENTRY_H

