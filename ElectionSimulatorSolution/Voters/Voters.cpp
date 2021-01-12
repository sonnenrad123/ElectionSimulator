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
#include <stdio.h>
#include <conio.h>
#include "../TCPLib/TCPLib.h"
#include "../ListLib/ListLib.h"
#include <iostream>
#include "../ThreadPoolLib/ThreadPoolLib.h"
#include <time.h>

using namespace std;

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016


bool InitializeWindowsSockets();
int GenerateRandomVoteNumber(int num_of_candidates);

void GoVote(int param) {
    printf("Voter entering elections. Voter thread id = %d\n", GetCurrentThreadId());
    if (InitializeWindowsSockets() == false)
    {
        return;
    }
    //konektuje se na server trazi id da bi glasao
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return;
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);

    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return;
    }
    iResult = SendIDRequestTCP(connectSocket);
    //printf("Bytes Sent: %ld\n", iResult);

    char* raw_data = (char*)malloc(2500);

    iResult = recv(connectSocket, raw_data, 2500, 0);
    int* size_p = (int*)raw_data;
    //printf("Size of list: %d\n", *size_p);
    
    char* list_data = raw_data + 4;
    int bytes_left = *size_p;//cela duzina liste primljena porukom

    int number_of_candidates = bytes_left / sizeof(CVOR); // toliko ima elemenata u listi tj opcija za glasanje da napravimo niz opcija i random izaberemo jednog kandidata
    int* options_array = (int*)malloc(number_of_candidates * 4);
    int opt_cnt = 0;

    //printf("Number of candidates: %d\n", number_of_candidates);
    //napravimo niz broja opcija da bi iz njega izabrali random opciju i glasali za nju
    for (int i = 0; i < bytes_left; ) {
        CVOR* c = (CVOR*)(list_data + i);
        //printf("%d:[%s]\n", c->broj_opcije, c->naziv_opcije);

        options_array[opt_cnt] = c->broj_opcije;//punimo niz opcijama i na kraju samo izaberemo random element iz niza i glasamo za njega
        opt_cnt++;

        i = i + sizeof(CVOR);
        
    }
    int* id_pointer = (int*)(list_data + bytes_left);
    int selected_option_index_for_vote = GenerateRandomVoteNumber(number_of_candidates);
    int id_to_send = *id_pointer;
    iResult = SendVoteOptionTCP(connectSocket, id_to_send, options_array[selected_option_index_for_vote]);
    if (iResult == -1) {
        printf("Sending vote option failed.\n");
        free(options_array);
        free(raw_data);
        closesocket(connectSocket);
        WSACleanup();
        return;
    }
    
    printf("VOTING COMPLETED FOR CLIENT WITH ID %d: Voted for option number %d.\n",id_to_send, options_array[selected_option_index_for_vote]);

    
    free(options_array);
    free(raw_data);
    closesocket(connectSocket);
    WSACleanup();
}




int main()
{
    srand(time(0));
    int number_of_voters = -1;
    while (number_of_voters == -1 && number_of_voters <= 0) {
        printf("Voting simulator starting. Please enter number of voters: \n");
        scanf_s("%d", &number_of_voters);
    }

    CreatePool(3, GoVote);
    InitializePool();

    int cnt = 0;
    
    for (int i = 0; i < number_of_voters; i++) {
        DoWork();
    }
    
    WaitForThreadsToFinish();
    DestroyPool();
    _CrtDumpMemoryLeaks();//detektuje memory leak ako postoji
    printf("All client voted.");
 
    getchar();
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

int GenerateRandomVoteNumber(int num_of_candidates) {
    
    return (rand() % (num_of_candidates));
}