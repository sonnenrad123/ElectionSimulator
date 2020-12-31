#pragma once
#include <Windows.h>


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
    CRITICAL_SECTION cs;
    Queue()
    {
        front = rear = NULL;

        InitializeCriticalSection(&cs);
    }

    void enQueue(int id, HANDLE t)
    {
        QNode* temp = new QNode(t, id);
        EnterCriticalSection(&cs);

        if (rear == NULL) {
            front = rear = temp;
            LeaveCriticalSection(&cs);
            return;
        }

        rear->next = temp;
        rear = temp;
        LeaveCriticalSection(&cs);
    }

    QNode deQueue()
    {

        if (front == NULL)
            return QNode(NULL, -1);


        EnterCriticalSection(&cs);
        QNode* temp = front;
        QNode ret = *temp;
        front = front->next;


        if (front == NULL)
            rear = NULL;

        delete (temp);
        LeaveCriticalSection(&cs);
        return ret;
    }

};


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