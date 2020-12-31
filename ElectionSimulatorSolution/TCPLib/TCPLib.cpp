// TCPLib.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512

int SendOrdinaryTCP(SOCKET s, char* data, unsigned int len) {
    int iResult = send(s, data, (int)len, 0);
    if (iResult == SOCKET_ERROR)//ako nije uspesno poslato vrati -1
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return -1;
    }
    else {//inace vrati broj poslatih bajta
        return iResult;
    }
}

int RecvOrdinaryTCP(SOCKET s, char* data) {
    int iResult = recv(s, data, DEFAULT_BUFLEN, 0);
    if (iResult >= 0) { //ako je close connection ili uspesno primljena poruka vrati iResult
        return iResult;
    }
    else {//doslo je do greske
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }
}


int SendIDRequestTCP(SOCKET s) {
    char* messageToSend = (char*)malloc(20);

    memcpy(messageToSend, "GiveMeID\0", strlen("GiveMeID") + 1);

    int iResult = send(s, messageToSend, (int)strlen(messageToSend)+1, 0);
    if (iResult == SOCKET_ERROR)//ako nije uspesno poslato vrati -1
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return -1;
    }
    else {//inace vrati broj poslatih bajta
        return iResult;
    }

}

int SendListTCP(SOCKET s, char* start, int broj_bajta) {
    int iResult = send(s, start, broj_bajta,0);
    if (iResult == SOCKET_ERROR)//ako nije uspesno poslato vrati -1
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return -1;
    }
    else {//inace vrati broj poslatih bajta
        return iResult;
    }
}