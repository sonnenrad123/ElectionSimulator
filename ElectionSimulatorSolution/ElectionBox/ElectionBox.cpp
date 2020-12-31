#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "TCPLib.h"
#include "ListLib.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

bool InitializeWindowsSockets();
void InicijalizujListuOpcija();
void PosaljiListu(SOCKET s);








CVOR* start = NULL;
CVOR* trenutni = NULL;

int  main(void)
{
    InicijalizujListuOpcija();
    stampaj_listu_opcija(start);
    


    //Prvi deo server aplikacije prijem glasaca i njihovih glasova
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket[3];
    for (int i = 0; i < 3; i++) {
        acceptedSocket[i] = INVALID_SOCKET;
    }
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
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
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

    /*-----------------------------------------------------*/

    //postavi listen socket u neblokirajuci rezim
    unsigned long mode = 1;
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR)
    {
        printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
        return 1;
    }
    /*-----------------------------------------------------*/


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

    /*Napravi read set*/
    fd_set readfds;
    FD_ZERO(&readfds);

    timeval timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;
    /*--------------------------*/



    printf("Server ElectionBox initialized, waiting for clients to ask for ID and Vote.\n");

    do
    {
        FD_ZERO(&readfds);
        //dodamo listen u set
        FD_SET(listenSocket, &readfds);
        //ako se neko vec konektovao dodajemo i acceptedsocket njegov u set
        for (int i = 0; i < 3; i++) {
            if (acceptedSocket[i] != INVALID_SOCKET)
            {
                FD_SET(acceptedSocket[i], &readfds);
            }
        }

        int result = select(0, &readfds, NULL, NULL, &timeVal);



        if (result == 0) {
            //printf("Vreme cekanja select-a isteklo. Nije se desio dogadjaj.\n");
            Sleep(100);
        }
        else if (result == SOCKET_ERROR) {
            printf("Select failed with error: %d\n", WSAGetLastError());
        }
        else {
            //desio se dogadjaj za konekciju na server
            if (FD_ISSET(listenSocket, &readfds)) {
                // nadjemo prvi slobodan slobodan slot i dodelimo korisniku
                for (int i = 0; i < 3; i++) {
                    if (acceptedSocket[i] == INVALID_SOCKET) {
                        acceptedSocket[i] = accept(listenSocket, NULL, NULL);

                        if (acceptedSocket[i] == INVALID_SOCKET)
                        {
                            printf("accept failed with error: %d\n", WSAGetLastError());
                            closesocket(listenSocket);
                            WSACleanup();
                            return 1;
                        }
                        break;
                    }
                }

            }

            for (int i = 0; i < 3; i++) {
                //ako se neki klijent vec povezao proveravamo i njega
                if (acceptedSocket[i] != INVALID_SOCKET) {
                    //ako postoji poruka koja treba biti ispisana ispisemo je
                    if (FD_ISSET(acceptedSocket[i], &readfds)) {
                        iResult = RecvOrdinaryTCP(acceptedSocket[i], recvbuf);
                        if (iResult > 0)
                        {
                            printf("Message received from client: %s.\n", recvbuf);
                            if (strcmp("GiveMeID", recvbuf)==0) {
                                PosaljiListu(acceptedSocket[i]);
                            }
                        }
                        else if (iResult == 0)
                        {
                            // connection was closed gracefully
                            printf("Connection with client closed.\n");
                            acceptedSocket[i] = INVALID_SOCKET;
                        }
                    }


                }
            }



        }

        /*if (acceptedSocket != INVALID_SOCKET) {
            FD_CLR(acceptedSocket, &readfds);

        }
        FD_CLR(listenSocket, &readfds);*/


    } while (1);

    // dalje potrebno ocistiti sve od gore i preci na komunikaciju sa brojacima i td...

    // cleanup
    closesocket(listenSocket);
    WSACleanup();

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

void InicijalizujListuOpcija() {
    inicijalizuj(&start, 1, "Opcija1\0");
    dodaj_na_kraj(&start, 2, "Opcija2\0");
    dodaj_na_kraj(&start, 3, "Opcija3\0");
    dodaj_na_kraj(&start, 4, "Opcija4\0");
    dodaj_na_kraj(&start, 5, "Opcija5\0");
    dodaj_na_kraj(&start, 6, "Opcija6\0");
    dodaj_na_kraj(&start, 7, "Opcija7\0");
    dodaj_na_kraj(&start, 8, "Opcija8\0");
    dodaj_na_kraj(&start, 9, "Opcija9\0");
    dodaj_na_kraj(&start, 10, "Opcija10\0");
    dodaj_na_kraj(&start, 11, "Opcija11\0");
    dodaj_na_kraj(&start, 12, "Opcija12\0");
    dodaj_na_kraj(&start, 13, "Opcija13\0");
    dodaj_na_kraj(&start, 14, "Opcija14\0");
    dodaj_na_kraj(&start, 15, "Opcija15\0");
    dodaj_na_kraj(&start, 16, "Opcija16\0");
}


void PosaljiListu(SOCKET s) {
    int size = izracunaj_zauzece(start);
    printf("Vote client accepted. Sending list. Size of list: %d\n", size);
    char* buffer = (char*)malloc(size+4);
    char* buffer_start = buffer;
    int* duzina = (int*)buffer;
    *duzina = size;
    buffer = buffer + 4;
    CVOR* temp = start;
    while (temp->sledeci != NULL) {
        memcpy(buffer, temp, sizeof(CVOR));
        buffer = buffer + sizeof(CVOR);
        temp = temp->sledeci;
    }
    memcpy(buffer, temp, sizeof(CVOR));
    SendListTCP(s, buffer_start, size + 4);
}