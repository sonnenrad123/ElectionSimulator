// ThreadPoolLib.cpp : Defines the functions for the static library.
//
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#include "pch.h"
#include "ThreadPoolLib.h"
#include "framework.h"
#include <stdio.h>

QNode* new_QNode(HANDLE t, int idd) {
	QNode* new_object = (QNode*)malloc(sizeof(QNode));
	new_object->thread = t;
	new_object->id = idd;
	new_object->next = NULL;
	return new_object;
}

void enQueue(Queue* q, int id, HANDLE t)
{
	QNode* temp = new_QNode(t, id);
	EnterCriticalSection(&q->cs);

	if (q->rear == NULL) {
		q->front = q->rear = temp;
		LeaveCriticalSection(&q->cs);
		return;
	}

	q->rear->next = temp;
	q->rear = temp;
	LeaveCriticalSection(&q->cs);
}


QNode deQueue(Queue* q)
{

	if (q->front == NULL) {
		QNode* ret_empty_pointer = new_QNode(NULL, -1);
		QNode ret_empty = *ret_empty_pointer;
		free(ret_empty_pointer);
		return ret_empty;
	}


	EnterCriticalSection(&q->cs);
	QNode* temp = q->front;
	QNode ret = *temp;
	q->front = q->front->next;


	if (q->front == NULL)
		q->rear = NULL;

	free(temp);
	LeaveCriticalSection(&q->cs);
	return ret;
}


Queue* new_Queue() {
	Queue* new_object = (Queue*)malloc(sizeof(Queue));
	new_object->front = NULL;
	new_object->rear = NULL;
	InitializeCriticalSection(&(new_object->cs));
	return new_object;
}

threadpool* pool;
DWORD* t;
HANDLE* h;
DWORD work_check;
HANDLE wchandle;
void (*task)(int);
int work_count = 0;
int tcount = 0;
HANDLE tasksem;
CRITICAL_SECTION work_cnt_sec;

DWORD WINAPI Worker(LPVOID lpParam) {
	while (true) {
		int id = (int)lpParam;
		WaitForSingleObject(pool->semaphores[id], INFINITE);//cekaj da dobijes neki posao
		EnterCriticalSection(&work_cnt_sec);
		work_count = work_count--;
		LeaveCriticalSection(&work_cnt_sec);
		task(0);//odradi posao
		//sada vrati sebe u red
		//pool->threads_q.enQueue(id, GetCurrentThread);
		enQueue(&(pool->threads_q), id, GetCurrentThread());
		Sleep(100);
	}
}

threadpool* CreatePool(int numt, void (*f)(int)) {
	int i = numt;
	tcount = numt;
	Queue* q = new_Queue();
	threadpool* tp = (threadpool*)malloc(sizeof(threadpool));
	tp->number_of_threads = numt;
	tp->threads_q = *q;
	for (int i = 0; i < numt; i++) {//initialize semaphores
		tp->semaphores[i] = CreateSemaphore(0, 0, 1, NULL);
	}
	t = (DWORD*) malloc(sizeof(DWORD)*numt);
	h = (HANDLE*)malloc(sizeof(HANDLE) * numt);
	pool = tp;
	task = f;
	for (int i = 0; i < numt; i++) {
		h[i] = CreateThread(NULL, 0, Worker, (LPVOID)i, 0, &(t[i]));
		//pool->threads_q.enQueue(i, h[i]);
		enQueue(&(pool->threads_q), i, h[i]);
	}
	free(q);
	return NULL;
}

DWORD WINAPI CheckFOrWOrk(LPVOID lpParam) //proverice posao kada dobije signal da postoji mogucnost da ima taskova neresenih
{
	while (true) {
		WaitForSingleObject(tasksem, INFINITE);
		//QNode t = pool->threads_q.deQueue();//ima posla uzimamo tred
		QNode t = deQueue(&(pool->threads_q));//ima posla uzimamo tred
		while (t.id == -1) {//ako trenutno nema slobodnih pokusavaj stalno dok ne bude prvi slobodan tred
			t = deQueue(&(pool->threads_q));
			Sleep(100);
		}
		ReleaseSemaphore(pool->semaphores[t.id], 1, NULL); //nasli slobodan neka obavi posao
		Sleep(100);
	}
}

void InitializePool() {
	tasksem = CreateSemaphore(0,0, 1, NULL);
	wchandle = CreateThread(NULL, 0, CheckFOrWOrk, (LPVOID)0, 0, &(work_check));
	InitializeCriticalSection(&work_cnt_sec);
}






void DoWork() {
	EnterCriticalSection(&work_cnt_sec);
	work_count = work_count++;
	//printf("%d\n", work_count);
	LeaveCriticalSection(&work_cnt_sec);
	ReleaseSemaphore(tasksem, 1, NULL);//obavestimo semafor da ima razloga za proveru za poslom
	Sleep(50);
}

void WaitForThreadsToFinish() {
	/*
		Ovde je bio bug. Bez ove funkcije moguce je bilo pozvati u mainu mnogo puta vise DoWork pre nego tredovi odrade posao i onda work_count ostaje veci od nule ali se nikad ne release-uje
		semafor za check for work koji uzima tred kada je slobodan i daje mu posao. Tako da ostaje zadanih taskova koji se nikada ne zavrse a main nastavi dalje.
		Na ovaj nacin smo obezbedili da kada se pozove i poslednji DoWork a ima preostalog nagomilanog posla prvo obavi se nagomilani posao
		a onda krene sa daljim izvrsenjem posla.
	*/
	while (work_count > 0) {
		ReleaseSemaphore(tasksem, 1, NULL);//obavestimo semafor da ima razloga za proveru za poslom
		Sleep(200);
	}
	Sleep(100);
}

void DestroyPool() {
	CloseHandle(wchandle);
	QNode qn = deQueue(&(pool->threads_q));
	while (qn.id != -1) {
		qn = deQueue(&(pool->threads_q));
	}
	for (int i = 0; i < tcount; i++) {
		CloseHandle(pool->semaphores[i]);
		CloseHandle(h[i]);
	}
	free(t);
	free(h);
	free(pool);
}
