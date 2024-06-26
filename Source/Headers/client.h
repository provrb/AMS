/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       client.h
 * @brief      control everything about clients
 * 
 * @note       
 * @history:
 *   Version   Date            Author          Modification    Email
 *   V1.0.0    Jun-05-2024     Ethan Oliveira                  ethanjamesoliveira@gmail.com
 * 
 * @verbatim
 * ==============================================================================
 * 
 * ==============================================================================
 * @endverbatim
 * ****************************(C) COPYRIGHT 2023 ****************************
 */

#ifndef __CLIENT_H__
#define __CLIENT_H__

#define COMMAND_PREFIX "--"

#include "server.h"
#include "browser.h"
#include "backend.h"
#include "root.h"

/*
    The local client.

    Describes the user who is running the application.
*/
extern User*  localClient; // Client struct

/*
    Allocate memory for the 'localClient'

    Without this a segmentation fault is bound
    to happen. Must be called before any work
    is done to the localClient
*/
void MallocLocalClient(); 

/*
    Experimental
*/
void ReceiveRootRequestsAsClient();

/*
    Disconnect the client safely.

    Removes the client from any connected servers,
    the root server, closes any of the clients sockets
    and cleans up anything allocated for that client/
*/
void DisconnectClient(); // Disconnect from the root server

/*
    FInd the index of a client in an array of 'User' structs

    For example, getting index of client from a servers client list.
    'size' is the size of the array.
    'value' is the user who's index to find in 'arr' 
*/
size_t ClientIndex(User* arr[], size_t size, User* value);

/*
    Receive messages from other clients on a server.

    Message can either be a command or a message
    to print on the console.
    Local client must be connected to the server in
    'serverInfo' for the function to work.    
*/
void ReceivePeerMessagesOnServer(void* serverInfo); // Receive messages from other clients in the server

/*
    Assign a default client username 
    based on the time they logged on the application.

    If the client does not select a name, this will 
    be their username throughout the lifetime of their app.
*/
void AssignDefaultHandle(char defaultName[]); // Default Client Username

/*
    Make the client choose a username.

    Take input from the client and let them
    select a username they will use throughout
    their stay on the app.
*/
void ChooseClientHandle(); // Select your username

/*
    Handle console commands entered by the client

    Perform any commands the client enters.
    If a command doesn't exist print an error
    message.
*/
void* HandleClientInput(); // handle commands entered by client


/*
    Perform commands sent by the client while in a server

    For example, if a client is in a server and they type --leave,
    handle the command and make the client leave the server 
*/
int PerformClientSideServerCommands(char* message);

/*
    Reset the clients connection info.

    This will reset their:
    1. Connected server
    2. Client file descriptor
    3. Root file descriptor will be set to the rootServers server file descriptor
    4. Client address info
    5. Time joined
*/
int DefaultClientConnectionInfo();


/*
    Get a user from the cuurent connected server client list
    by using the clients username

    O(n) since we iterate through the client list looking for
    a match.
    ( too lazy to implement hashmap or make it O(1) )
*/
User CSClientFromName(char* username);

/*
    Decrypt a message received by a peer client in the same server.

    All CMessage.message's are encrypted on the server.
    When a client receives it, it must be decrypted
    to be readable.
*/
CMessage DecryptPeerMessage(CMessage* message);

/*
    Make a request from the localClient to their
    currently connected server.
    
    Tell the server to perform 'command'.
    'optionalClientMessage' is used if you want to
    send a normal message to other peer clients in 'server'
*/
ResponseCode MakeServerRequest(
    CommandFlag command,
    User        requestMaker,
    CMessage    optionalClientMessage
);

/*
    Connect the local client to the root server.

    Must be called so that the program can resume
    and the client can take and make requests.
*/
int ConnectToRootServer();

/*
    Disconnect from localClients connected server.

    Make a server request asking to be kicked
    and update the server list.
*/
void LeaveConnectedServer();

#endif // __CLIENT_H__