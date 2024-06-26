/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       browser.c
 * @brief      server browser manager
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

#include "Headers/browser.h"

unsigned int onlineServers = 0;
Server       serverList[kMaxServersOnline] = {0};
Server       backendServerList[1000] = {0};


/**
 * @brief           Add a valid server to server list
 * @param[in]       server: server to add
 * @return          int
 * @retval          success or failure
 */
int SSAddServerToList(Server server) {
    if (server.online == false){
        printf("ERROR: Cannot add offline server to list");
        return -1;
    }

    serverList[onlineServers] = server;
    onlineServers++;

    return 0;
}

Server* UpdateServerList() {
    RootResponse response = MakeRootRequest(
        k_cfRequestServerList,
        (Server){0}, // no related server
        (User){0},  // no user
        (CMessage){0} // No cmessage
        );
    
    if (response.rcode != k_rcRootOperationSuccessful)
        return NULL;

    // MakeRootRequest already updates serverList
    // and onlineServers client side
    // When k_cfRequestServerList is passed.
    return serverList;
}
