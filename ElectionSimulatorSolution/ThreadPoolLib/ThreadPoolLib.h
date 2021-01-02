#pragma once
#include <Windows.h>
#include <stdlib.h>

#pragma pack(4)
struct QNode {
    HANDLE thread;
    int id;
    QNode* next;
};

QNode* new_QNode(HANDLE t, int idd);

struct Queue {
    QNode* front, * rear;
    CRITICAL_SECTION cs;

};

void enQueue(Queue* q, int id, HANDLE t);

QNode deQueue(Queue* q);

Queue* new_Queue();






#pragma pack(4)
typedef struct tp_st {
    int number_of_threads;
    Queue threads_q;
    HANDLE semaphores[100]; //tpool moze imati maks 100 tredova 
}threadpool;

/*
* Kreira pool sa odredjenih brojem tredova
*   numt-broj tredova
*/
threadpool* CreatePool(int numt, void (*f)(int));

/*
* Inicijalizu pool da otpocne proveru stanja zadataka
*/
void InitializePool();


/*
* Zadaje neki task pool-u da odradi
*   f - pokazivac na funkciju sa jednim int parametrom
*/
void DoWork();

/*
* Unistava pool i oslobadja resurse
*/
void DestroyPool();