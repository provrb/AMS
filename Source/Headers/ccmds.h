/**
 * ****************************(C) COPYRIGHT 2023 ****************************
 * @file       ccmds.h
 * @brief      Client commands used in the cli
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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "backend.h"

/*
    Specific functions that can be ran
    through the command-line by the client.
    
    These command functions do NOT have parameters.
*/
typedef void (*CommandFunction)();

/*
    A structure representing a typeable command
    for the client.

    A client can enter 'kCommandName' to perform a command,
    provided that the arguments are all entered correctly.
    
    If 'function' is null, it means the function takes
    arguments and they cannot be passed through a simple
    'CommandFunction'' function like in the struct below.
*/
typedef struct CMD {
    const char* kCommandName;
    char* cmdDesc;
    CommandFunction function;    
} Command; 

/*
    Array of 'Command' structs that 
    represent valid commands 
    the local client can input and run.
*/
extern Command   validCommands[];

/*
    Number of valid commands.
    Essentially the length of 'validCommands' array.
*/
extern const int kNumOfCommands;

#endif // __INTERFACE_H__