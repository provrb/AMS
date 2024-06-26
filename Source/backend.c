/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       backend.cpp
 * @brief      all backend operations
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

#include "Headers/backend.h"
#include "Headers/browser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __unix__
#include <netdb.h>
#endif

/**
 * @brief           Get a Server struct from a server alias(name)
 * @param[in]       alias: Name of the server to get info on
 * @return          Server
 * @retval          Struct of info about the server
 */
Server* ServerFromAlias(char* alias) {
    bool duplicateNames = false;
    int foundServers = 0;
    Server* sarr[] = {};
    Server* foundServer = NULL;

    for (int i=0; i < onlineServers; i++){
        Server* server = &serverList[i];

        if (server->alias != NULL){
            toLowerCase(alias); 
            toLowerCase(server->alias);

            // Compare everything lowercased
            if (strcmp(server->alias, alias) == 0)
            {
                sarr[foundServers] = server;
                foundServers+=1;
                foundServer = server;
            }
        }
    }

    if (foundServers > 1)
    {
        // Ask user which to choose
        SystemPrint(UNDR, true, "Choose Between These %i Servers", foundServers);
        duplicateNames = true;
        Server* server = NULL;

        for (int i = 1; i < foundServers + 1; i++)
        {
            server = sarr[i];
            printf("%i: [%i/%i] Host: %s - Server Name: %s\n", i, server->connectedClients, server->maxClients, server->host.handle, server->alias);
        }

        while (1)
        {
            char option[10];

            printf("Option: ");

            fgets(option, 10, stdin);
            
            // Remove the trailing newline character if it exists
            if (option[strlen(option) - 1] == '\n') {
                option[strlen(option) - 1] = '\0';
            }

            if (atoi(option) <= foundServers)
            {
                foundServer = sarr[atoi(option)];
                

                break;
            }
        }
    }

    return foundServer;
}

/**
 *    --------[ SETTINGS FUNCTIONS ]--------
 * Functions to control the settings
 * for the user. 
 * 
 * Settings reset every session.z
 * 
 */
// void EnableDebugMode(){    
//     DEBUG = !DEBUG;
//     (DEBUG ? SystemPrint(YEL, true, "Debug Mode On.", DEBUG) : SystemPrint(YEL, true, "Debug Mode Off.") ); 
// }

void ExitApp(){ 
    DisconnectClient();
}

/**
 * 
 *      --------[ JOIN FUNCTIONS ]--------         
 * Many different ways a user can join a server
 * Used with commands and arguements.
 * 
 */
void JoinServer(Server* server) {

    // check if there is room to join
    if (server->connectedClients+1 > server->maxClients) {
        SystemPrint(RED, true, "Cannot Join Server. Server Full.");
        return;
    }

    int cfd = socket(server->domain, server->type, server->protocol);
    if (cfd < 0){
        ErrorPrint(true, "Failed To Create Client Socket", "Error while making a client socket for local client to join a server.");
        return;
    }

    int cnct = connect(cfd, (struct sockaddr*)&server->addr, sizeof(server->addr));
    if (cnct < 0){
        ErrorPrint(true, "Failed To Connect To A Server", "Error while connecting local client to a server using connect()");
        return;
    }

    /*
        Pass dereference local client to the server
        we are joining to ask them to add us to the
        client list and return updated information
        about the server were connecting to
    */
    User dereferenceClient = *localClient;
    dereferenceClient.cfd = cfd;
    dereferenceClient.addressInfo = server->addr;
    dereferenceClient.connectedServer = server;

    // Send client info to server you're joining
    int sentBytes = send(cfd, (void*)&dereferenceClient, sizeof(dereferenceClient), 0);
    if (sentBytes <= 0)
    {
        ErrorPrint(true, "Sending Local Client Info To Server", "Failed while sending local clients info to requested server");
        return;
    }

    // Receive most updated server info
    Server updatedServer = {0};
    int recvBytes = recv(cfd, (void*)&updatedServer, sizeof(Server), 0);
    if (recvBytes <= 0)
    {
        ErrorPrint(true, "Receiving Local Client Info From Server", "Failed while receiving updated local client info from requested server");
        return;
    }

    /*
        Update localClient struct.
    */
    localClient->addressInfo = server->addr;
    localClient->cfd = cfd;
    localClient->connectedServer = &updatedServer;

    Chatroom(&updatedServer);
}

void JoinServerByName(char* name){ // Join server from its alias
    UpdateServerList();

    Server* server = ServerFromAlias(name);
    if (!server) { // ServerFromAlias returns null if no server is found
        SystemPrint(YEL, true, "No Server Found With That Name.");
        return;
    }

    JoinServer(server);
}
