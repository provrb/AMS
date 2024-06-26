/**
 * The minimum and maximum allowed values
 * for certain parameters and input
*/

// The maximum amount a specific value/parameter is allowed to be
typedef enum MaximumValues
{
    kMaxServerMembers       = 100, // Can't set a servers maxClient more than this
    kDefaultMaxClients      = 30, // Default parameter of a servers allowed clients
    kMaxServerAliasLength   = 32,  // Max server name length in chars
    kMaxClientHandleLength  = 20, // Max client user name length in chars
    kMaxClientMessageLength = 2000, // Max msg length in chars
    kMaxGlobalClients       = 1000, // Max allowed clients connected to app
    kMaxServersOnline       = 30, // Max allowed servers online
    kMaxCommandLength       = 300,
} MaxValue;

// The minimum amount a specific value/parameter is allowed to be
typedef enum MinimumValues
{
    kMinServerAliasLength  = 3,
    kMinClientHandleLength = 3 
} MinValue;