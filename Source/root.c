/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       root.c
 * @brief      functions which control the root server. NOTE: ALL OF THESE FUNCTIONS SHOULD BE SERVER-SIDED
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

#include "Headers/root.h"

/*
    Global statistics about the
    root server
*/
unsigned int onlineGlobalClients = 0; // All clients connected to the root server
User   rootConnectedClients[] = {};  // List of all clients on the root server
Server rootServer = { 0 };          // Root server info

void SSUpdateClientWithNewInfo(User updatedUserInfo)
{
    // Iterate through rootConnectedClients list to find the client to update
    for (size_t i = 0; i < onlineGlobalClients; i++)
    {
        if (strcmp(rootConnectedClients[i].handle, updatedUserInfo.handle) == 0) 
        {
            rootConnectedClients[i] = updatedUserInfo;
            break;
        }
    }
}

void RSUpdateServerWithNewInfo(Server* updatedServerInfo)
{
    // Iterate through server list to find the server to update
    for (size_t i = 0; i < onlineServers; i++)
    {
        // 'server' is at index 'i' in the list.
        if (strcmp(serverList[i].alias, updatedServerInfo->alias) == 0) 
        {
            // found it now replace it with updated server aka 'server'
            serverList[i] = *updatedServerInfo;
            backendServerList[updatedServerInfo->serverId] = *updatedServerInfo;
            break;
        }
    }
    // Now the server should be updated in the serverList
}

void UpdateClientInConnectedServer(User* userToUpdate)
{
    if (!userToUpdate->connectedServer->online)
        return;

    Server* server = userToUpdate->connectedServer;

    // Get the index of the client from the connectedServers Client list
    int clientIndexInCList = ClientIndex(
                                server->clientList,
                                server->connectedClients,
                                userToUpdate);
    
    // Temp array to copy all clients to including the updated client info
    User* tmpArray[] = {0};
    
    // Copy over the connected servers client list to tmpArray
    // and input the most updated info into tmpArray
    for (int i=0; i<server->connectedClients; i++)
    {
        // Found the user to update in the connected servers client list
        if (strcmp(server->clientList[i].handle, userToUpdate->handle) == 0)
            // Put updated User* struct in tmpArray
            tmpArray[i] = userToUpdate;
        else
            // Normal client we dont need to update
            tmpArray[i] = &server->clientList[i];
    }

    // Now copy tmpArray into connected servers client list so client list
    // is fully up to date
    for (int i=0; i<server->connectedClients; i++)
        server->clientList[i] = *tmpArray[i];

    // Finally take changes into affect and update this server on the root server
    RSUpdateServerWithNewInfo(server);
}

/**
 * @brief           Make a request from the client to the root server. Kind of like http
 * @param[in]       commandFlag:   tell the server what to do with the data
 * @param[in]       currentServer: server to make the request from
 * @param[in]       relatedClient: client who made the request
 * @return          void*
 * @retval          RootResponse from server as void*
 */
RootResponse MakeRootRequest(
    CommandFlag    commandFlag,
    Server    currentServer,
    User      relatedClient,
    CMessage clientMessageInfo
)
{
    RootRequest request;
    request.cmdFlag           = commandFlag;
    request.server            = currentServer;
    request.user              = relatedClient;
    request.clientSentMessage = clientMessageInfo;

    // Default response values
    RootResponse response = {0};
    response.rcode        = k_rcInternalServerError;
    response.rflag        = k_rfNoResponse;
    response.returnValue  = NULL;

    // Send request to root server
    int sentBytes = send(rootServer.sfd, (const void*)&request, sizeof(RootRequest), 0);
    if (sentBytes <= 0) { // Client disconnected or something went wrong sending
        printf(RED "Error making request to root server...\n" RESET);
        return response;
    }

    // Client wont be able to receive messages when their socket file descriptor is closed
    if (request.cmdFlag == k_cfDisconnectClientFromRoot)
        return response;

    // Receive a response from the root server
    int receivedBytes = recv(rootServer.sfd, (void*)&response, sizeof(RootResponse), 0);
    if (receivedBytes <= 0) { // Client disconnected or something went wrong receiving
        printf(RED "Failed to receive data from root server...\n" RESET);
        return response;
    }
 
    // In the case of special commands
    // where we may need to send or recv more than once
    switch (request.cmdFlag)
    {
    case k_cfClientRequestPrivateMessage:
        if (response.rcode == k_rcRootOperationSuccessful) {
            // client accepted the pm
            char pmName[400];
            snprintf(pmName, sizeof(pmName), "%s-%s", localClient->handle, request.clientSentMessage.message);
            int pmServer = MakeServer(AF_INET, SOCK_STREAM, 0, 5023, 2, pmName);
        }
        break;
    case k_cfMakeNewServer:
        break;
    case k_cfRequestServerList: // Need to receive all servers individually
    {
        // Receive the number of online servers
        uint32_t onlineServersTemp = 0;
        int initialReceivedBytes = recv(rootServer.sfd, &onlineServersTemp, sizeof(onlineServersTemp), 0);
        
        if (initialReceivedBytes <= 0)
            break;
        
        onlineServers = ntohl(onlineServersTemp);

        // Receive all servers
        for (int i = 0; i < onlineServers; i++){
            Server receivedServer = {0};
            int receive = recv(rootServer.sfd, (void*)&receivedServer, sizeof(Server), 0);
            
            if (receive <= 0)
                break;
            
            // Update server list
            serverList[i] = receivedServer;
        } 

        break;
    }
    default:
        break;
    }

    return response;
}

ResponseCode DoRootRequest(void* req)
{
    RootRequest  request = *(RootRequest*)req;
    RootResponse response;
    memset(&response, 0, sizeof(response)); 

    // Default response
    response.rcode       = k_rcRootOperationSuccessful;
    response.returnValue = NULL;
    response.rflag       = k_rfSentDataWasUnused;

    printf("Doing root request\n");

    // Handle possible request commands
    switch (request.cmdFlag)
    {
    case k_cfClientRequestPrivateMessage:
        CMessage options = request.clientSentMessage;
        bool userExists = false;
        User peer = {0};

        printf("%s wants to pm %s\n", request.user.handle, options.message);
        for (int ci = 1; ci <= onlineGlobalClients; ci++) { // terrible o(n) dont have tiem for hashmap
            printf("- %s\n", rootConnectedClients[ci].handle);
            
            if (strcmp(rootConnectedClients[ci].handle, options.message) == 0) {
                userExists = true;
                peer = rootConnectedClients[ci];
                break;
            }
        }

        if (!userExists) {
            response.rcode = k_rcInternalServerError;
            response.rflag = k_rfNoResponse;
            RSRespondToRootRequestMaker(&request.user, response);
            break;
        }

        printf("found the client %s\n", peer.handle);
        CMessage pmCMD = {0};
        pmCMD.cflag = k_cfClientRequestPrivateMessage;
        pmCMD.sender = request.user;

        CMessage peerResponse = {0};

        send(peer.rfd, (void*)&pmCMD, sizeof(pmCMD), 0);
        int recvd = recv(peer.rfd, (void*)&peerResponse, sizeof(peerResponse), 0);
        printf("recvd %d, cf %d\n", recvd, peerResponse.cflag);

        if (peerResponse.cflag == k_cfClientAcceptedPrivateMessage) {
            printf("they accepted the pm!\n");
            
        } else if (peerResponse.cflag == k_cfClientDeclinedPrivateMessage) {
            printf("declined :(\n");
            response.rcode = k_rcInternalServerError;
        }

        RSRespondToRootRequestMaker(&request.user, response);

        break;
    case k_cfRequestServerList: // Client wants to know the updated server list 
        RSRespondToRootRequestMaker(&request.user, response);

        // First send the amount of online servers as an int
        uint32_t nlOnlineServers = htonl(onlineServers); // htonl version of onlineServers int
        
        int sentBytes = send(request.user.rfd, &nlOnlineServers, sizeof(nlOnlineServers), 0);
        if (sentBytes <= 0)
            SystemPrint(RED, true, "Error sending online servers int. Errno %i", errno);

        // After prepare and send all the servers from the list individually
        for (int servIndex=0; servIndex<onlineServers; servIndex++)
        {
            Server server = serverList[servIndex];
            Server updated = backendServerList[server.serverId];

            printf("serv: %s, online: %d, backend name: %s, backend online: %d\n", server.alias, server.online, updated.alias, updated.online);

            // the server isnt online so don't send it
            if (!updated.online) {
                printf("server not online dotn sedn it\n");
                continue;
            }

            int sent = send(request.user.rfd, (void*)&updated, sizeof(Server), 0);
            if (sent <= 0)
                SystemPrint(RED, true, "Error couldn't send server: %s", updated.alias);
        }

        break;
    case k_cfAppendServer: // Add server to server list
        printf("Append server\n");
        SSAddServerToList(request.server);
        response.rflag = k_rfNoValueReturnedFromRequest;
        RSRespondToRootRequestMaker(&request.user, response);
        break;
    case k_cfRemoveServer: // Remove server from server list
        int index = -1;
        for (int i=0; i < onlineServers; i++){
            if (strcmp(serverList[i].alias, request.server.alias) == 0) // Server exists in the list
                index = i;
        }

        if (index == -1) // Server doesnt exist...
            break;

        // shift array to remove that server
        for (int b=index; b < onlineServers - 1; b++) 
            serverList[b] = serverList[b + 1]; 
        
        onlineServers--;
        // Server list now removed
        RSRespondToRootRequestMaker(&request.user, response);
        break;
    case k_cfMakeNewServer: // Make new server and run it
        printf("Make new server\n");
        // Index will be the port hashed by max servers allowed online
        int indexInList = request.server.port % kMaxServersOnline;

        // Make sure port isnt in use
        // Try and find the port in the hash map
        // Check if ports in use
        if (portList[indexInList].port == request.server.port && portList[indexInList].inUse)
        {
            // in use
            printf("In use\n");
            response.rcode = k_rcErrorPortInUse;
            response.rflag = k_rfSentDataWasUnused;
            RSRespondToRootRequestMaker(&request.user, response);
            break;
        }

        // Add it to the list of ports
        PortDesc portInfo = {0};
        portInfo.inUse = true;
        portInfo.port = request.server.port;
        portList[indexInList] = portInfo;

        ServerCreationInfo creationInfo = {0};
        creationInfo.serverInfo = &request.server;
        creationInfo.clientAKAhost = &request.user;

        cpthread tinfo = cpThreadCreate(RSServerBareMetal, (void*)&creationInfo);
        printf("Created Server\n");
        break;
    // pthread_exit(NULL);
    case k_cfDisconnectClientFromRoot:
        RSDisconnectClientFromRootServer(request.user);
        return k_rcRootOperationSuccessful;
    case k_cfKickClientFromServer:
        SSDisconnectClientFromServer(&request.user);
        break;
    case k_cfRSUpdateServerWithNewInfo:
        RSUpdateServerWithNewInfo(&request.server);
        break;
    case k_cfSSUpdateClientWithNewInfo:
        request.user.connectedServer = &request.server;
        SSUpdateClientWithNewInfo(request.user);
        response.rflag = k_rfRequestedDataUpdated;
        RSRespondToRootRequestMaker(&request.user, response);
        break;
    default:
        return k_rcInternalServerError; // LIkely the command doesnt exist
    }

    return response.rcode;
}
 
void* AcceptClientsToRoot() {
    Server rootServerBackup = rootServer;
    
    while (1) {
        // Make sure onlineGlobalClients does not exceed max allowed clients
        if (onlineGlobalClients + 1 >= kMaxGlobalClients)
            continue;

        int cfd = accept(rootServerBackup.sfd, (struct sockaddr*)NULL, NULL);

        if (cfd < 0) // Bad client file descriptor.
            continue;

        printf("Accepted a join.\n");

        // Receive client info on join
        RootRequest request = { 0 };
        int clientInfo = recv(cfd, (void*)&request, sizeof(request), 0);
        
        if (clientInfo < 0) 
        {
            SystemPrint(RED, false, "Error Receiving Client '%i' Info.", cfd);
            close(cfd);
            continue;
        }

        // user wants to join
        if (request.cmdFlag == k_cfConnectClientToServer)
        {
            SystemPrint(CYN, false, "%s Joined!", request.user.handle);
                        
            request.user.rfd             = cfd;
            request.user.connectedServer = &rootServer;
            onlineGlobalClients++;
            rootConnectedClients[onlineGlobalClients] = request.user;

            // Send info back
            RootResponse response = {0};
            response.rcode        = k_rcRootOperationSuccessful;
            response.returnValue  = (void*)&request.user;
            response.rflag        = k_rfRequestedDataUpdated;
           
            int sendResponse = send(cfd, (void*)&response, sizeof(response), 0);
            if (sendResponse <= 0)
            {
                printf(RED "\tError Sending Updated Struct Back\n" RESET);
                continue;
            }

            // Make a thread for each client who connects
            cpthread tinfo = cpThreadCreate(PerformRootRequestFromClient, (void*)&request.user);
            printf("Make a thread for client\n");

            // Reset rootServer to rootServerBackup incase of a garbage values ??*
            // rootServer = rootServerBackup;
        }
    }
    printf("Stopped accepting clients root\n");
    // pthread_exit(NULL);
}

void* PerformRootRequestFromClient(void* client) {
    
    User connectedClient = rootConnectedClients[onlineGlobalClients];
    printf("- Root thread created for %s: %i\n", connectedClient.handle, connectedClient.rfd);
    
    /*
        Forever receive requests
        from a client unless a condition is met
        to break from the loop. Usually an error.
    */
    while (1) 
    {
        RootRequest receivedRequest = { 0 };
        size_t      receivedBytes = recv(connectedClient.rfd, (void*)&receivedRequest, sizeof(RootRequest), 0);
        printf("Received information from %s: %i\n", connectedClient.handle, connectedClient.rfd);

        if (receivedBytes < 0) 
        {
            SystemPrint(RED, false, "Failed to Receive Message");
            continue;
        }
        else if (receivedBytes == 0)
        {
            RSDisconnectClientFromRootServer(receivedRequest.user);
            break;
        }

        // No command to perform
        if (receivedRequest.cmdFlag == k_cfNone)
            continue;

        if (receivedRequest.cmdFlag != k_cfDisconnectClientFromRoot)
            receivedRequest.user = connectedClient;
        else
            receivedRequest.user.connectedServer = &receivedRequest.server;

        ResponseCode result = DoRootRequest((void*)&receivedRequest);
        if (receivedRequest.cmdFlag == k_cfDisconnectClientFromRoot)
            break;

        if (result != k_rcRootOperationSuccessful) 
        {
            printf(RED "Error Doing Request '%i' From %s\n" RESET, receivedRequest.cmdFlag, connectedClient.handle);
            continue;
        }
    }
    printf("Thread for %s ended\n", connectedClient.handle);
    // pthread_exit(NULL);
} 

void RSRespondToRootRequestMaker(User* to, RootResponse response) {
    fprintf(stderr, CYN "[AMS] Response to Client '%s' ", to->handle);
    
    int msg_signal = 0;

#ifdef __unix__
    msg_signal = MSG_NOSIGNAL;
#endif

    int snd = send(to->rfd, (void*)&response, sizeof(response), msg_signal);
    // int snd = sendto(to->rfd, (void*)&response, sizeof(response), msg_signal, (struct sockaddr*)&to->addressInfo, sizeof(to->addressInfo));

    if (snd <= 0)
        printf(RED "Failed\n" RESET);
    else
        printf(GRN "Good\n" RESET);
}

void RSDisconnectClientFromRootServer(User usr) {
    int index = 0;

    // Get index where client is on the root connected clients arra    
    for (int i = 0; i < onlineGlobalClients; i++){
        // Is the user we want equal to the user in the list at the index 'i'
        if (strcmp(rootConnectedClients[i].handle, usr.handle) == 0)
            index = i;
    }

    // remove client from rootConnectedClients by shifting array
    for (int i = index; i < onlineGlobalClients - 1; i++) 
        rootConnectedClients[i] = rootConnectedClients[i + 1]; 

    // Check if client is connected to server
    // If client is, update the server statistics
    if (!usr.connectedServer->isRoot) {
        if (IsUserHost(usr, usr.connectedServer))
        {
            ShutdownServer(usr.connectedServer);
        }
    }

    close(usr.cfd);
    close(usr.rfd);

    // Remove 1 client from connected client count
    onlineGlobalClients--;
    printf("Disconnected %s\n", usr.handle);
}

/**
 * @brief           Create a root server all clients connect to when starting the app
 * @return          int
 * @retval         successful result
 */
int CreateRootServer() {
    // DEBUG_PRINT("Creating root server. Make sure this is only made once");
    printf(BLU "\t|||| INITIAL ROOT SERVER CREATION ||||\n" RESET);
    printf("Creating root server. Filling in struct... ");

    rootServer.port       = ROOT_PORT;
    rootServer.domain     = AF_INET;
    rootServer.type       = SOCK_STREAM;
    rootServer.protocol   = 0;
    rootServer.maxClients = -1; // Infinite connections
    rootServer.isRoot     = true;
    strcpy(rootServer.alias, "__root__");

    printf("Done\n");

    printf("Creating socket for the server... ");
	int sfd = socket(rootServer.domain, rootServer.type, rootServer.protocol);
    if (sfd < 0)
        return -1;
    printf("Done\n");

    const bool kReuseAddr = true;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &kReuseAddr, sizeof(int));

    printf("Creating server address info... ");
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family      = rootServer.domain;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serv_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDRESS)
	serv_addr.sin_port        = htons(rootServer.port);

    printf("Done\n");
    printf("Binding socket to address info... ");

	int bnd = bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (bnd < 0) {
        close(sfd);
        return -1;
    }

    printf("Done\n");
    printf("Setting up listener for client connections... ");

	int lsn = listen(sfd, 10);
    if (lsn < 0) {
        close(sfd);
        return -1;
    }

    printf("Done\n");

    rootServer.addr = serv_addr; 
    rootServer.sfd  = sfd;

    printf(BLU "\t     |||| ROOT SERVER CREATED ||||\n" RESET);

    return 0;
}

void SSDisconnectClientFromServer(User* user) {
    Server* server = user->connectedServer;
    printf("Disconnecting %s from server\n", user->handle);

    // get index of client in server client list
    int index = -1;
    for (int i=0; i < server->connectedClients; i++){
        if (strcmp(server->clientList[i].handle, user->handle) == 0) // Server exists in the list
            index = i;
    }            

    printf("Getting index of client\n");

    // shift array to remove client from clientlist
    for (int b=index; b < server->connectedClients; b++) 
        server->clientList[b] = server->clientList[b + 1]; 

    printf("Shift array\n");

    server->connectedClients--;

    // close(user->cfd);
    printf("Closed the client file descriptor\n");
    
    // Update server with new info since 
    // the client list and connectedClients has been updated
    RSUpdateServerWithNewInfo(server);
    printf("Updated server with new info\n");

    if (IsUserHost(*user, server))
    {
        ShutdownServer(server);
    }
    user->connectedServer = &rootServer;
    // printf("Now the user is connected to %s\n", user->connectedServer->alias);
    SSUpdateClientWithNewInfo(*user);

    char announcement[kMaxClientHandleLength + 50];
    snprintf(announcement, sizeof(announcement), "%s left the server.", user->handle);
    ServerAnnouncement(server, announcement);
}