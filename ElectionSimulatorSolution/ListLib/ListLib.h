#pragma once
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
/*
* Lista ce imati polja za ime opcije,broj opcije 
* i vreme glasanja koje ce se popunjavati u trenutcima kada je potrebno
*/
#pragma pack(4)
typedef struct cvor_st {
    char naziv_opcije[31];
    int broj_opcije;
    time_t vreme_glasanja;
    cvor_st* sledeci;
}CVOR;

/*
* Stampa broj opcije i naziv opcije
*/
void print_options(CVOR* start);

/*
* Inicijalizacija liste bez vremena glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
*/
void init(CVOR** start, int broj_opcije,const char* naziv_opcije);
/*
* Inicijalizacija sa vremenom glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
* time - vreme glasanja
*/
void init(CVOR** start, int broj_opcije,const char* naziv_opcije, time_t time);

/*
* Dodavanje na pocetak liste bez vremena glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
*/
void add_to_start(CVOR** start, int broj_opcije, char* naziv_opcije);

/*
* Dodavanje na pocetak liste sa vremenom glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
* time - vreme glasanja
*/
void add_to_start(CVOR** start, int broj_opcije, char* naziv_opcije, time_t time);

/*Dodavanje na kraj liste bez vremena glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
*/
void add_to_end(CVOR * *start, int broj_opcije, char* naziv_opcije);

/*
* Dodavanje na kraj liste sa vremenom glasanja
* start - pocetni cvor liste
* broj_opcije - broj opcije na listicu
* naziv_opcije - naziv opcije na listicu
* time - vreme glasanja
*/
void add_to_end(CVOR** start, int broj_opcije, char* naziv_opcije, time_t time);

/*
* Vraca memorijsku velicinu liste da bi znali koliko je zauzece fajlova koje saljemo kroz socket
*/
int count_size(CVOR* start);

/*
* Potrebno implementirati ciscenje memorije. Videce vec neko od nas dvojice komentar
*/