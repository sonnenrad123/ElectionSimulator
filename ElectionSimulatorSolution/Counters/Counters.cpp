#include <stdio.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include "../TCPLib/TCPLib.h"


#define DEFAULT_BUFLEN 512


bool InitializeWindowsSockets();



int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Invalid number of arguments passed!");
        exit(127);
    }
    char* port = argv[1];
    printf("Counter application successfully started.\n");
    
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server Counter initialized on port %s, waiting for ElectionBox to send data for proccessing.\n", port);

    //do
    //{
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.
        acceptedSocket = accept(listenSocket, NULL, NULL);

        if (acceptedSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        bool toAllocate = true;
        int *arrayToProcess = NULL;
        int *temp = NULL;
        int should_recieve_elements = 0;
        int arrayCount = 0;
        do
        {
            // Receive data until the client shuts down the connection
            iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
            {   
                int currentPosition = 0;
                temp = (int*)recvbuf;
                if (toAllocate) {
                    toAllocate = false;
                    //arrayToProcess = (int*)malloc(ntohl(*temp)*sizeof(int));
                    should_recieve_elements = ntohl(*temp);
                    printf("Receieving %d elements...\n", should_recieve_elements);
                    temp = temp + 1;

                    arrayCount = ntohl(*temp);
                    arrayToProcess = (int*)malloc(ntohl(*temp) * sizeof(int));
                    for (int i = 0; i < arrayCount; i++) {
                        arrayToProcess[i] = 0; //inicijalizujemo listu opcija...
                    }
                    temp = temp + 1;

                    currentPosition += 8;
                }
               
                for (currentPosition; currentPosition < DEFAULT_BUFLEN; currentPosition += 4) {
                    arrayToProcess[ntohl(*temp)-1]++; //indeks predstavlja broj opcije [0..N]
                    temp = temp + 1;
                    if (--should_recieve_elements == 0) {
                        if (arrayCount > 0) {
                            char* buffer = (char*)malloc(arrayCount * sizeof(int));
                            char* buffer_start = buffer;

                            for (int i = 0; i < arrayCount; i++) {
                                int* params = (int*)buffer;
                                *params = htonl(arrayToProcess[i]);
                                buffer = buffer + sizeof(int);
                            }

                            SendOrdinaryTCP(acceptedSocket, buffer_start, arrayCount * sizeof(int));
                        }
                        break;
                    }
                }
     
            }
            else if (iResult == 0)
            {
                
                // connection was closed gracefully
                printf("Connection with client closed.\n");
                closesocket(acceptedSocket);
            }
            else
            {
                // there was an error during recv
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocket);
            }
        } while (iResult > 0);

        
        printf("Counted votes for: \n");
        for (int i = 0; i < arrayCount; i++) {
            if(arrayToProcess[i] !=0)
                printf("Option: %d, Votes: %d\n", i + 1, arrayToProcess[i]);
        }
        // here is where server shutdown loguc could be placed

   // } while (1);

    // shutdown the connection since we're done
    /*iResult = shutdown(acceptedSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocket);
        WSACleanup();
        return 1;
    }*/

    // cleanup
    closesocket(listenSocket);
    closesocket(acceptedSocket);
    WSACleanup();

    getchar();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
