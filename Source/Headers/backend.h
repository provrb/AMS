/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       backend.h
 * @brief      control all backend operations for the interface
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

#ifndef __BACKEND_H__
#define __BACKEND_H__   

#include <string.h>          
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "cli.h"
#include "server.h"
#include "tools.h"
#include "client.h"

/*
    Get a server struct from a server name

    If the server does not exist, NULL will be returned.
    Otherwise, a pointer to that 'Server' struct will be returned.
*/
Server* ServerFromAlias(char* alias);

/*
    Exit the client application.

    Safely removes the client and makes a request
    to remove the client from any servers and update
    the statistics on the root server. i.e online clients.
*/
void ExitApp();

/* Removed */
// void EnableDebugMode();

/*
    Connect the local client to a server
    from a 'Server' struct
*/
void JoinServer(Server* server);

/*
    Connect the local client o a server
    using a server name
*/
void JoinServerByName(char* name);

#endif // __BACKEND_H__