/*
 * Author: Alexander Dibb
 * Version: 1.0
 * Last Edited: 5/26/2017
 * 
 * Driver interface for the network device and student solution for 
 * the NetworkDriver project for Spring the 2017 class of CIS 415. 
 * 
 * (De)multiplexor for a simulated network. Enables both blocking
 * and nonblocking versions of packet send and receive. 
 */

/*-----------------------------------------------------------------*/
/*------------------------Included Headers-------------------------*/
/*-----------------------------------------------------------------*/

// For logging
#include <stdio.h>

// For threads
#include <pthread.h>

// For argument parsing in the logging functions
#include <stdarg.h>

// For storing incoming and outgoing packets on nonblocking calls
#include "BoundedBuffer.h"

// For the packet descriptors themselves
#include "packetdescriptor.h"

// For the network gateway to send/receive packets
#include "networkdevice.h"

// For creating and utilizing the Free Packet Descriptor Store
#include "freepacketdescriptorstore__full.h"

// For initializing the Free Packet Descriptor Store
#include "packetdescriptorcreator.h"

// For operating the network device
#include "networkdevice.h"

// For distinguishing between different client applications
#include "pid.h"

/*-----------------------------------------------------------------*/
/*----------------------Forward Declarations-----------------------*/
/*-----------------------------------------------------------------*/

/* Forward declarations of public functions */
#include "networkdriver.h"

/* Forward declarations of private functions */

/* Thread functions */
void *snd_func(void *);
void *rcv_func(void *);

/* Logging functions */
void log_info(const char *, ...);
void log_err(const char *, ...);

/*-----------------------------------------------------------------*/
/*----------------------------Globals------------------------------*/
/*-----------------------------------------------------------------*/

/* Location of the Free Packet Descriptor Store */
FreePacketDescriptorStore *FPDS;

/* Location of the Network Device */
NetworkDevice *ND;

/* Pointers to sending and receiving threads */
pthread_t *SND;
pthread_t *RCV;

/* Pointers for send and receive bounded queues */
BoundedBuffer *SND_BUF;
BoundedBuffer *RCV_BUF[MAX_PID];

/* Pointer for the packet descriptor waiting for the network */
PacketDescriptor *DEST;

/* Indicator for whether the driver has been initialized yet */
int INIT = 0;

/* Logging toggles */
int LOG_INFO = 1;
int LOG_ERR = 1;

/*-----------------------------------------------------------------*/
/*-------------------Public Function Definitions-------------------*/
/*-----------------------------------------------------------------*/

/*
 * Initializes critical data structures and spins up necessary 
 * threads. Must be called before any other function for the network
 * driver to work. 
 * 
 * The call fails if the network device cannot be resolved or the 
 * Free Packet Descriptor Store cannot be initialized. If it fails,
 * the reason is printed to standard error.
 * 
 * PARAMS: 
 *     NetworkDevice *nd
 *         The address of the network device that the driver will use
 *         to send and receive packets from the network. 
 *     void *mem_start
 *         The address of the memory block for the Free Packet 
 *         Descriptor Store. The call will fail if the pointer 
 *         prevents the Free Packet Descriptor Store from 
 *         initializing itself when passed.
 *     unsigned long mem_length
 *         The length of the block to be assigned to the Free Packet
 *         Descriptor Store. May indirectly result in a failure if 
 *         not enough memory is allocated for the driver to function
 *         effectively. 
 *     FreePacketDescriptorStore **fpds
 *         A pointer to the location in memory the Free Packet 
 *         Descriptor Store will be placed. Modified and returned
 *         rather than used as passed. 
 */
void init_network_driver(NetworkDevice              *nd,
                         void                       *mem_start,
                         unsigned long              mem_length,
                         FreePacketDescriptorStore  **fpds)
{
    log_info("Initializing with ND %p, FPDS %p of %d", nd, mem_start, mem_length);
    
    /* Track returns and throwaway index */
    int rc, i;
    
    /* Create the FPDS */
    FPDS = create_fpds();
    create_free_packet_descriptors(FPDS, mem_start, mem_length);
    
    log_info("Created FPDS at %p", FPDS);
    
    /* Create the buffers */
    SND_BUF = createBB(10);    
    if (SND_BUF == NULL)
    {
        log_err("Could not created buffers for sending packets.");
        return;
    }
    
    for (i = 0; i < MAX_PID; i++)
    {
        RCV_BUF[i] = createBB(2);
        if (RCV_BUF[i] == NULL)
        {
            log_err("Could not create one of the buffers for "
                     "receiving packets.");
            return;
        }
    }
    
    /* Store the address of the network device */
    ND = nd;
    
    /* Initialize the packet descriptor waiting for the network */
    rc = nonblocking_get_pd(FPDS, &DEST);
    if(!rc)
    {
        log_err("Could not retrieve initial packet from FPDS.");
        return;
    }
    
    /* Register that packet descriptor with the network device */
    init_packet_descriptor(DEST);
    register_receiving_packetdescriptor(ND, DEST);
    
    /* Spin up the threads */
    rc = pthread_create(SND, NULL, snd_func, NULL);
    if (rc)
    {
        log_err("Could not create sending thread: returned %d;", rc);
        return;
    } 
    
    rc = pthread_create(RCV, NULL, rcv_func, NULL);
    if (rc)
    {
        log_err("Could not create receiving thread: returned %d;", rc);
        return;
    }
    
    /* Everything appears to be in working order, so return */
    *fpds = FPDS;
    INIT = 1;
    return;
}

/*
 * Accepts a packet descriptor for sending. Usually returns promptly,
 * but may delay while it waits for room to appear in the buffers.
 * 
 * PARAMS: 
 *     PacketDescriptor *pd
 *         A pointer to the packet descriptor to send through the 
 *         network device. The call will fail if NULL.
 */
void blocking_send_packet(PacketDescriptor *pd)
{
    if (!INIT)
    {
        log_err("Did not initialize driver!");
        return;
    }
    
    
}

/*
 * Accepts a packet descriptor for sending. Always returns promptly, 
 * but success is not guaranteed. 
 * 
 * PARAMS: 
 *     PackDescriptor *pd
 *         A pointer to the packet descriptor to send through the 
 *         network device. The call will fail if NULL.
 * 
 * RETURN: 
 *     0
 *         Indicates that the packet has successfully been queued for
 *         sending onto the network.
 *     1
 *         Indicates that the packet could not be queued to send onto
 *         the network for some reason. 
 */
int nonblocking_send_packet(PacketDescriptor *pd)
{
    if (!INIT)
    {
        log_err("Did not initialize driver!");
        return;
    }
    
    return 1;
}

/*
 * Retrieves a packet for the caller, blocking it until a packet for
 * them appears in the RCV_BUF.
 * 
 * PARAMS: 
 *     PacketDescriptor **pd
 *         The return proxy for the packet descriptor received. 
 *     PID pid
 *         The PID of the application seeking a packet. 
 */
void blocking_get_packet(PacketDescriptor **pd, PID pid)
{
    if (!INIT)
    {
        log_err("Did not initialize driver!");
        return;
    }
    
    
}

/*
 * Retrieves a packet for the caller, returning promptly. Does not 
 * guarantee receiving a packet.
 * 
 * PARAMS:
 *     PacketDescriptor **pd
 *         The return proxy for the packet descriptor received. 
 *         Remains unmodified if the call was unsuccessful.
 *     PID pid
 *         The PID of the application seeking a packet.
 * 
 * RETURN:
 *     0 
 *         Indicates that a packet was successfully found in the 
 *         receiver buffer and returned.
 *     1
 *         Indicates that no packet for the process was found, or the
 *         call otherwise failed.
 */
int nonblocking_get_packet(PacketDescriptor **pd, PID pid)
{
    if (!INIT)
    {
        log_err("Did not initialize driver!");
        return;
    }
    
    return 1;
}

/*-----------------------------------------------------------------*/
/*-------------------Private Function Definitions------------------*/
/*-----------------------------------------------------------------*/

/*
 * 
 */
void *snd_func(void *args)
{
    return NULL;
}

/*
 * 
 */
void *rcv_func(void *args)
{
    return NULL;
}

/*
 * Logs the passed formatted string to standard output.
 * 
 * Drawn from the Lab 7 code.
 * 
 * PARAMS: 
 *     const char *format
 *         The format of the string to be printed to output. Uses the
 *         rules of standard printf() calls and its sibling 
 *         functions.
 *     ...
 *         Any items that are to be parsed into the printed line as
 *         indicated by the format.
 * 
 * TODO: Make sure the printing is atomic as a whole with a mutex.
 *       Might be threads that need to print to the same log at the
 *       same time, and would interleave.
 */
void log_info(const char *format, ...)
{
    if (!LOG_INFO)
        return;
    
    va_list argptr;
    va_start(argptr, format);
    
    fprintf(stdout,"INFO :\n");
    vfprintf(stdout,format, argptr);
    fprintf(stdout,"\n");
    
    va_end(argptr);
}

/*
 * Logs the passed formatted string to standard error.
 * 
 * Drawn from the Lab 7 code.
 * 
 * PARAMS: 
 *     const char *format
 *         The format of the string to be printed to output. Uses the
 *         rules of standard printf() calls and its sibling 
 *         functions.
 *     ...
 *         Any items that are to be parsed into the printed line as
 *         indicated by the format.
 * 
 * TODO: Make sure the printing is atomic as a whole with a mutex.
 *       Might be threads that need to print to the same log at the
 *       same time, and would interleave.
 */
void log_err(const char *format, ...)
{
    if (!LOG_ERR)
        return;
    
    va_list argptr;
    va_start(argptr, format);
    
    fprintf(stderr,"ERROR :\n");
    vfprintf(stderr,format, argptr);
    fprintf(stderr,"\n");
    
    va_end(argptr);
}
