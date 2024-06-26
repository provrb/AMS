# AMS
TEJ2OI Final Project - Anonymous Messaging Service (IRC) 
IRC inspired project for Computer Engineering Technology final project.


**RAN AND TESTED ON LINUX MINT, DESGIGNED FOR LINUX**


**- PM'ing was going to be a feature but I thought of it 2 hours before class started. Did not have sufficient time to implement.**



# How it works
- There must be a root server online for anyone to connect and the program to work


1. Client selects name
2. Client sends request to root server asking to join
3. Client gets accepted and information about the client is updated on both sides


- Client can make servers, view servers, information about servers-
- Client does not have access to root server, they do have info about servers though
- In those servers, clients can talk to each other
- Host can kick any client they want.
- Once the host leaves the server is destroyed.

### Encryption?
- WHen sending messages to other clients, there is a simple layer of encryption.
- The message is encrypted on the sender client side, and decrypted on the receivers side.
- The message is XOR'd with the servers unique id as the constant
- I had plans to implemented AES, however thought it wouldn't be needed for a school project
- In no way is this application safe from man in the middle attacks, because I didn't build it for that, I built it to demonstrate an idea.

# Backend
int CreateRootServer();
- Creates a special custom Server struct
- Will be the server clients connect to
- Created on port 18081 with a TCP socket
- Socket binds to that port. binds to INADDR_ANY (no specific ip to bind to)
- Root server uses listen() for any client connections

void* AcceptClientsToRoot();
- While loop that accepts connections with accept()
- Attempts to receive info about the client sent by the client on join
- Returns a RootResponse to the client telling them they have been connected or have not
- A thread is created to receive requests from that client

# Sending and receiving- How it works
On the root server
- A thread is created to perform root requests from each client
- Clients make requests to the root server using MakeRootRequest();
- Root request will perform the request with DoRootRequest();
- Will then return a response to the client with any neccessary info

On a channel/server
- Servers perform just like the root server
- Accepts, binds, creates, listens like the root server!
- Performs requests like leaving, kicking, echoing message
- Has acceess to root server info
- Assigned a client as a host
- Makes sure to update all info on root server!
- Echos client by sending a client-sent message to all other clients connected
