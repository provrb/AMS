/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       cli.c
 * @brief      all things regarding command line interface for the user
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

#include "Headers/cli.h"
#include "Headers/client.h"
#include "Headers/ccolors.h"

/*
    By default, the user interface is not laoded.
    Changed when LoadClientUserInterface is called
*/
bool userInterfaceLoaded = false;


/**
 * @brief           Prints to the console with a prefix. Wrapper for printf
 * @param[in]       color: color of the text to print
 * @param[in]       prefixNewline: whether to add a newline at the beginning
 * @param[in]       str: string to print
 * @param[in]       ...: formatting
 * @retval          void
 */
void SystemPrint(const char* color, bool prefixNewline, const char* str, ...) { 
    va_list argp;
    va_start(argp, str);
    
    // If you want a newline before all the text
    if (prefixNewline)
        printf("\n");
    
    printf("%s[AMS] ", color);

    // Print the message and variable args
    vfprintf(stdout, str, argp);
    va_end(argp);    
    printf(RESET "\n");
}

void ErrorPrint(bool prefixNewline, const char* errorHeader, const char* errorMessage) 
{ 
    
    // If you want a newline before all the text
    if (prefixNewline)
        printf("\n");
    
    printf("%s[Error] %s\n", RED, errorHeader);
    printf("  Message: %s\n", errorMessage);
    printf("  Code: %i\n", errno);

    // Print the message and variable args
    printf(RESET "\n");
}


/**
 * @brief           Main interface function, runs everything front end
 * @return          void*: Needed for threading
 * @retval          Does not return anything.
 */
void LoadClientUserInterface() {
    if (!userInterfaceLoaded)
    {
        userInterfaceLoaded = true;

        DefaultClientConnectionInfo();
        SplashScreen();

        DisplayCommands(); // Display commands when you load up
        
        // // Create a thread to handle client inpuit
        cpthread tinfo = cpThreadCreate(HandleClientInput, NULL);
        cpThreadJoin(tinfo);
    }
}

/**
 * @brief           Splash screen with text output to the console
 * @return          int
 * @retval          represents success
 */
void SplashScreen() {
    struct tm* timestr = gmt();
    printf(CYN "\nWelcome to AMS! The Time Is: %02d:%02d:%02d UTC\n", timestr->tm_hour, timestr->tm_min, timestr->tm_sec);
    printf("  - Search and create your own servers.\n");
    printf("  - Chat completely anonymously.\n");
    printf("  - No saved data. All chat logs deleted. \n" RESET);
}

/**
 * @brief           Shows all online servers
 * @return          int
 * @retval          success
 */
void DisplayServers() {
    UpdateServerList();

    // Print server list header
    SystemPrint(UNDR WHT, true, "Server List (%i Online):", onlineServers);
    printf("  [ID] - [USR/COUNT] HOST: '' : NAME: ''\n");

    // Print Server List
    for (int servIndex=0; servIndex<onlineServers; servIndex++){
        Server server = serverList[servIndex];
        printf("[%i] - [%i/%i] HOST: %s : NAME: %s\n", server.serverId, server.connectedClients, server.maxClients, server.host.handle, server.alias);
    }
}

/**
 * @brief           Display all application commands
 * @return          int
 * @retval          represents success
 */
void DisplayCommands() {
    int maxCommandLength = 0;

    // Find the maximum length of kCommandName to align the command names
    for (int i = 0; i < kNumOfCommands; i++) {
        int commandLength = strlen(validCommands[i].kCommandName);
        if (commandLength > maxCommandLength) {
            maxCommandLength = commandLength;
        }
    }

    SystemPrint(UNDR WHT, true, "> Showing All (%d) Commands:", kNumOfCommands);

    // Display commands with aligned colons
    for (int i = 0; i < kNumOfCommands; i++) {
        printf("\t%-*s : %s\n", maxCommandLength, validCommands[i].kCommandName, validCommands[i].cmdDesc);
    }
}

void ClearOutput()
{
#ifdef _WIN64
    system("cls");
#elif __unix__
    system("clear");
#endif
}

void DisableTerminalInput() {
    system("stty -echo");
}

void EnableTerminalInput() {
    system("stty echo");
}

void PrintClientMessage(User sender, char* message) {
    // Clear current line and move cursor up one line
    //     // Move cursor up one line

    // Print the message
    // printf("\033[2K");    // Clear the line again to ensure it's empty

    // Move cursor to the next line
    // printf("\x1B[2K");
    // printf("\n"); // trigger any message if there is one which causes us to move down a line
    // printf("\033[1A"); // move up the line
    
    printf("\033[2K\r"); // Clear current line
    fprintf(stdout, "%s: %s\n", sender.handle, message);
    fflush(stdout);
}

int Chatroom(Server* server) {
    // Clear previous terminal output
    ClearOutput();
    ServerPrint(CYN, "You are now connected to '%s'", server->alias);
    ServerPrint(CYN, "Use '--leave' to Disconnect.\n");

    cpthread tinfo = cpThreadCreate(ReceivePeerMessagesOnServer, (void*)server);
    // localClient->connectedServer = server;

    while (1) {
        EnableTerminalInput();
        char* message = malloc(kMaxClientMessageLength);
        if (message == NULL)
            break;

        // take input from the client
        fgets(message, kMaxClientMessageLength, stdin);
        DisableTerminalInput();

        // Remove the trailing newline character if it exists
        if (message[strlen(message) - 1] == '\n') {
            message[strlen(message) - 1] = '\0';
        }

        if (strlen(message) <= 0) {
            free(message);
            continue;
        }

        if (strcmp(localClient->connectedServer->alias, server->alias) != 0) {
            free(message);
            break;
        }

        // message is a command
        int commandResult = PerformClientSideServerCommands(message);
        if (commandResult == 99) {
            free(message);
            break;
        } else if (commandResult != -1) { // command performed so dont echo the msg
            free(message);
            continue;
        }

        CMessage cmsg = {0};
        cmsg.cflag    = k_cfEchoClientMessageInServer;
        cmsg.sender   = *localClient;
        strcpy(cmsg.message, message);
        free(message);

        ResponseCode requestStatus = MakeServerRequest(k_cfEchoClientMessageInServer, *localClient, cmsg);
        // at this point we just sent a message
        // go up a line if we just sent a message
        // then delete that
        // come back down
        printf("\033[A\r"); // move up a line and go to the beginning
        printf("\033[2K"); // Clear current line

        // PrintClientMessage(*localClient, cmsg.message); // print our own message
        sleep(0.3);
    }

    // We leave the connected server so default the local clients 'connectedServer' field to the rootServer again
    localClient->connectedServer = &rootServer;
    ClearOutput();
    SystemPrint(CYN, false, "Disconnected From the Server '%s'", server->alias);
    SplashScreen();
    EnableTerminalInput();
    return 0;
}

/**
 * @brief           Display information about a server
 * @param[in]       serverName: Name of the server to get info about
 * @return          int
 * @retval          Success code
 */
void DisplayServerInfo(char* serverName) {
    UpdateServerList();
    Server* server = ServerFromAlias(serverName);
    
    if (server == NULL) 
    {
        SystemPrint(RED, true, "No Server Found With Name '%s'", serverName);
    }
    else
    {
        SystemPrint(UNDR WHT, true, "Found Information on Server '%s'. Displaying:", serverName);
        printf("\tDomain            : %i\n", server->domain);
        printf("\tType              : %i\n", server->type);
        printf("\tProtocol          : %i\n", server->protocol);
        printf("\tPort              : %i\n", server->port);
        printf("\tConnected Clients : %i\n", server->connectedClients);
        printf("\tMax Clients       : %i (%i/%i)\n", server->maxClients, server->connectedClients, server->maxClients);
        printf("\tServer Alias/Name : %s\n", server->alias);        
    }
}

/**
 * @brief           Simply print the amount of online servers
 * @return          int
 * @retval          0
 */
void TotalOnlineServers() {
    UpdateServerList();
    SystemPrint(WHT UNDR, true, "There Are %i Online Servers", onlineServers);
}

