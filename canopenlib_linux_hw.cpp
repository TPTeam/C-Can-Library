// The functions contained in this file are pretty dummy
// and are included only as a placeholder. Nevertheless,
// they *will* get included in the shared library if you
// don't remove them :)
//
// Obviously, you 'll have to write yourself the super-duper
// functions to include in the resulting library...
// Also, it's not necessary to write every function in this file.
// Feel free to add more files in this project. They will be
// included in the resulting library.
#include <stdio.h>
#include <string.h>

//#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#include "libpcan.h"
#include "canopenlib_linux_hw.h"



//portato nell header
//extern "C"
//{

#define MAX_CAN_DEVICES     4
#define RX_QUEUE_SIZE       100

typedef struct
{
  canPortHandle handle;
  bool opened;
  bool in_use;
  int echo_enabled;
  int bitrate;
  pthread_mutex_t rx_queue_cs;
  TPCANMsg rx_queue[RX_QUEUE_SIZE];
  int rx_queue_put_pos;
  int rx_queue_get_pos;
  bool can_bus_on;
} CanPortDataStruct;

static CanPortDataStruct can_port_data_devices[ MAX_CAN_DEVICES ];

canOpenStatus putCanMessageInQueue (CanPortDataStruct *p,
                                    TPCANMsg *can_frame)
{
  //printf("putCanMessageInQueue\n");
  canOpenStatus res = CANOPEN_OK;

  //EnterCriticalSection( &p->rx_queue_cs ); // We need to imlplement a "faked" echo mechanism.
  pthread_mutex_lock( &p->rx_queue_cs );

  int put_pos = p->rx_queue_put_pos;

  if ( ++put_pos >= RX_QUEUE_SIZE )
    put_pos = 0;

  if (put_pos != p->rx_queue_get_pos) {
    memcpy( &(p->rx_queue[ p->rx_queue_put_pos ]), can_frame, sizeof(TPCANMsg) );
    p->rx_queue_put_pos = put_pos;
    res = CANOPEN_OK;
  } else {
    res = CANOPEN_ERROR_CAN_LAYER_OVRFLOW;
  }

  //LeaveCriticalSection( &p->rx_queue_cs );
  pthread_mutex_unlock (&p->rx_queue_cs);

  return res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus getCanMessageFromQueue (CanPortDataStruct *p,
                                      TPCANMsg *can_frame)
{
  //printf("getCanMessageFromQueue\n");
    //aggiunto default
  canOpenStatus res = CANOPEN_ERROR_NO_MESSAGE;
  int get_pos = p->rx_queue_get_pos;

  if (get_pos != p->rx_queue_put_pos ) {
    memcpy( can_frame, &(p->rx_queue[ p->rx_queue_get_pos ]), sizeof(TPCANMsg) );
    if ( ++get_pos >= RX_QUEUE_SIZE )
      get_pos = 0;
    p->rx_queue_get_pos = get_pos;
    res = CANOPEN_OK;
  } else {
    res = CANOPEN_ERROR_NO_MESSAGE;
  }

  return res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

void canSinglePortReset(int port)
{
    can_port_data_devices[port].opened = false;
    can_port_data_devices[port].in_use = false;
    can_port_data_devices[port].echo_enabled = false;
    can_port_data_devices[port].can_bus_on = false;

    can_port_data_devices[port].handle = NULL;
    //InitializeCriticalSection( &can_port_data_devices[i].rx_queue_cs );
    //create mutex attribute variable
    pthread_mutexattr_t mAttr;
    // setup recursive mutex for mutex attribute
    pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);
    // Use the mutex attribute to create the mutex
    pthread_mutex_init(&can_port_data_devices[port].rx_queue_cs, &mAttr);
    // Mutex attribute can be destroy after initializing the mutex variable
    pthread_mutexattr_destroy(&mAttr);
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortLibraryInit(void)
{
  //printf("canPortLibraryInit\n");
  canOpenStatus canopen_res = CANOPEN_OK;

  // Make sure that all CAN ports are available.
  for ( int i = 0; i < MAX_CAN_DEVICES; i++ ) {
    canSinglePortReset(i);
  }

  return canopen_res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
canOpenStatus canPortOpen(int port)
{
    char buf[12];
    sprintf(buf, "/dev/pcan%d", port);
    const char *devName = buf;
    return canPortOpenByName(port,devName);
}

canOpenStatus canPortOpenByName(int port, const char *devName)
{
  //printf("canPortOpen\n");
  if ( port >= 0 && port < MAX_CAN_DEVICES ) {
    can_port_data_devices[port].handle = LINUX_CAN_Open(devName, O_RDWR);
    can_port_data_devices[port].opened = true;
    return CANOPEN_OK;
  } else {
    return CANOPEN_ERROR_HW_NOT_CONNECTED;
  }

}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortClose(int port)
{
  //printf("canPortClose\n");
  if (can_port_data_devices[ port ].can_bus_on)
    canPortGoBusOff(port);

  canOpenStatus ret = CANOPEN_ERROR_CAN_LAYER ;
  canSinglePortReset(port);
  ret = CANOPEN_OK;
  return ret;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortBitrateSet(int port, int bitrate)
{
  //printf("canPortBitrateSet\n");
  canOpenStatus canopen_res = CANOPEN_ERROR;
  switch ( bitrate )
  {
    case CAN_BAUD_125K:
      can_port_data_devices[port].bitrate = CAN_BAUD_125K;
      canopen_res = CANOPEN_OK;
      break;
    case CAN_BAUD_250K:
      can_port_data_devices[port].bitrate = CAN_BAUD_250K;
      canopen_res = CANOPEN_OK;
      break;
    case CAN_BAUD_500K:
      can_port_data_devices[port].bitrate = CAN_BAUD_500K;
      canopen_res = CANOPEN_OK;
      break;
    case CAN_BAUD_1M:
      can_port_data_devices[port].bitrate = CAN_BAUD_1M;
      canopen_res = CANOPEN_OK;
      break;
    default:
      canopen_res = CANOPEN_ERROR;
      break;
  }

  return canopen_res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortEcho( int port, bool enabled )
{
  //printf("canPortEcho\n");
  canOpenStatus canopen_res = CANOPEN_OK;
  can_port_data_devices[port].echo_enabled = enabled;
  return canopen_res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortGoBusOn(int port)
{
  //printf("canPortGoBusOn\n");
  canOpenStatus canopen_res = CANOPEN_ERROR;

  if (!can_port_data_devices[ port ].opened)
    return CANOPEN_ERROR;

  DWORD res = CAN_Init(can_port_data_devices[ port ].handle, can_port_data_devices[ port ].bitrate, CAN_INIT_TYPE_EX);
  if ( res == CAN_ERR_OK ) {
    can_port_data_devices[ port ].can_bus_on = true;
    canopen_res = CANOPEN_OK;
  } else {
      printf("Error on bus %i",res);
  }
  return canopen_res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortGoBusOff(int port)
{
  //printf("canPortGoBusOff");
  canOpenStatus canopen_res = CANOPEN_ERROR;

  DWORD res = CAN_Close(can_port_data_devices[ port ].handle);
  if ( res == CAN_ERR_OK ) {
    can_port_data_devices[ port ].opened = false;
    can_port_data_devices[ port ].can_bus_on = false;
    canopen_res = CANOPEN_OK;
  }
  return canopen_res;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

canOpenStatus canPortWrite(             int port,
                                        long id,
                                        void *msg,
                                        unsigned int dlc,
                                        unsigned int flags)
{

  canOpenStatus canopen_res = CANOPEN_CAN_LAYER_FAILED;
  TPCANMsg frame;
  frame.ID = id;
  frame.LEN = dlc;
  frame.MSGTYPE = 0; // Clear all flags.
  if ( flags & CAN_MSG_EXT )
    frame.MSGTYPE = MSGTYPE_EXTENDED;
  if ( flags & CAN_MSG_RTR )
    frame.MSGTYPE = MSGTYPE_RTR;
  else
    memcpy( frame.DATA, msg, dlc);

  //DWORD res = CAN_Write( &handle, &frame );

  if (can_port_data_devices[ port ].opened && can_port_data_devices[ port ].can_bus_on) {
    DWORD res = CAN_Write( can_port_data_devices[ port ].handle, &frame );
    if ( res == CAN_ERR_OK ) {
        if ( can_port_data_devices[ port ].echo_enabled ) {
            (void)putCanMessageInQueue( &can_port_data_devices[ port ], &frame);
        }
        canopen_res = CANOPEN_OK;
    }
    //printf("\nRes of write %d %d \n",res, can_port_data_devices[ port ].echo_enabled );
    return canopen_res;

  } else {
    return CANOPEN_CAN_LAYER_FAILED;
  }
}

//------------------------------------------------------------------------
// non blocking
// to be cleaned more
// blocking again...
//------------------------------------------------------------------------

canOpenStatus canPortRead(              int port,
                                        long *id,
                                        void *msg,
                                        unsigned int *dlc,
                                        unsigned int *flags)
{
  //printf("\ncanPortRead\n");
  canOpenStatus canopen_res = CANOPEN_CAN_LAYER_FAILED;
  TPCANMsg frame;
  //TPCANRdMsg frame;
  DWORD res;
  *flags = 0;
  //printf("\n1\n");
  //First check if you are connected and on
  if (can_port_data_devices[ port ].opened && can_port_data_devices[ port ].can_bus_on) {
    // First check if there are any echos avilable.
    //printf("\n2\n");
    canopen_res = getCanMessageFromQueue( &can_port_data_devices[ port ], &frame );
    //printf("\n--> %d \n",canopen_res);
    if ( canopen_res == CANOPEN_OK ) {
        /*
        TPCANRdMsg tmp_frame;
        //printf("MSG in QUEUE!!!");
        //with 0 it becomes non blocking
        res = LINUX_CAN_Read_Timeout(can_port_data_devices[port].handle, &tmp_frame,0);

        if ( res == CAN_ERR_OK && !( frame.MSGTYPE & MSGTYPE_STATUS ) ) {
            (void)putCanMessageInQueue( &can_port_data_devices[ port ], &tmp_frame.Msg);
        } else {
            res = CANOPEN_ERROR_NO_MESSAGE;
        }*/
    }  else {
        TPCANRdMsg tmp_frame;
        res = LINUX_CAN_Read_Timeout(can_port_data_devices[port].handle, &tmp_frame,-1);
        //res = CAN_Read(can_port_data_devices[port].handle, &frame);
        if ( res == CAN_ERR_OK ) {
            //printf("FRAME RECEIVED!!!\n");
            frame = tmp_frame.Msg;
            if  ( frame.MSGTYPE & MSGTYPE_STATUS  ) {
                res = CANOPEN_ERROR_NO_MESSAGE;
            } else {
                canopen_res = CANOPEN_OK;
            }
        }
    }
    if ( canopen_res == CANOPEN_OK  ) {
        *id = (long)frame.ID;
        *dlc = frame.LEN;
        if ( frame.MSGTYPE & MSGTYPE_RTR )
            *flags = CAN_MSG_RTR;
        if ( frame.MSGTYPE & MSGTYPE_EXTENDED )
            *flags = MSGTYPE_EXTENDED;
        memcpy( msg, frame.DATA, *dlc);
        canopen_res = CANOPEN_OK;
    }

    return canopen_res;
  } else {
    return CANOPEN_CAN_LAYER_FAILED;
  }
}
