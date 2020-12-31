#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "TCPLib.h"
#include "ListLib.h"
#include <iostream>
using namespace std;

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

CRITICAL_SECTION cs;

#pragma pack(4)
struct QNode {
    HANDLE thread;
    int id;
    QNode* next;
    QNode(HANDLE t, int idd)
    {
        thread = t;
        id = idd;
        next = NULL;
    }
};

struct Queue {
    QNode* front, * rear;
    Queue()
    {
        front = rear = NULL;
    }

    void enQueue(int id, HANDLE t)
    {

        // Create a new LL node 
        QNode* temp = new QNode(t, id);

        // If queue is empty, then 
        // new node is front and rear both 
        EnterCriticalSection(&cs);

        if (rear == NULL) {
            front = rear = temp;
            LeaveCriticalSection(&cs);
            return;
        }

        // Add the new node at 
        // the end of queue and change rear 
        rear->next = temp;
        rear = temp;
        LeaveCriticalSection(&cs);
    }

    // Function to remove 
    // a key from given queue q 
    QNode deQueue()
    {
        // If queue is empty, return false. 
        if (front == NULL)
            return QNode(NULL, -1);

        // Store previous front and 
        // move front one node ahead 
        EnterCriticalSection(&cs);
        QNode* temp = front;
        QNode ret = *temp;
        front = front->next;

        // If front becomes NULL, then 
        // change rear also as NULL 
        if (front == NULL)
            rear = NULL;

        delete (temp);
        LeaveCriticalSection(&cs);
        return ret;
    }
};


//thread queue
Queue q;
HANDLE hSemaphores[3];
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);} 




bool InitializeWindowsSockets();


DWORD WINAPI VoteThread(LPVOID lpParam)
{
    while (true) {
        int id = (int)lpParam;
        WaitForSingleObject(hSemaphores[id], INFINITE);//cekaj dok se ne zatrazi od tebe da radis

        if (InitializeWindowsSockets() == false)
        {
            return -1;
        }
        printf("Voter entering elections. Voter thread id = %d\n", GetCurrentThreadId());

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
            return -1;
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
            return -1;
        }
        iResult = SendIDRequestTCP(connectSocket);
        printf("Bytes Sent: %ld\n", iResult);

        char* raw_data = (char*)malloc(1000);

        iResult = recv(connectSocket, raw_data, 1000, 0);
        int* size_p = (int*)raw_data;
        printf("Size of list: %d\n", *size_p);
        char* list_data = raw_data + 4;
        int bytes_left = *size_p;
        for (int i = 0; i < bytes_left; ) {
            CVOR* c = (CVOR*)(list_data + i);
            printf("%d:[%s]\n", c->broj_opcije, c->naziv_opcije);
            i = i + sizeof(CVOR);
        }


        closesocket(connectSocket);
        WSACleanup();
        q.enQueue(id, NULL);
    }
}



//-------------------------------------





VOID WINAPI ThreadPoolCallBack(PTP_CALLBACK_INSTANCE instance, PVOID param)
{
    if (InitializeWindowsSockets() == false)
    {
        return;
    }
    printf("Voter entering elections. Voter thread id = %d\n", GetCurrentThreadId());

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
    }
    iResult = SendIDRequestTCP(connectSocket);
    printf("Bytes Sent: %ld\n", iResult);

    char* raw_data = (char*)malloc(1000);

    iResult = recv(connectSocket, raw_data, 1000, 0);
    int* size_p = (int*)raw_data;
    printf("Size of list: %d\n", *size_p);
    char* list_data = raw_data + 4;
    int bytes_left = *size_p;
    for (int i = 0; i < bytes_left; ) {
        CVOR* c = (CVOR*)(list_data+i);
        printf("%d:[%s]\n", c->broj_opcije, c->naziv_opcije);
        i = i + sizeof(CVOR);
    }


    closesocket(connectSocket);
    WSACleanup();
    Sleep(100); 
    return;
}

int main()
{
    hSemaphores[0] = CreateSemaphore(0, 0, 1, NULL);
    hSemaphores[1] = CreateSemaphore(0, 0, 1, NULL);
    hSemaphores[2] = CreateSemaphore(0, 0, 1, NULL);
    InitializeCriticalSection(&cs);

    int number_of_voters = -1;
    while (number_of_voters == -1 && number_of_voters <= 0) {
        printf("Voting simulator starting. Please enter number of voters: \n");
        scanf("%d", &number_of_voters);
    }

    /*PTP_POOL tPool;
    tPool = CreateThreadpool(NULL);             

    DWORD dwMaxThread = 3;  //maksimalno tri glasaca istovremeno                    
    
    SetThreadpoolThreadMaximum(tPool, dwMaxThread); 
    SetThreadpoolThreadMinimum(tPool, 1);       

    TP_CALLBACK_ENVIRON tcEnv;
    InitializeThreadpoolEnvironment(&tcEnv);    
    SetThreadpoolCallbackPool(&tcEnv, tPool);  

    printf("The number of threads in the thread pool is: %d\n\n", dwMaxThread);
   
    for (int i = 0; i < number_of_voters; i++)
    {
        TrySubmitThreadpoolCallback(ThreadPoolCallBack, (PVOID)i, &tcEnv);
    }*/


    DWORD t1, t2, t3;
    HANDLE ht1, ht2, ht3;
    int id1 = 0;
    int id2 = 1;
    int id3 = 2;

    ht1 = CreateThread(NULL, 0, VoteThread, (LPVOID)0, 0, &t1);
    ht2 = CreateThread(NULL, 0, VoteThread, (LPVOID)1, 0, &t2);
    ht3 = CreateThread(NULL, 0, VoteThread, (LPVOID)2, 0, &t3);
    
   
    q.enQueue(0, ht1);
    q.enQueue(1, ht2);
    q.enQueue(2, ht3);

    for (int i = 0; i < number_of_voters; i++) {
        QNode t = q.deQueue();
        while (t.id == -1) {
            t = q.deQueue();
            Sleep(10);
        }
        ReleaseSemaphore(hSemaphores[t.id], 1, NULL);//thread sam sebe vraca u red
    }

    
    Sleep(100000);
    SAFE_DELETE_HANDLE(hSemaphores[0]);
    SAFE_DELETE_HANDLE(hSemaphores[1]);
    SAFE_DELETE_HANDLE(hSemaphores[2]);
    SAFE_DELETE_HANDLE(ht1);
    SAFE_DELETE_HANDLE(ht2);
    SAFE_DELETE_HANDLE(ht3);

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