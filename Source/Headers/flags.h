/**
 * Flags to set when we ant the root server to perform commands
 * Tells the root server what commands to perform with the data
 */

// NOTE: Whenever you see cf, it is a flag to set 
// client side for the root server to perform that command
typedef enum
{
    k_cfNone = -123, // No Command to perform

    // Commands regarding server browser/explorer
    k_cfAppendServer = 10,  // Append server to explorer 
    k_cfRemoveServer = -10, // Remove server from explorer
    k_cfRequestServerList = 100, // Get all updated server list
    k_cfMakeNewServer = 920, // Create a new server clients can connect to
    k_cfRSUpdateServerWithNewInfo = 892, // A server has updated info to be pushed onto the root server
    k_cfClientDeclinedPrivateMessage = -193,
    k_cfClientAcceptedPrivateMessage = -403,

    // Commands regarding client
    k_cfDisconnectClientFromRoot = 914, // Remove client from root server
    k_cfConnectClientToServer = 932, // User joined specifed server
    k_cfBanClientFromServer = 482, // Ban a client from the server. Must be host
    k_cfKickClientFromServer = 423, // Remove client from the server. Must be host
    k_cfAddClientToServer = 10023, // Add client to server list
    k_cfSSUpdateClientWithNewInfo = 122, // Update client in rootConnectedClients
    k_cfConnectedServerShutDown = 829, // THe server the client was connected to was shut down
    k_cfClientRequestPrivateMessage = 9403,

    // Message command
    k_cfEchoClientMessageInServer  = 1840, // Send message from client to all clients in server 
    k_cfPrintPeerClientMessage = 1323, // Receive a message from client in a server and print it out
    k_cfPrintServerAnnouncement = 9301 // Print a message sent from the server, aka server announcement
} CommandFlag;


/**
 * Flags to tell the client what happened
 * After the response.
 * Different from ResponseCode as ResponseFlags-
 * gives more detailed explanation on what happened with the clients request
 */
typedef enum
{
    k_rfNoResponse = 1,   // No flags. Default
    k_rfRequestedDataUpdated = 903, // The data was updated somehow.
    k_rfNoDataChanged = 923, // No data in the request was modified
    k_rfSentDataWasUnused = 103, // No new data was added. Return value is likely null
    k_rfValueReturnedFromRequest = 823, // A value has been returned
    k_rfNoValueReturnedFromRequest = -823, // No return value. Return value is null
} ResponseFlag;

 
/**
 * Response codes from the root server
 * After a request is made to it
 * Indicate what happened- is it successful or fail
 */
typedef enum
{
    k_rcRootOperationSuccessful = 0,    // Normal Success Code.
    k_rcInternalServerError = -192, // Something went wrong on the servers side
    
    // Types of errors
    k_rcErrorPortInUse = -302, // The port client tried to make a server with is in use.
} ResponseCode;