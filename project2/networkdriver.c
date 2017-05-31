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

/* For logging */ 
#include <stdio.h>

/* For atexit functionality */
#include <stdlib.h>

/* For threads */
#include <pthread.h>

/* For argument parsing in the logging functions */
#include <stdarg.h>

/* For sleep (just for context switches I swear!) */
#include <unistd.h>

/* For storing incoming and outgoing packets on nonblocking calls */
#include "BoundedBuffer.h"

/* For the packet descriptors themselves */
#include "packetdescriptor.h"

/* For the network gateway to send/receive packets */
#include "networkdevice.h"

/* For creating and utilizing the Free Packet Descriptor Store */
#include "freepacketdescriptorstore__full.h"

/* For initializing the Free Packet Descriptor Store */
#include "packetdescriptorcreator.h"

/* For operating the network device */
#include "networkdevice.h"

/* For distinguishing between different client applications */
#include "pid.h"

/*-----------------------------------------------------------------*/
/*----------------------Forward Declarations-----------------------*/
/*-----------------------------------------------------------------*/

/* Forward declarations of public functions */
#include "networkdriver.h"

/* Forward declarations of private functions */

/* Locate a client's buffer based on PID */
int find_client(PID);

/* Thread functions */
void *snd_func(void *);
void *rcv_listen(void *);
void *rcv_process(void *);
void *rcv_upkeep(void *);

/* Logging functions */
void log_info(const char *, ...);
void log_err(const char *, ...);

/* Cleanup function */
void cleanup(void);

/*-----------------------------------------------------------------*/
/*----------------------------Globals------------------------------*/
/*-----------------------------------------------------------------*/

/* UNUSED qualifier */
#define UNUSED __attribute__((unused))

/* Location of the Free Packet Descriptor Store */
FreePacketDescriptorStore *FPDS;

/* Location of the Network Device */
NetworkDevice *ND;

/*
 * Thread IDs for all threads used
 * 
 * SND sends packets from the SND_BUF buffer out onto the network
 * 
 * RCV_LISTEN listens to the network and places packets into an 
 *     intermediate buffer
 * 
 * RCV_PROCESS takes packets from the intermediate buffer RCV_TEMP 
 *     and places them into the right GET_BUF[] element
 * 
 * RCV_UPKEEP maintains the auxiliary PD that the RCV_LISTEN thread
 *     uses to register new packet descriptors quickly.
 */
pthread_t SND;
pthread_t RCV_LISTEN;
pthread_t RCV_PROCESS;
pthread_t RCV_UPKEEP;

/*
 * Pointers for bounded buffers
 * 
 * SND_BUF holds the queued packet descriptors to be sent out
 * 
 * GET_BUF[] holds the array of queued packet descriptors awaiting 
 *     pickup from their application
 * 
 * RCV_TEMP holds the packets waiting for sorting by the RCV_PROCESS
 *     thread
 */
BoundedBuffer *SND_BUF;
BoundedBuffer *GET_BUF[MAX_PID];
BoundedBuffer *RCV_TEMP;

/* Size limitations for bounded buffers */
/* DEVNOTE: Make sure, if changing, that the totals never exceed the 
   amount available in the FPDS. */
const int SND_SIZE          = 10;
const int GET_SIZE          = 2; /* this * MAX_PID possible */
const int RCV_TEMP_SIZE     = 3;

/* Array of client PIDs */
PID CLIENTS[MAX_PID];

/* Pointer for the packet descriptor waiting for the network */
PacketDescriptor *DEST;

/* Pointer for the destination's replacement */
PacketDescriptor *NEXT;

/* Sentinel for whether the driver has been initialized yet */
int INIT = 0;

/* Sentinel for whether it's time to close threads */
int QUIT = 0;

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
    SND_BUF = createBB(SND_SIZE);    
    if (SND_BUF == NULL)
    {
        log_err("Could not create buffers for sending packets.");
        return;
    }
    
    log_info("Created SND_BUF at %p", SND_BUF);
    
    
    for (i = 0; i < MAX_PID; i++)
    {
        /* Initialize both buffer and client PID dummy value */
        
        CLIENTS[i] = 0;
        
        GET_BUF[i] = createBB(GET_SIZE);
        if (GET_BUF[i] == NULL)
        {
            log_err("Could not create one of the buffers for "
                     "receiving packets.");
            return;
        }
    }
    
    log_info("Created GET_BUF at %p", GET_BUF[0]);
    
    
    RCV_TEMP = createBB(RCV_TEMP_SIZE);    
    if (RCV_TEMP == NULL)
    {
        log_err("Could not create buffer for holding received" 
                "packets.");
        return;
    }
    
    log_info("Created RCV_TEMP_BUF at %p", RCV_TEMP);
    
    
    
    /* Store the address of the network device */
    if (nd == NULL)
    {
        log_err("Address of Network Device was null");
        return;
    }
    
    ND = nd;
    
    log_info("Set ND to %p", ND);
    
    
    
    /* Initialize the packet descriptor waiting for the network */
    rc = nonblocking_get_pd(FPDS, &DEST);
    if(!rc)
    {
        log_err("Could not retrieve initial packet from FPDS");
        return;
    }
    
    log_info("Retrieved initial packet from FPDS");
    
    
    
    /* Register that packet descriptor with the network device */
    init_packet_descriptor(DEST);
    register_receiving_packetdescriptor(ND, DEST);
    
    log_info("Registered initial packet for reception");
    
    
    
    /* Spin up the threads */
    rc = pthread_create(&SND, NULL, &snd_func, NULL);
    if (rc)
    {
        log_err("Could not create sending thread: returned %d", rc);
        return;
    } 
    
    log_info("Spun up SND thread");
    
    
    rc = pthread_create(&RCV_LISTEN, NULL, &rcv_listen, NULL);
    if (rc)
    {
        log_err("Could not create listening thread: code %d;", rc);
        return;
    }
    
    log_info("Spun up RCV_LISTEN thread");
    
    
    rc = pthread_create(&RCV_PROCESS, NULL, &rcv_process, NULL);
    if (rc)
    {
        log_err("Could not create processing thread: code %d;", rc);
        return;
    }
    
    log_info("Spun up RCV_PROCESS thread");
    
    
    rc = pthread_create(&RCV_UPKEEP, NULL, &rcv_upkeep, NULL);
    if (rc)
    {
        log_err("Could not create upkeep thread: code %d;", rc);
        return;
    }
    
    log_info("Spun up RCV_UPKEEP thread");
    
    
    /* TODO: Ton of repetition here. Move to functions if time 
       allows. Was OK with early, simpler iterations, but bloated 
       now. */
    
    
    /* Everything appears to be in working order, so finalize */
    *fpds = FPDS;
    /*atexit(cleanup);  Can't use, breaks by closing threads */
    INIT = 1;
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
    
    blockingWriteBB(SND_BUF, pd);
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
        return 1;
    }
    
    int rc = nonblockingWriteBB(SND_BUF, pd);
    if (rc == 1)
    {
        log_info("Could not write packet to buffer (NB)");
        return 1;
    }
    
    log_info("Wrote packet %p to outgoing buffer", pd);
    return 0;
}

/*
 * Retrieves a packet for the caller, blocking it until a packet for
 * them appears in the GET_BUF.
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
    
    int client_index = find_client(pid);
    if (client_index == -1)
    {
        log_err("Bad PID trying to get packet");
        return;
    }
    
    log_info("Packet sent to application %d", pid);
    
    BoundedBuffer *bb = GET_BUF[client_index];
    *pd = (PacketDescriptor *) blockingReadBB(bb);
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
        return 1;
    }
    
    int client_index = find_client(pid);
    if (client_index == -1)
    {
        log_err("Bad PID trying to get packet");
        return 1;
    }
    
    BoundedBuffer *bb = GET_BUF[client_index];
    int rc = nonblockingReadBB(bb, (void **) pd);
    
    if (rc == 1)
    {
        log_info("Could not read packet from buffer (NB)");
        return 1;
    }
    
    log_info("Packet sent to application %d", pid);
    
    return 0;
}

/*-----------------------------------------------------------------*/
/*-------------------Private Function Definitions------------------*/
/*-----------------------------------------------------------------*/

/*
 * Find which index a client occupies for the array of clients and 
 * the array of bounded buffers containing the processes for them.
 * 
 * If it finds a value of 0 in the CLIENTS[] array instead, it 
 * instantiates that PID into CLIENTS[]. 
 * 
 * PARAMS:
 *     PID pid
 *         The PID of the client application.
 * 
 * RETURN:
 *     0 <= r < MAX_PID
 *         The index linking the CLIENTS[] to their GET_BUF[].
 *     -1 
 *         The call failed for some reason, likely due to a bad PID.
 */
int find_client(PID pid)
{
    /* The index */
    int i; 
    
    for (i = 0; i < MAX_PID; i++)
    {
        if (CLIENTS[i] == 0)
        {
            /* Empty spaces - fill with new client */
            CLIENTS[i] = pid;
            return i;
        } 
        else if (CLIENTS[i] == pid) 
        {
            /* Found the client */
            return i;
        }
    }
    
    /* Found neither empty space nor the client - failure */
    return -1;
}

/*
 * Function tethered to the SND thread
 */
void *snd_func(UNUSED void *args)
{    
    /* Take items from the SND_BUF and pass them to the network */
    while (!QUIT)
    {
        /* Wait for the next packet */
        PacketDescriptor *pd = 
            (PacketDescriptor *) blockingReadBB(SND_BUF);
        
        /* Try to send the packet three times */
        int attempts = 0
        int rc = 0;
        while((attempts < 5) && (rc == 0))
        {
            rc = send_packet(ND, pd);
            attempts++;
        }
        
        /* Interpret the results */
        if (rc == 1)
        {
            log_info("Successfully sent packet %p after %d tries",
                     pd,
                     attempts);
        } 
        else 
        {
            log_err("Failed to send packet %p after %d tries", 
                    pd, 
                    attempts);
        }
        
        /* Return packet descriptor to the FPDS */
        blocking_put_pd(FPDS, pd);
    }
    
    /* QUIT has been set, so return to rejoin parent */
    return NULL;
}

/*
 * Function tethered to the RCV_LISTEN thread
 */
void *rcv_listen(UNUSED void *args)
{
    /* Constantly listen to the network for incoming packets */
    while (!QUIT)
    {
        await_incoming_packet(ND);
        
        /* DEVNOTE: KEEP THIS FAST AS POSSIBLE! */
       
        if (nonblockingWriteBB(RCV_TEMP, DEST) == 0)
        {
            /* Writing incoming packet failed, scrub DEST */
            init_packet_descriptor(DEST);
        } 
        else 
        {
            /* Ensure NEXT has gotten a new value */
            while (NEXT == NULL)
            {
                /* Force a context switch */
                sleep(0);
            }
            
            /* Replace the destination, and nullify the next */
            DEST = NEXT;
            NEXT = NULL;
        }
        
        register_receiving_packetdescriptor(ND, DEST);
    }
    
    return NULL;
}

/*
 * Function tethered to the RCV_PROCESS thread
 */
void *rcv_process(UNUSED void *args)
{
    /* Look for and sort any items in the RCV_TEMP buffer */
    while (!QUIT)
    {
        /* Grab the packet from the intermediate buffer */
        PacketDescriptor *pd = 
            (PacketDescriptor *) blockingReadBB(RCV_TEMP);
        
        /* Get its PID to find the right buffer to write to */
        PID pid = packet_descriptor_get_pid(pd);
        int client_index = find_client(pid);
        BoundedBuffer *bb = GET_BUF[client_index];
        
        /* If write fails, overwrite oldest */
        while (nonblockingWriteBB(bb, pd) == 0)
        {
            /* Grab the oldest packet */
            PacketDescriptor *temp = 
                (PacketDescriptor *) blockingReadBB(bb);
            
            /* Return 'dropped' packet to FPDS */
            blocking_put_pd(FPDS, temp);
            
            /* Terminate loop, let while conditional handle it */
        }
    }
    
    return NULL;
}

/*
 * Function tethered to the RCV_UPKEEP thread
 */
void *rcv_upkeep(UNUSED void *args)
{
    /* Constantly try to add new packets from FPDS to RCV_PDS */
    while (!QUIT)
    {
        if (NEXT == NULL)
        {
            blocking_get_pd(FPDS, &NEXT);
            init_packet_descriptor(NEXT);
        }
    }
    
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
 * TODO: Make sure the printing is atomic as a whole (mutex stuff?)
 *       Might be threads that need to print to the same log at the
 *       same time, and would interleave. Fix that sometime.
 */
void log_info(const char *format, ...)
{
    if (!LOG_INFO)
        return;
    
    va_list argptr;
    va_start(argptr, format);
    
    fprintf(stdout,"DRIVER | INFO : ");
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
 * TODO: Same as log_info.
 */
void log_err(const char *format, ...)
{
    if (!LOG_ERR)
        return;
    
    va_list argptr;
    va_start(argptr, format);
    
    fprintf(stderr,"DRIVER | ERROR : ");
    vfprintf(stderr,format, argptr);
    fprintf(stderr,"\n");
    
    va_end(argptr);
}

/*
 * Cleanup function to be called on exiting. Sweeps the threads and
 * heap-allocated items if present.
 */
void cleanup()
{
    /* Throwaway index for looping */
    int i;
    
    /* Set the quitting time sentinel to make threads terminate */
    QUIT = 1;
    
    /* Join the threads */
    pthread_join(SND, NULL);
    pthread_join(RCV_LISTEN, NULL);
    pthread_join(RCV_PROCESS, NULL);
    pthread_join(RCV_UPKEEP, NULL);
    
    /* Clean the bounded buffers */
    destroyBB(SND_BUF);
    destroyBB(RCV_TEMP);
    for (i = 0; i < MAX_PID; i++)
    {
        destroyBB(GET_BUF[i]);
    }
    
    /*
     * DEVNOTE: Here would be a perfect place to destroy the FPDS if
     * necessary, but we pass it back in init_network_driver() so 
     * other programs might still be using it.
     */
}
