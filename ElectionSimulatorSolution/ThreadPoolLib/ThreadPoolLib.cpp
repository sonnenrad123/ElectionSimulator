// ThreadPoolLib.cpp : Defines the functions for the static library.
//
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#include "pch.h"
#include "ThreadPoolLib.h"
#include "framework.h"
#include <stdlib.h>

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
		task(0);//odradi posao
		EnterCriticalSection(&work_cnt_sec);
		work_count = work_count--;
		LeaveCriticalSection(&work_cnt_sec);
		//sada vrati sebe u red
		pool->threads_q.enQueue(id, GetCurrentThread);
		Sleep(100);
	}
}

threadpool* CreatePool(int numt, void (*f)(int)) {
	int i = numt;
	tcount = numt;
	Queue q;
	threadpool* tp = (threadpool*)malloc(sizeof(threadpool));
	tp->number_of_threads = numt;
	tp->threads_q = q;
	for (int i = 0; i < numt; i++) {//initialize semaphores
		tp->semaphores[i] = CreateSemaphore(0, 0, 1, NULL);
	}
	t = (DWORD*) malloc(sizeof(DWORD)*numt);
	h = (HANDLE*)malloc(sizeof(HANDLE) * numt);
	pool = tp;
	task = f;
	for (int i = 0; i < numt; i++) {
		h[i] = CreateThread(NULL, 0, Worker, (LPVOID)i, 0, &(t[i]));
		pool->threads_q.enQueue(i, h[i]);
	}
	return NULL;
}

DWORD WINAPI CheckFOrWOrk(LPVOID lpParam) //will check for work when semaphore indicates there is new work to do
{
	while (true) {
		WaitForSingleObject(tasksem, INFINITE);
		QNode t = pool->threads_q.deQueue();//ima posla uzimamo tred
		while (t.id == -1) {//ako trenutno nema slobodnih pokusavaj stalno dok ne bude prvi slobodan tred
			t = pool->threads_q.deQueue();
			Sleep(100);
		}
		ReleaseSemaphore(pool->semaphores[t.id], 1, NULL); //nasli slobodan neka obavi posao
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
	LeaveCriticalSection(&work_cnt_sec);
	ReleaseSemaphore(tasksem,1,NULL);//obavestimo semafor da ima razloga za proveru za poslom
	Sleep(50);
}

void DestroyPool() {
	CloseHandle(wchandle);
	for (int i = 0; i < tcount; i++) {
		CloseHandle(pool->semaphores[i]);
		CloseHandle(h[i]);
	}
	delete(t);
	delete(h);
	delete(pool);
}
