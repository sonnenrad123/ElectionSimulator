#pragma once

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

/* funkcija za slanje najosnovnijih podataka putem tcp protokola
	S: socket za komunikaciju
	data: podaci za slanje
	len: duzina podataka
	Povratna vrednost predstavlja broj poslatih bajta
*/
int SendOrdinaryTCP(SOCKET s, char* data, unsigned int len);


/* funkcija za prijem najosnovnijih podataka putem tcp protokola
	S: socket za komunikaciju
	data: podaci za slanje
	Povratna vrednost predstavlja broj primljenih bajta
*/
int RecvOrdinaryTCP(SOCKET s, char* data);


/* funkcija za slanje zahteva za brojackim ID-em putem tcp protokola
	S: socket za komunikaciju
	Povratna vrednost predstavlja broj poslatih/primljenih bajta
*/
int SendIDRequestTCP(SOCKET s);

/*
* funkcija za prijem liste podataka putem tcp protokola
* S: socket za komunikaciju
* start: podaci za slanje
* broj_bajta: broj bajta koje zauzima lista
*/
int SendListTCP(SOCKET s, char* start, int broj_bajta);