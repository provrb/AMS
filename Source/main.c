#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "Headers/server.h"
#include "Headers/client.h"

#ifdef _WIN32
#include <Windows.h>
#endif

/*
    Allocate memory and load client.
    Connect to the root server.
    Load user interface for client
*/
int main() 
{
    /*
        Setup base client struct.
        Allocate memory and assign a default username
    */ 
    MallocLocalClient(); 
    AssignDefaultHandle(localClient->handle);

    // Just a procedure to set saying the client
    // hasnt loaded anything really
    // useful if they disconnect without being setup
    localClient->cfd = -99;
    localClient->rfd = -99;

    /* 
        Connect interrupted terminal input (i.e Ctrl+C) 
        to DisconnectClient() to make sure client gets
        safely disconnected and resources are freed
    */
    signal(SIGINT, DisconnectClient);
    signal(SIGTERM, DisconnectClient);
    
    /* 
        Get the client to choose their username
    */
    ChooseClientHandle();
    
    printf("\nWelcome to AMS, %s\n", localClient->handle);

    /* 
        Connect to root server
    */
    if (ConnectToRootServer() != 0) {
        printf("Aborting... Done\n"); 
        return -1;
    }

    /*
        Make a thread to load the client ui 
    */
    cpthread tinfo = cpThreadCreate(LoadClientUserInterface, NULL);
    
    // cpThreadCreate(ReceiveRootRequestsAsClient, NULL);
        // DisconnectClient(localClient);
    cpThreadJoin(tinfo);
    // pthread_join(pid, NULL);

    return 0;
}