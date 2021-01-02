

#include "pch.h"
#include "framework.h"
#include "ListLib.h"

void* safe_malloc(size_t n)
{
    void* p = malloc(n);
    if (p == NULL) {
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", n);
        abort();
    }
    return p;
}

void print_options(CVOR* start) {
    CVOR* temp = start;
    while (temp->sledeci != NULL) {
        printf("%d: [%s] \n", temp->broj_opcije, temp->naziv_opcije);
        temp = temp->sledeci;
    }
    printf("%d: [%s] \n", temp->broj_opcije, temp->naziv_opcije);
}


void init(CVOR** start,int broj_opcije,const char* naziv_opcije) {
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    strcpy_s(c->naziv_opcije, naziv_opcije);
    c->vreme_glasanja = NULL;
    c->sledeci = NULL;
    *start = c;
}

void init(CVOR** start, int broj_opcije,const char* naziv_opcije, time_t time) {
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    strcpy_s(c->naziv_opcije, naziv_opcije);
   // memcpy(c->naziv_opcije, naziv_opcije, strlen(naziv_opcije) + 1);
    c->vreme_glasanja = time;
    c->sledeci = NULL;
    *start = c;
}

void add_to_start(CVOR** start, int broj_opcije, char* naziv_opcije) {
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    c->vreme_glasanja = NULL;
    
    strcpy_s(c->naziv_opcije, naziv_opcije);
    c->sledeci = *start;
    *start = c;
}

void add_to_start(CVOR** start, int broj_opcije, char* naziv_opcije, time_t time) {
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    c->vreme_glasanja = time;
    strcpy_s(c->naziv_opcije, naziv_opcije);
    c->sledeci = *start;
    *start = c;
}

void add_to_end(CVOR** start, int broj_opcije, char* naziv_opcije) {
    CVOR* temp;
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    c->vreme_glasanja = NULL;
    strcpy_s(c->naziv_opcije, naziv_opcije);
    temp = *start;
    while (temp->sledeci != NULL) {
        temp = temp->sledeci;
    }
    temp->sledeci = c;
    c->sledeci = NULL;

}

void add_to_end(CVOR** start, int broj_opcije, char* naziv_opcije, time_t time) {
    CVOR* temp;
    CVOR* c = (CVOR*)safe_malloc(sizeof(CVOR));
    c->broj_opcije = broj_opcije;
    c->vreme_glasanja = time;
    strcpy_s(c->naziv_opcije, naziv_opcije);
    temp = *start;
    while (temp->sledeci != NULL) {
        temp = temp->sledeci;
    }
    temp->sledeci = c;
    c->sledeci = NULL;
}

int count_size(CVOR* start) {
    int size = 0;
    CVOR* temp = start;
    while (temp->sledeci != NULL) {
        size = size + sizeof(CVOR);
        temp = temp->sledeci;
    }
    size = size + sizeof(CVOR);
    return size;
}

//TODO: Unistavanje liste radi prevencije curenja memorije