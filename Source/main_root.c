#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "Headers/server.h"
#include "Headers/client.h"

int main() {
    /*
        Set the rootServer to all 0's
    */
    memset(&rootServer, 0, sizeof(rootServer));
    
    /*
        Create the root server on ROOT_PORT.
        Hosted on the machine thats running the 
        root application.
    */

    if (CreateRootServer() != 0) 
    {
        SystemPrint(RED, false, "Failed Making Root Server. Error Code: %i\n", errno);
        return -1;
    }

    /*
        Listen and accept any client connections
        to the root server
    */
    AcceptClientsToRoot();

    return 0;
}