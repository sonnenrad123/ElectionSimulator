#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>



#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS


#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "../TCPLib/TCPLib.h"
#include "../ListLib/ListLib.h"
#include "../ThreadPoolLib/ThreadPoolLib.h"
#include <process.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define DEFAULT_COUNTERS 5
#define DEFAULT_COUNTER_PORT 29001


bool InitializeWindowsSockets();
void InitElectionOptions();
void PosaljiListu(SOCKET s);
void PackAndSend(SOCKET s);
void SendVotes(int param);
void SendToInfoServer();

CRITICAL_SECTION PORT_NUM, SHARED_NODE, FIRST_THREAD, ALL_VOTES;
int port_number = -1;
bool firstThread = true;

//counter za id
int id_counter = 0;

//pokazivaci na listu opcija za glasanje
CVOR* start = NULL;
CVOR* trenutni = NULL;

//pokazivac na listu predatih glasova
CVOR* startVote = NULL;
CVOR* sharedNode = NULL;

//niz sa prebrojanim glasovima
int* allVotes = NULL;
int allVotesCount = 0;

int  main(void)
{   
    //getchar();
    InitElectionOptions();
    print_options(start);


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

    int workTime = 0;
    printf("Enter work time of ElectionBox in seconds: ");
    scanf_s("%d", &workTime);
    getchar();
    
    
    printf("\nServer ElectionBox initialized, waiting for clients to ask for ID and Vote.\n");
    clock_t timeStart = clock();
    bool firstVote = true;
    char nazivOpcije[7] = "Opcija";
    char charOpcija[3];
    char konkretnaOpcija[9];
    do
    {
        FD_ZERO(&readfds);
        
        clock_t timeStop = clock();
        int duration = (int)(timeStop - timeStart) / CLOCKS_PER_SEC;
        //da li je isteklo vreme glasanja?
        if (duration >= workTime)
           break;

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
                            //printf("Message received from client: %s.\n", recvbuf);
                            if (strcmp("GiveMeID", recvbuf)==0) {//ako je dobio zahtev da isporuci listu opcija i novogenerisan id
                                PosaljiListu(acceptedSocket[i]);
                            }
                            else {//inace je u pitanju glasanje od vec identifikovanog klijenta
                                time_t now = time(0);
                                char* dt = ctime(&now);//daje trenutni datum i vreme u obliku stringa sa \n na kraju koji cemo ukloniti ispod radi lepseg formata
                                if (dt[strlen(dt) - 1] == '\n')
                                    dt[strlen(dt) - 1] = '\0';
                                int *cidp = (int*)recvbuf;//prvo mesto je uvek id
                                int *optselectedp = (int*)(recvbuf + 4);//drugo mesto opcija
                                printf("[%s:]Client successfully voted. Client ID:%d\t\tSelected option: %d \n",dt,*cidp,*optselectedp);
                                konkretnaOpcija[0] = '\0';
                                _itoa(*optselectedp, charOpcija, 10);
                                strcat(konkretnaOpcija, nazivOpcije);
                                strcat(konkretnaOpcija, charOpcija);
                                if (firstVote) {
                                    firstVote = false;                                  
                                    init(&startVote, *optselectedp, konkretnaOpcija, now);
                                }
                                else {
                                    add_to_start(&startVote, *optselectedp, konkretnaOpcija, now);
                                }
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
    // cleanup
    FD_ZERO(&readfds);
    for (int i = 0; i < 3; i++) {
        shutdown(acceptedSocket[i], 2);
        closesocket(acceptedSocket[i]);
    }
    shutdown(listenSocket, 2);
    closesocket(listenSocket);
    WSACleanup();

    char appName[20] = "start Counters.exe ";
    char portString[6];
    char runCounter[25];

    for (int i = 0; i < DEFAULT_COUNTERS; i++) {
        runCounter[0] = '\0';
        _itoa(DEFAULT_COUNTER_PORT+i, portString, 10);
        strcat(runCounter, appName);
        strcat(runCounter, portString);
        printf("Starting counter application...\n");
        system(runCounter);
    }
    //getchar();
    InitializeCriticalSection(&PORT_NUM);
    InitializeCriticalSection(&SHARED_NODE);
    InitializeCriticalSection(&FIRST_THREAD);
    InitializeCriticalSection(&ALL_VOTES);

    CreatePool(DEFAULT_COUNTERS, SendVotes);
    InitializePool();

    for (int i = 0; i < DEFAULT_COUNTERS; i++) {
        DoWork();
    }

    printf("Waiting for counters to finish...\n");

    WaitForThreadsToFinish();
    DestroyPool();
    DeleteCriticalSection(&PORT_NUM);
    DeleteCriticalSection(&SHARED_NODE);
    DeleteCriticalSection(&FIRST_THREAD);
    DeleteCriticalSection(&ALL_VOTES);

    /*for (int i = 0; i < allVotesCount; i++) {
     printf("Option: %d, Votes: %d\n", i + 1, allVotes[i]);
    }*/

    printf("Counters finished. Sending results to InfoServer...\n");
    SendToInfoServer();

    free(allVotes);
    free_list(start);
    free_list(startVote);

    _CrtDumpMemoryLeaks();//detektuje memory leak ako postoji

    printf("\nAll done.\n");
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

void InitElectionOptions() {
    init(&start, 1, "Opcija1\0");
    add_to_end(&start, 2, "Opcija2\0");
    add_to_end(&start, 3, "Opcija3\0");
    add_to_end(&start, 4, "Opcija4\0");
    add_to_end(&start, 5, "Opcija5\0");
    add_to_end(&start, 6, "Opcija6\0");
    add_to_end(&start, 7, "Opcija7\0");
    add_to_end(&start, 8, "Opcija8\0");
    add_to_end(&start, 9, "Opcija9\0");
    add_to_end(&start, 10, "Opcija10\0");
    add_to_end(&start, 11, "Opcija11\0");
    add_to_end(&start, 12, "Opcija12\0");
    add_to_end(&start, 13, "Opcija13\0");
    add_to_end(&start, 14, "Opcija14\0");
    add_to_end(&start, 15, "Opcija15\0");
    add_to_end(&start, 16, "Opcija16\0");
    add_to_end(&start, 17, "Opcija17\0");
    add_to_end(&start, 18, "Opcija18\0");
    add_to_end(&start, 19, "Opcija19\0");
    add_to_end(&start, 20, "Opcija20\0");
    add_to_end(&start, 21, "Opcija21\0");
    add_to_end(&start, 22, "Opcija22\0");
    add_to_end(&start, 23, "Opcija23\0");
    add_to_end(&start, 24, "Opcija24\0");
    add_to_end(&start, 25, "Opcija25\0");
    add_to_end(&start, 26, "Opcija26\0");
    add_to_end(&start, 27, "Opcija27\0");
    add_to_end(&start, 28, "Opcija28\0");
    add_to_end(&start, 29, "Opcija29\0");
    add_to_end(&start, 30, "Opcija30\0");
    add_to_end(&start, 31, "Opcija31\0");
    add_to_end(&start, 32, "Opcija32\0");
    add_to_end(&start, 33, "Opcija33\0");
    add_to_end(&start, 34, "Opcija34\0");
    add_to_end(&start, 35, "Opcija35\0");
    add_to_end(&start, 36, "Opcija36\0");
    add_to_end(&start, 37, "Opcija37\0");
    add_to_end(&start, 38, "Opcija38\0");
    add_to_end(&start, 39, "Opcija39\0");
    add_to_end(&start, 40, "Opcija40\0");
    add_to_end(&start, 41, "Opcija41\0");
    add_to_end(&start, 42, "Opcija42\0");
    add_to_end(&start, 43, "Opcija43\0");
    add_to_end(&start, 44, "Opcija44\0");
    add_to_end(&start, 45, "Opcija45\0");
    add_to_end(&start, 46, "Opcija46\0");
    add_to_end(&start, 47, "Opcija47\0");
    add_to_end(&start, 48, "Opcija48\0");
    add_to_end(&start, 49, "Opcija49\0");
    add_to_end(&start, 50, "Opcija50\0");

    allVotesCount = count_size(start) / sizeof(CVOR);
    allVotes = (int*)malloc(allVotesCount*sizeof(int));
    for (int i = 0; i < allVotesCount; i++) {
        allVotes[i] = 0;
    }
}


void PosaljiListu(SOCKET s) {
    int size = count_size(start);
   // printf("Vote client accepted. Sending list. Size of list: %d\n", size);
    char* buffer = (char*)malloc(size+8);
    char* to_free = buffer;
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
    memcpy(buffer, temp, sizeof(CVOR));//dodali duzinu liste i celu listu u ovom trenutku ostaje da dodamo generisani id
    buffer = buffer + sizeof(CVOR);
    int* id_pointer = (int*)buffer;
    *id_pointer = id_counter;
    id_counter++;

    SendListTCP(s, buffer_start, size + 8);
    free(to_free);
}

void PackAndSend(SOCKET s) {
    int voteOptions = count_size(start) / sizeof(CVOR); //broj mogucih opcija za glasanje

    int voteCount = count_size(startVote) / sizeof(CVOR); //broj glasova
    int voteCountForOneThread = voteCount / DEFAULT_COUNTERS;
    static int spareVotes = voteCount % DEFAULT_COUNTERS;

    EnterCriticalSection(&FIRST_THREAD);
    if (firstThread) {
        firstThread = false;
        sharedNode = startVote;
    }
    if (spareVotes > 0) {
        spareVotes--;
        voteCountForOneThread += 1;
    }

    //printf("\tvotes: %d\n", voteCountForOneThread);
    LeaveCriticalSection(&FIRST_THREAD);

    if (voteCountForOneThread == 0)
        return;

    //izracunavamo koliko je potrebno bajtova memorije inicijalizovati pre slanja
    int sizeOfArray = (voteCountForOneThread + 2) * sizeof(int);

    char* bufferToSend = (char*)malloc(sizeOfArray);
    char* buffer_start = bufferToSend;
    char* to_free = bufferToSend;

    //na prvom mestu u bufferu upisi broj glasova
    int* params = (int*)bufferToSend;
    *params = htonl(voteCountForOneThread);
    bufferToSend = bufferToSend + sizeof(int);

    //na drugom mestu u bufferu upisi broj mogucih opcija
    params = (int*)bufferToSend;
    *params = htonl(voteOptions);
    bufferToSend = bufferToSend + sizeof(int);

    EnterCriticalSection(&SHARED_NODE);
    //printf("\tsize: %d\n", sizeOfArray);
    while (sharedNode->sledeci != NULL) {
        params = (int*)bufferToSend;
        *params = htonl(sharedNode->broj_opcije);
        bufferToSend = bufferToSend + sizeof(int);
        sharedNode = sharedNode->sledeci;
        if (--voteCountForOneThread == 0)
            break;
    }
    if (sharedNode->sledeci == NULL && voteCountForOneThread == 1) {
        params = (int*)bufferToSend;
        *params = htonl(sharedNode->broj_opcije);
        //printf("\tOnly one print here!\n");
    }
    LeaveCriticalSection(&SHARED_NODE);

    int iResult = SendOrdinaryTCP(s, buffer_start, sizeOfArray);

    char recvbuf[DEFAULT_BUFLEN];
    int* temp = NULL;

    iResult = recv(s, recvbuf, DEFAULT_BUFLEN, 0);

    temp = (int*)recvbuf;
    if (iResult > 0) {
        for (int i = 0; i < voteOptions; i++) {
            EnterCriticalSection(&ALL_VOTES);
            allVotes[i] += ntohl(*temp); //zbrajanje svih glasova
            LeaveCriticalSection(&ALL_VOTES);
            temp = temp + 1;
        }
    }


    free(to_free);
}

void SendVotes(int param)
{
    // socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    EnterCriticalSection(&PORT_NUM);
    port_number = port_number + 1;
    LeaveCriticalSection(&PORT_NUM);

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_COUNTER_PORT + port_number);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    PackAndSend(connectSocket);

    // cleanup
    shutdown(connectSocket, 2);
    closesocket(connectSocket);
    WSACleanup();

    return;
}

void SendToInfoServer() {
    // socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(27017);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    if (allVotesCount > 0) {
        char* buffer = (char*)malloc((allVotesCount+1) * sizeof(int));
        char* buffer_start = buffer;
        char* to_free = buffer;

        int* params = (int*)buffer;
        *params = htonl(allVotesCount);
        buffer = buffer + sizeof(int);

        for (int i = 0; i < allVotesCount; i++) {
            params = (int*)buffer;
            *params = htonl(allVotes[i]);
            buffer = buffer + sizeof(int);
        }

        SendOrdinaryTCP(connectSocket, buffer_start, (allVotesCount + 1) * sizeof(int));
        free(to_free);
    }


    // cleanup
    shutdown(connectSocket, 2);
    closesocket(connectSocket);
    WSACleanup();
    return;
};