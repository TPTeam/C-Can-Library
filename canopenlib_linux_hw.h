
#include "CANopenLibDefines.h"
#include <pthread.h>
#include "libpcan.h"

//per linux sembra corretto così...
//typedef /*int*/void      canPortHandle;
typedef HANDLE canPortHandle;

//
// CAN informaiton flags.
//
#define CAN_MSG_RTR              0x0001      // Message is a remote request
#define CAN_MSG_EXT              0x0002      // Message has a standard ID

extern "C"
{

void           canSinglePortReset(int port);
canOpenStatus  canPortLibraryInit(void);

canOpenStatus  canPortOpenByName( int port, const char *devName);
canOpenStatus  canPortOpen( int port );
canOpenStatus  canPortClose( int port );

canOpenStatus  canPortBitrateSet( int port, int bitrate );

canOpenStatus  canPortEcho( int port, bool enabled );

canOpenStatus  canPortGoBusOn(int port);
canOpenStatus  canPortGoBusOff(int port);

canOpenStatus  canPortWrite(int port,
                                                 long id,
                                                 void *msg,
                                                 unsigned int dlc,
                                                 unsigned int flags);

canOpenStatus  canPortRead(int port,
                                                 long *id,
                                                 void *msg,
                                                 unsigned int *dlc,
                                                 unsigned int *flags);



}

