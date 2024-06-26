/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       server.c
 * @brief      con and destruct server and manage it.
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
#include <stdbool.h>

bool DEBUG = false; // Debug mode

#include "Headers/server.h"
#include "Headers/browser.h"
#include "Headers/client.h"
#include "Headers/ccmds.h"
#include "Headers/tools.h"

PortDesc portList[] = {};

void ServerPrint(const char* color, const char* str, ...) {
    struct tm* timestr = gmt();

    va_list argp;
    va_start(argp, str);
    printf("%s", color);
    printf("[srv/][%02d:%02d:%02d] ", timestr->tm_hour, timestr->tm_min, timestr->tm_sec);

    // Prefix
    vfprintf(stdout, str, argp);
    va_end(argp);    
    printf(RESET "\n");
}

void ServerAnnouncement(Server* server, char* message) {    
    CMessage msgToSend = {0};
    msgToSend.cflag = k_cfPrintServerAnnouncement;
    strcpy(msgToSend.message, message);

    for (int clientIndex = 1; clientIndex<=server->connectedClients; clientIndex++){
        int sent = send(server->clientList[clientIndex].cfd, (void*)&msgToSend, sizeof(msgToSend), 0);
    }
}

void* ListenForRequestsOnServer(void* server)
{
    Server* serverToListenOn = (Server*)server;
    User requestMaker = serverToListenOn->clientList[serverToListenOn->connectedClients];
    printf("Listening for requests from %s, %i\n", requestMaker.handle, requestMaker.cfd);
    
    while (1)
    {
        ServerRequest request = {0};

        // Recv info from the most recent client
        // New thread created every client. Bad but will do
        int recvBytes = recv(requestMaker.cfd, (void*)&request, sizeof(request), 0);

        if (recvBytes <= 0)
            continue;
        
        request.requestMaker.cfd = serverToListenOn->clientList[serverToListenOn->connectedClients].cfd;
        request.requestMaker.connectedServer = serverToListenOn;

        printf(CYN "[%s] Received Server Request: %i\n" RESET, serverToListenOn->alias, request.command);

        DoServerRequest(request);
        
        // Don't listen for requests from that client anymore
        if (request.command == k_cfKickClientFromServer && strcmp(request.optionalClientMessage.message, request.requestMaker.handle) == 0)
            break;
    }
}

/**
 * 
 * @brief           Accept all client connections on the server provided
 * @param[in]       serverInfo: the server to accept client connections for
 * @return          void*
 * @retval          None
 */
void ServerAcceptThread(void* serverInfo)
{
    // Server info
    Server* server = (Server*)serverInfo; 

    int lstn = listen(server->sfd, server->maxClients);
    if (lstn < 0) {
        ServerPrint(RED, "Error Listening on Server %s", server->alias);
        close(server->sfd);
        return;
    }

    // 'server' becomes unsafe since we are in a thread.
    // make copy of 'server' by dereferencing it. dont even need it as a pointer anymore
    Server dereferencedServer = *server;
    dereferencedServer.connectedClients = 0;
    int cfd = 0; // client file descriptor. get it from accepting the client
    while (1)
    {
        cfd = accept(dereferencedServer.sfd, NULL, NULL);

        if (cfd < 0) {
            continue;
        }

        User receivedUserInfo = {0};
        int clientInfo = recv(cfd, (void*)&receivedUserInfo, sizeof(User), 0);  
        if (clientInfo < 0)
            continue;
        else if (clientInfo == 0) // client disconnected
            break;

        printf("Received client information %s\n", receivedUserInfo.handle);
        
        /*
            Update the clients connection info
        */
        receivedUserInfo.cfd             = cfd;
        receivedUserInfo.connectedServer = &dereferencedServer;

        /*
            Update the servers client list and since its dereferenced we 
            dont have to worry about putting inaccurate information
            in the actual server.
        */
        dereferencedServer.connectedClients++;
        dereferencedServer.clientList[dereferencedServer.connectedClients] = receivedUserInfo;
        backendServerList[dereferencedServer.serverId] = dereferencedServer;

        // Send the server info
        int sentServerInfo = send(cfd, (void*)&backendServerList[dereferencedServer.serverId], sizeof(backendServerList[dereferencedServer.serverId]), 0);
        if (sentServerInfo <= 0) // Error sending info
            break;
        
        cpthread tinfo = cpThreadCreate(ListenForRequestsOnServer, (void*)&backendServerList[dereferencedServer.serverId]);
    }
}

unsigned int GenerateServerUID(Server* server)
{
    srand(server->port ^ 0x92EFF);

    int uid = rand() % 1000;
    server->serverId = uid;
    printf("Generated UID: %d\n", uid);

    return uid;
}

/**
 * @brief           Construct a server using arguments found in the Thread_ServerArgs struct
 * @param[in]       serverArgsStruct: Thread_AcceptArgs struct with parameters
 * @return          void*
 */
void RSServerBareMetal(void* serverStruct)
{

    /* 
     * Create struct representing server
     * Used to resolve information about a chatroom
     */
    ServerCreationInfo* creationInfo = (ServerCreationInfo*)serverStruct;
    Server*             serverInfo   = creationInfo->serverInfo; // The info the user provided about the server they want to make
    SystemPrint(CYN, false, "%s: Request: Make New Server", creationInfo->clientAKAhost->handle);

    RootResponse response;
    memset(&response, 0, sizeof(RootResponse));
    response.rcode       = k_rcInternalServerError;
    response.returnValue = (void*)serverInfo;
    response.rflag       = k_rfSentDataWasUnused;

    if (serverInfo->maxClients > kMaxServerMembers){
        // Max clients is greater
        serverInfo->maxClients = kDefaultMaxClients;
    }

    /* Server address information */
    struct sockaddr_in addrInfo;
    memset(&addrInfo, 0, sizeof(struct sockaddr_in));
	addrInfo.sin_family      = serverInfo->domain;
	// addrInfo.sin_addr.s_addr = inet_addr(HOST_IP_ADDRESS);
    addrInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	addrInfo.sin_port        = htons(serverInfo->port);

    // Create server socket
    int sfd = socket(serverInfo->domain, serverInfo->type, 0);
    if (sfd == -1) {
        ServerPrint(RED, "[ERROR]: Failed Creating Socket. Error Code %i", errno);
        RSRespondToRootRequestMaker(creationInfo->clientAKAhost, response);
        goto server_close;
    }
    
    /*
        We set SO_REUSEADDR to true, in case a server
        that previously used this port is closed but 
        we cant create another socket with this port because of
        the TIME_WAIT state in the kernel.
    */
    int optVal = 1;
    int set = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (set < 0) {
        ServerPrint(RED, "setsockopt() for SO_REUSEADDR failed. Maybe port is in use.");
        RSRespondToRootRequestMaker(creationInfo->clientAKAhost, response);
        goto server_close;
    }

    // Bind socket to the address info provided
    int bnd = bind(sfd, (struct sockaddr * )&addrInfo, sizeof(addrInfo));
    if (bnd == -1) {
        ServerPrint(RED, "[ERROR]: Failed Binding Socket. Error Code %d", errno);
        RSRespondToRootRequestMaker(creationInfo->clientAKAhost, response);
        goto server_close;
    }

    // Listen for client connections
    int lsn = listen(sfd, serverInfo->maxClients);
    if (lsn == -1) {
        ServerPrint(RED, "[ERROR]: Error while listening. Error Code %d", errno);
        RSRespondToRootRequestMaker(creationInfo->clientAKAhost, response);
        goto server_close;
    }

    /* Server online, fill in rest of info */
    User hostCopy = *creationInfo->clientAKAhost;

    GenerateServerUID(serverInfo);
    serverInfo->connectedClients = 0;
    serverInfo->host             = hostCopy;
    serverInfo->domain           = creationInfo->serverInfo->domain;
    serverInfo->isRoot           = false;
    serverInfo->port             = serverInfo->port;
    serverInfo->type             = serverInfo->type;
    serverInfo->maxClients       = creationInfo->serverInfo->maxClients;
    serverInfo->sfd              = sfd;
    serverInfo->addr             = addrInfo;
    serverInfo->online           = true; // True. Server online and ready
    serverInfo->clientList       = (User**)malloc(sizeof(User*) * serverInfo->maxClients); // Allocate memory for the servers client list
    strcpy(serverInfo->alias, creationInfo->serverInfo->alias);
    
    // add server to server list

    if (strcmp(serverInfo->alias, "direct-message") != 0) {
        serverList[onlineServers++] = *serverInfo; 
        backendServerList[serverInfo->serverId] = *serverInfo;
    }

    // respond to host telling them their server was made
    // update their client info server sided
    
    hostCopy.connectedServer = serverInfo;
    SSUpdateClientWithNewInfo(hostCopy);

    response.rcode       = k_rcRootOperationSuccessful;
    response.returnValue = (void*)serverInfo;
    response.rflag       = k_rfRequestedDataUpdated;
    RSRespondToRootRequestMaker(&serverInfo->host, response);
    printf("Responded saying server creation successful\n");

    cpthread tinfo = cpThreadCreate(ServerAcceptThread, (void*)serverInfo);
    cpThreadJoin(tinfo);

    return;

server_close:
    close(sfd);
    // pthread_exit(NULL);
    return;
}

/** 
 * @brief           Wrapper for RSServerBareMetal()
 * @param[in]       domain:     Domain for the socket
 * @param[in]       type:       Type of the socket
 * @param[in]       protocol:   Protocol for the socket
 * @param[in]       port:       Port for the socket
 * @param[in]       ip:         Ip to host server on
 * @param[in]       maxClients: Max clients able to connect
 * @param[in]       alias:      Name of the server
 * @return          int
 * @retval          Server Status or Setup status
 */
int MakeServer(
    int domain, // AF_INET
    int type, // SOCK_STREAM
    int protocol, // USUALLY ZERO
    int port,
    unsigned int maxClients,
    char* alias
)
{
    // Make sure server name follows rules
    if (strlen(alias) < kMinServerAliasLength){
        SystemPrint(YEL, true, "[WARNING]: Server Name Too Short. Min %i", kMinServerAliasLength);
        return -1;
    }

    if (strlen(alias) > kMaxServerAliasLength){
        SystemPrint(YEL, true, "[WARNING]: Server Name Longer Than (%i) Chars.", kMaxServerAliasLength);
        return -1;
    }

    if (strstr(alias, " ") != NULL){
        SystemPrint(YEL, true, "[WARNING]: Server Name Cannot Have Spaces. Select a Different Name.");
        return -1;
    }

    // Iterate through servers, Check if the alias, and port is already in use
    // TODO: Optimize. Might be extremely slow when theres a lot of servers
    // O(n). Slow
    
    // for (int i = 0; i < onlineServers; i++){
        
    //     // Check if port is in use
    //     if (port == serverList[i].port)
    //     {
    //         SystemPrint(YEL, true, "[WARNING]: Port Already In Use. Choose a Different Port. (%i)", port);
    //         return -1;
    //     }
        
    //     // Check if alias is in use
    //     char currentServerName[kMaxServerAliasLength + 1];
    //     strcpy(currentServerName, serverList[i].alias);
    //     if (currentServerName != NULL){
    //         toLowerCase(alias); 
    //         toLowerCase(currentServerName);

    //         if (strcmp(currentServerName, alias) == 0) { // Server name is already used
    //             SystemPrint(YEL, true, "[WARNING]: Cannot Create Server: Name Already In Use. (%s)", alias);
    //             return -1;
    //         }
    //     }
    // }
    
    Server serv;
    memset(&serv, 0, sizeof(Server));

    // Set the server properties to be created
    serv.domain     = domain;
    serv.type       = type;
    serv.protocol   = protocol;
    serv.port       = port;
    serv.maxClients = maxClients;
    serv.isRoot     = false;
    serv.host       = *localClient;
    strcpy(serv.alias, alias);
    
    // Tell root server to host a server
    RootResponse response = MakeRootRequest(k_cfMakeNewServer, serv, *localClient, (CMessage){0});
    if (response.rcode == k_rcRootOperationSuccessful) 
    {
        localClient->connectedServer = &serv;
        SystemPrint(CYN, false, "Server '%s' Created on Port '%i'\n", serv.alias, serv.port);
    }
    else
        SystemPrint(RED, false, "Error Making Server '%s'\n", serv.alias);

    return response.rcode;
}

bool IsUserHost(User user, Server* server)
{
    return (strcmp(user.handle, server->host.handle) == 0);
}

void ShutdownServer(Server* server)
{
    Server serverCopy = *server;

    printf("Server shutdown requested for '%s'...\n", serverCopy.alias);
    
    int shutdownMethod = 0;
    
#ifdef _WIN32 
    shutdownMethod = SD_BOTH; 
#elif __unix__ 
    shutdownMethod = SHUT_RDWR;
#endif

    // if (shutdown(server->sfd, shutdownMethod) < 0)
    //     printf("Error Calling 'shutdown()' for Server. Error Code %i\n", errno);

    printf("Disconnecting all %d clients from server...\n", server->connectedClients);
    for (int cl_index=1; cl_index<=server->connectedClients; cl_index++)
    {
        User clientToDisconnect = server->clientList[cl_index];
        printf("- Closing: %s\n", clientToDisconnect.handle);
        // send a message to the client saying that the current server they were connected
        // to has been shutdown
        CMessage disconnectMessage = {0};
        disconnectMessage.cflag = k_cfConnectedServerShutDown;
        disconnectMessage.sender = clientToDisconnect;
        int sent = send(clientToDisconnect.cfd, (void*)&disconnectMessage, sizeof(disconnectMessage), 0);
        printf("sent bytes %d to %d, errno %d\n", sent, clientToDisconnect.cfd, errno);
        sleep(1);
        // close(clientToDisconnect.cfd);
        memset(clientToDisconnect.connectedServer, 0, sizeof(Server));
        printf(" - Closed\n");
    }

    printf("Done\n");
    printf("Removing server from server list... \n");
    int serverIndex = -1;
    printf("is name still good %s\n", serverCopy.alias);

    // Get index where the server is located on serverList
    for (int i=0; i < onlineServers; i++){
        printf("i: %d, name: %s, strcmp: %d, name2: %s\n", i, serverList[i].alias, strcmp(serverList[i].alias, serverCopy.alias), serverCopy.alias);
        if (strcmp(serverList[i].alias, serverCopy.alias) == 0){
            printf("located server in serverlist\n");
            serverIndex = i;
            break;
        }        
    }

    // Remove the server from the server list
    if (serverIndex != -1){
        for (int i = serverIndex; i < onlineServers; i++)
        {
            serverList[i] = serverList[i + 1];
            printf("shifted\n");
        }

        onlineServers--;
    }

    // Unuse port
    int portIndex = server->port % kMaxServersOnline;
    portList[portIndex].inUse = false;

    serverCopy.online = false;
    backendServerList[server->serverId].online = false;
    printf("Server online? %d\n", backendServerList[server->serverId].online);
    printf("server online2: %d\n", serverList[serverIndex].online);
    // printf("Done\n");
    printf("Closing server socket and freeing memory... ");

    close(server->sfd);
    printf("Done\n");
    printf("Server closed successfully... Done\n");
}

CMessage EncryptClientMessage(CMessage* message)
{
    // TODO: Use aes lib

    char kXorConstant = (unsigned char)rootServer.serverId;

    for (int i=0; i<strlen(message->message); i++)
    {
        message->message[i] = message->message[i] ^ kXorConstant;
        // printf("%c", message->message[i]);
    }

    return *message;
}

User GetClientFromClientList(char* username, Server* server) {
    for (int ci = 1; ci<=server->connectedClients; ci++)
        if (strcmp(server->clientList[ci].handle, username) == 0) 
            return server->clientList[ci];
    
    return (User){0};
}

bool IsClientInServer(char* username, Server* server){
    if (strcmp(GetClientFromClientList(username, server).handle, username) == 0) 
        return true;

    return false;
}

ResponseCode DoServerRequest(ServerRequest request)
{
    User sender = request.requestMaker;
    
    ResponseCode responseStatus = k_rcInternalServerError;
    switch (request.command)
    {
    case k_cfKickClientFromServer:
        CMessage detailedCommand = request.optionalClientMessage;
        if (strcmp(detailedCommand.message, detailedCommand.sender.handle) == 0) {
            // user just wants to disconnect not kick
            SSDisconnectClientFromServer(&sender);
            sender.connectedServer = &rootServer;
        } else {
            // user wants to kick someone
            printf("Going to kick %s\n", detailedCommand.message);

            if (!IsClientInServer(detailedCommand.message, sender.connectedServer))
                break;

            User client = GetClientFromClientList(detailedCommand.message, sender.connectedServer);
            if (strcmp(client.handle, detailedCommand.message) != 0) { break; }

            CMessage kick = {0};
            kick.cflag = k_cfKickClientFromServer;
            send(client.cfd, (void*)&kick, sizeof(kick), 0);
            
            SSDisconnectClientFromServer(&client);
            char announcement[kMaxClientHandleLength + 50];
            snprintf(announcement, sizeof(announcement), "%s was kicked from the server.", detailedCommand.message);
            ServerAnnouncement(sender.connectedServer, announcement);
        }

        responseStatus = k_rcRootOperationSuccessful;

        break;
    case k_cfEchoClientMessageInServer:
        
        printf("Echoing client message. %s\n", request.optionalClientMessage.message);
        Server* connectedServer = sender.connectedServer;

        // EncryptClientMessage(&request.optionalClientMessage);
        // printf("- Encrypted\n");

        // tell the clients that we want them to print it out when they recv
        request.optionalClientMessage.cflag = k_cfPrintPeerClientMessage;
        
        // relay encrypted message to all connected clients
        for (int ci = 1; ci <= connectedServer->connectedClients; ci++)
        {
            User recipient = connectedServer->clientList[ci];
            // if (strcmp(sender.handle, recipient.handle) == 0)
                // continue; // Dont send the message to the client who sent the message

            printf("- [%i] cfd: %i name: %s\n", ci, recipient.cfd, recipient.handle);

            int sentBytes = send(recipient.cfd,
                                 (void*)&request.optionalClientMessage, 
                                 sizeof(request.optionalClientMessage), 0);
            fprintf(stderr, "-- Sent bytes: %i\n", sentBytes);
            // printf("Sent messaage to one client\n");
        }

        responseStatus = k_rcRootOperationSuccessful;

        printf("Responded. Message sent successfully.");

        break;
    default:
        break;
    }

    return responseStatus;
}
