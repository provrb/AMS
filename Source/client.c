
#include "Headers/client.h"
#include "Headers/ccolors.h"

User* localClient = { 0 }; // Current client who ran the app

void MallocLocalClient() {
    localClient = (User*)malloc(sizeof(User));
}

CMessage DecryptPeerMessage(CMessage* message)
{
    char xorConstant = (unsigned char)rootServer.serverId;
    for (int charIndex = 0; charIndex<strlen(message->message); charIndex++)
    {
        message->message[charIndex] = message->message[charIndex] ^ xorConstant;
    }

    return *message;
}

ResponseCode MakeServerRequest(
    CommandFlag command,
    User        requestMaker,
    CMessage    optionalClientMessage
)
{
    optionalClientMessage = EncryptClientMessage(&optionalClientMessage);

    ServerRequest request         = {0};
    request.command               = command;
    request.optionalClientMessage = optionalClientMessage;
    request.requestMaker          = requestMaker;

    ResponseCode response = k_rcInternalServerError;

    // Send the request to the conncted server
    int sentBytes = send(requestMaker.cfd, (void*)&request, sizeof(request), 0);
    if (sentBytes < 0) // Error sending message
    {
        ServerPrint(RED, "Error Making Server Request");
        return response;
    }

    // Don't received on connected server socket if we left the server
    if (request.command != k_cfKickClientFromServer)
    {
        if (request.command != k_cfEchoClientMessageInServer)
        {
            /*
                Receive a response from the server as ResponseCode enum
            */
            int recvBytes = recv(requestMaker.cfd, (void*)&response, sizeof(ResponseCode), 0);
            if (recvBytes <= 0)
            {
                ServerPrint(RED, "Error Making Request");
                return response;
            }
        }
    }
    else 
    {
        if (strcmp(request.optionalClientMessage.message, localClient->handle) == 0) {
            // Client left the server so remove connectedServer
            localClient->connectedServer = (Server*){0};
            localClient->cfd = 0;
        }
    }

    return response;
}

size_t ClientIndex(User* arr[], size_t size, User* value)
{
    for (int i=0; i<size; i++)
    {
        if (strcmp(arr[i]->handle, value->handle) == 0)
            return i;
    }

    return -1;
}

void DisconnectClient() {
    
    
    // Client likely hasnt been setup yet so just exit.
    if (localClient->rfd == -99 && localClient->cfd == -99)
    {
        exit(EXIT_SUCCESS);
        return;
    }

    SystemPrint(WHT, false, "Disconnecting %s. %s", localClient->handle, localClient->connectedServer->alias);
    
    /*
        Make a root request to disconnect the client.
        Also send the server the clients connected to
        dereferenced so the server can check if it needs to be
        shutdown in the case the localClient is the host.
    */
    Server tempServer = *localClient->connectedServer;
    RootResponse response = MakeRootRequest(k_cfDisconnectClientFromRoot, tempServer, *localClient, (CMessage){0});
    if (response.rcode == k_rcRootOperationSuccessful){
        SystemPrint(GRN, false, "Disconnected. Goodbye");
    }

    close(localClient->cfd);
    close(localClient->rfd);

    exit(EXIT_SUCCESS);
}

void LeaveConnectedServer()
{
    ResponseCode req = MakeServerRequest(k_cfKickClientFromServer,
                                        *localClient,
                                        (CMessage){0}
                                        );

    UpdateServerList();
}

void ReceiveRootRequestsAsClient()
{
    while (1) {
        CMessage receivedCMessage = {0};
        int recvd = recv(localClient->rfd, (void*)&receivedCMessage, sizeof(receivedCMessage), 0);
        
        if (recvd < 0) // Error
            continue;

        switch (receivedCMessage.cflag)
        {
        case k_cfClientRequestPrivateMessage:
            SystemPrint(GRN, true, "%s Wants to PM! Y = accept. N = decline", receivedCMessage.sender.handle);
            
            CMessage clientResponse = {0};
            clientResponse.cflag = k_cfClientDeclinedPrivateMessage;
            clientResponse.sender = *localClient;

            // respond back with yes or no
            while (1) {
                char* inp = malloc(10);
                fgets(inp, 10, stdin);

                // Remove the trailing newline character if it exists
                if (inp[strlen(inp) - 1] == '\n') {
                    inp[strlen(inp) - 1] = '\0';
                }

                // Command is empty, don't need to process anything
                if (strlen(inp) == 0) {
                    continue;
                }

                if (strcmp(inp, "Y") == 0) {
                    clientResponse.cflag = k_cfClientAcceptedPrivateMessage;
                    SystemPrint(GRN, true, "You accepted the PM request.");
                    break;
                } else if (strcmp(inp, "N") == 0) {
                    clientResponse.cflag = k_cfClientDeclinedPrivateMessage;
                    SystemPrint(RED, true, "You declined the PM request.");
                    break;
                }
            }

            send(localClient->rfd, (void*)&clientResponse, sizeof(clientResponse), 0);

            break;
        default:
            break;
        }
    }
}

void ReceivePeerMessagesOnServer(void* serverInfo)
{
    Server* server = (Server*)serverInfo;
    
    /*
        Check if the local client is connected to the server
        we want to receive messages on.
    */
    if (strcmp(server->alias, localClient->connectedServer->alias) != 0) // Not on the server
        return;
    while (1)
    {
        /*
            Receive encrypted messages from other clients
            Decrypt them once received
        */
        CMessage receivedCMessage = { 0 };
        int receivedBytes = recv(localClient->cfd, (void*)&receivedCMessage, sizeof(receivedCMessage), 0);
        if (receivedBytes < 0) // Error
            continue;
        else if (receivedBytes == 0 && server->online) // Disconnected from seerver/Server went offline
            break;
        else if (receivedBytes == 0 && server->online == false) // Server was shutdown
        {
            ServerPrint(YEL, "Server Was Shutdown. Quitting.");
            LeaveConnectedServer();
            break;
        }
        
        // decrypt  the msg
        if (receivedCMessage.cflag == k_cfPrintPeerClientMessage) {
            receivedCMessage = DecryptPeerMessage(&receivedCMessage);
        }

        /*
            Find out what the peer wants us to do with
            the message. Map it to a command.
        */
        switch (receivedCMessage.cflag)
        {
        case k_cfClientRequestPrivateMessage:
            printf("client wants to pm\n");
            break;
        case k_cfPrintServerAnnouncement:
            ServerPrint(CYN, receivedCMessage.message);
            break;
        case k_cfPrintPeerClientMessage:
            receivedCMessage.sender.handle[strlen(receivedCMessage.sender.handle)] = '\0';
            // if (strcmp(receivedCMessage.sender.handle, localClient->handle) != 0)
            PrintClientMessage(receivedCMessage.sender, receivedCMessage.message);

            break;
        case k_cfKickClientFromServer:
            /*
                Peer told server they want this specific user to be
                kicked from the server.
                
                Request the server to remove local client.
            */
            localClient->connectedServer = &rootServer;
            ServerPrint(RED, "You have been kicked from '%s'", server->alias);
            ServerPrint(RED, "Press any key to continue...");
            return;
        case k_cfBanClientFromServer:
            // TODO: Add an array of banned clients to Server struct and add this user to it.
            
            LeaveConnectedServer();
            ServerPrint(RED, "You Have Been Banned From '%s'\n", server->alias);
            return;
        case k_cfConnectedServerShutDown:
            localClient->connectedServer = &rootServer;
            printf("\n");
            ServerPrint(RED, "The connected server has been shutdown.");
            ServerPrint(RED, "Enter any key to continue... ");
            return;
        default:
            break;
        }
    }
    // pthread_exit(NULL);
}

User CSClientFromName(char* username) {
    User* clientList = localClient->connectedServer->clientList;
    for (int ci = 1; ci<=localClient->connectedServer->connectedClients; ci++)
        if (strcmp(clientList[ci].handle, username) == 0)
            return clientList[ci];

    return (User){0};
}

/**
 * @brief           Handle clientinput by the user and treat it as a app command
 * @return          void*
 * @retval          Nothing.
 */
void* HandleClientInput(){
    localClient->connectedServer = &rootServer;

    while (1) {
        char cmd[300];
        bool validCmd = false;

        printf("Enter Command> ");

        fgets(cmd, kMaxCommandLength, stdin);

        // Remove the trailing newline character if it exists
        if (cmd[strlen(cmd) - 1] == '\n') {
            cmd[strlen(cmd) - 1] = '\0';
        }

        // Command is empty, don't need to process anything
        if (strlen(cmd) == 0) {
            continue;
        }

        /*
        *
        * All functions that take command line arguemnts
        * In a case a function takes a command line argument,
        * it must be processed specially like below
        * 
        */

        // Server info command. It takes command-line arguments
        if (strstr(cmd, "--si") != NULL) {
            char serverName[kMaxServerAliasLength + 1];

            if (sscanf(cmd, "--si %32s", serverName) != 1) {
                SystemPrint(RED, true, "Invalid Usage for --si. View --help for more info.");
                continue;
            }

            DisplayServerInfo(serverName);
            validCmd = true;
        }

        // Join server command. It takes command-line arguments
        else if (strstr(cmd, "--joins") != NULL) {
            char serverName[kMaxServerAliasLength + 1];

            if (sscanf(cmd, "--joins %32s", serverName) != 1) {
                SystemPrint(RED, false, "Invalid Usage for --joins. View --help for more info.");
                continue;
            }

            JoinServerByName(serverName);
            validCmd = true;
        }

        else if (strstr(cmd, "--makes") != NULL) {
            char         serverName[kMaxServerAliasLength + 1];
            unsigned int maxClients;
            unsigned int port;

            if (sscanf(cmd, "--makes %32s %u %u", serverName, &port, &maxClients) != 3) {
                SystemPrint(RED, false, "Invalid Usage for --makes. View --help for more info.");
                continue;
            }

            SystemPrint(CYN, true, "Making Server '%s' on Port '%i'", serverName, port);

            int serverCreated = MakeServer(AF_INET, SOCK_STREAM, 0, port, maxClients, serverName); // Error handling in the MakeServer() function
            
            validCmd = true;
        }

        else if (strstr(cmd, "--pm") != NULL) {
            char peerName[kMaxClientHandleLength + 1];
            if (sscanf(cmd, "--pm %s", peerName) != 1) {
                SystemPrint(RED, false, "Invalid Usage for --pm. View --help for more info.");
                continue;
            }

            if (strcmp(peerName, localClient->handle) == 0) {
                SystemPrint(RED, false, "Cannot PM Yourself. Try a Different User.");
                continue;
            }

            CMessage command = {0};
            command.cflag = k_cfClientRequestPrivateMessage;
            strcpy(command.message, peerName);
            command.sender = *localClient;
            RootResponse requestPM = MakeRootRequest(k_cfClientRequestPrivateMessage, rootServer, *localClient, command);
        }

        // Normal command function without command-line args
        for (int i = 0; i < kNumOfCommands; i++)
        {
            if (strcmp(cmd, validCommands[i].kCommandName) == 0) {
                validCommands[i].function();
                validCmd = true;
                break;
            }
        }

        if (!validCmd)
        {
            SystemPrint(RED, true, "Unknown Command. Enter --help to view commands.");
            continue;
        }
    }

    // pthread_exit(NULL);
}


/*
    Return -1 if no command was performed
    Return 99 if were leaving to break out of a chatroom loop
    Return 123 if command was unsuccessful
    Return 0 if command was successful
*/
int PerformClientSideServerCommands(char* message) {
    if (strcmp(message, "--leave") == 0) {
        ResponseCode leaveRequest = MakeServerRequest(k_cfKickClientFromServer, *localClient, (CMessage){0});
        return 99; // return 99 to say the command was successful and to break if were in a thread
    }
    else if (strstr(message, "--kick") != NULL) {
        // local client isnt host so return out
        if (!IsUserHost(*localClient, localClient->connectedServer)) { return -1; }

        char* peerName = malloc(kMaxClientHandleLength + 1);
        if (sscanf(message, "--kick %22s", peerName) != 1) {
            ServerPrint(YEL, "Invalid input. User may not exist.");
            free(peerName);
            return 123; // return telling command was unsuccessful
        }

        // client wants to kick themself. not allowed
        if (strcmp(peerName, localClient->handle) == 0)
            return 123;

        // at this point we know,
        // user is the host, the user is correct and the user is in the server
        CMessage command = {0};
        command.cflag = k_cfKickClientFromServer;
        command.sender = *localClient;
        strcpy(command.message, peerName);

        // kick the user
        ResponseCode kickRequest = MakeServerRequest(k_cfKickClientFromServer, *localClient, command);
        free(peerName);
        return 0; // performed
    } else {
        return -1;
    }
    
    return -1; // no command found
}

/**
 * @brief           Choose your username throughout the stay on the app
 * @retval          Nothing
 */
void ChooseClientHandle() {
    bool warned = false;
    bool goodUsername = false; // Does the client username inputted follow the rules

    SystemPrint(CYN, true, "Enter a username before joining:");
    
    // Make sure username is valid and follows rules
    do {
        char username[kMaxClientHandleLength + 1];                   
        printf(CYN "Select Your Username> " RESET);        
        fgets(username, kMaxClientHandleLength, stdin);    
        
        // Remove the trailing newline character if it exists
        if (username[strlen(username) - 1] == '\n') {
            username[strlen(username) - 1] = '\0';
        }
        
        for (int i=0; i < kNumOfCommands; i++){
            if (strstr(username, validCommands[i].kCommandName) != NULL) 
            {
                SystemPrint(YEL, true, "[WARNING]: Username cannot be a command.");
                warned = true;
                break;
            }
        }

        if (warned) {
            warned=false;
            continue;
        }

        if (strlen(username) <= 2 && !warned){
            SystemPrint(YEL, true, "[WARNING]: Username too short (Min 3 Chars).");
            warned=!warned;
            continue;
        }

        goodUsername=true;
        strcpy(localClient->handle, username);
    } while(!goodUsername);

    SystemPrint(CYN, false, "You will now be known as: '%s' for this session.", localClient->handle);
}

void AssignDefaultHandle(char defaultName[]) {
    time_t now = time(NULL);
    struct tm* timestr = gmtime(&now);

    char nameIdentifier[kMaxClientHandleLength + 1];
    sprintf(nameIdentifier, "%d", timestr->tm_sec);

    strcpy(defaultName, "usr");
    strcat(defaultName, nameIdentifier);
}

int DefaultClientConnectionInfo() {
    localClient->joined = gmt();
    localClient->addressInfo = rootServer.addr;
    localClient->connectedServer = (Server*){0};
    localClient->cfd = 0;
    localClient->rfd = rootServer.sfd;
    return 0;
}

/**
 * @brief           Connect to the server which holds information about all other servers
 * @return          int
 * @retval          success
 */
int ConnectToRootServer() {
    // Fill out information about the root server used to establish client connection.

    printf("Filling root server struct... ");
    
    rootServer.domain     = AF_INET;
    rootServer.type       = SOCK_STREAM;
    rootServer.protocol   = 0;
    rootServer.port       = ROOT_PORT;
    rootServer.maxClients = -1;
    rootServer.isRoot     = true;
    strcpy(rootServer.alias, "__root__");

    printf("Done\n");
    printf("Creating client socket using port %i... ", rootServer.port);

    // Connect client to server. Establish a connection

    int cfd = socket(rootServer.domain, rootServer.type, rootServer.protocol); // These are the settings the root server uses
    if (cfd < 0)
        goto close_root_connection;
    
    rootServer.sfd = cfd;

    printf("Done\n");
    printf("Filling address info for client to connect... ");

    // Address info for main server

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = rootServer.domain;
    addr.sin_port   = htons(rootServer.port); // Constant. Server will always be on this port
    // addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDRESS);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // if (inet_pton(rootServer.domain, "192.168.0.12", &addr.sin_addr) < 0)
        // goto close_root_connection;
    
    printf("Done\n");
    fprintf(stderr, "Connecting client socket to main server... ");

    // Attempt to retry connection again if it fails.
    int attempts = 0;
    do {
        int con = connect(cfd, (struct sockaddr*)&addr, sizeof(addr));
        if (con < 0) {
            attempts++;
            sleep(2);
            continue;
        }
        break;
    }
    while (attempts < 5);
    if (attempts >= 5) goto close_root_connection;

    printf("Done\n");
    printf("Client connected to main server.\n");
    printf("Filling out local client info struct... ");
   
    // Fill out local client info struct
    // Handle already filled out at start of program
    DefaultClientConnectionInfo();

    printf("Done\n");
    
    // sleep(1);
    // Send client info
    RootRequest req       = {0};
    req.clientSentMessage = (CMessage){0}; // No Client Message.
    req.cmdFlag           = k_cfConnectClientToServer;
    req.server            = rootServer;
    req.user              = *localClient;
    int sendInfo = send(rootServer.sfd, (const void*)&req, sizeof(req), 0);

    printf("Sent current user info. Now trying to recv.\n");

    // Get response
    RootResponse resp = {0};
    int recvInfo = recv(rootServer.sfd, (void*)&resp, sizeof(resp), 0);

    printf("Received updated user info.\n");
    printf("Client Root File Descriptor: %i\n", localClient->rfd);

    if (resp.rcode == k_rcRootOperationSuccessful && resp.rflag == k_rfRequestedDataUpdated)
    {
        localClient->rfd = rootServer.sfd;
        localClient->connectedServer = &rootServer;

        // Success
        return 0;
    }

    goto close_root_connection;

// Error
close_root_connection:
    printf("Failed\n");
    printf(RED "Failed to connect to main servers. Error code %i, %i attempts\n" RESET, errno, attempts);
    close(cfd);

    if (errno == ECONNREFUSED)
        printf(RED "Server may be offline. Please try again later.\n" RESET);

    return -1;
}