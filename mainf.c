#include<stdio.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<unistd.h>

sem_t* sem_a;
sem_t* sem_b;



void* fA(){	
	printf("Thread A: %llu\n\n", (long long unsigned int)pthread_self());
//	printf("before waiting fa\n");
	sem_wait(sem_a);	
	printf("after A alocar a\n");
	sem_wait(sem_b);
	printf("after A alocar b\n");
//	sleep(2);
//	printf("sleeping fa\n");
//	sleep(5);
//	printf("A requisitando B");
//	sem_wait(sem_b);
//	printf("After A alocar b\n");
//	sleep(4);
	sem_post(sem_a);
	printf("After a free a\n");
	sem_post(sem_b);
	printf("After a free b\n");
	printf("fa out\n");
	pthread_exit(0);
}

void* fB(){
	printf("Thread B: %llu\n\n", (long long unsigned int)pthread_self());
//	printf("before waiting fb\n");
//	sleep(2);
	sem_wait(sem_b);
	printf("after B alocar b\n");
//	printf("sleeping fb\n");
//sleep(5);
	sem_wait(sem_a);
	printf("After B alocar a\n");	
	sem_post(sem_b);
	printf("After b free b\n");
	sem_post(sem_a);
	printf("After b free a\n");
	printf("fb out\n");
	pthread_exit(0);
}


int main(){
	pthread_t A,B;
	
//  sem_t* sem_a;
//	sem_t* sem_b;
	
	sem_a = (sem_t*)malloc(sizeof(sem_t));
	sem_b = (sem_t*)malloc(sizeof(sem_t));

//	printf("sem_a: %p\n", sem_a);
//	printf("sem_b: %p\n\n", sem_b);

	sem_init(sem_a, 0, 1);
	sem_init(sem_b, 0, 1);

//	pthread_join(A, NULL);
	pthread_create(&A, NULL, fA, NULL);
	pthread_create(&B, NULL, fB, NULL);

	pthread_join(A, NULL);
	pthread_join(B, NULL);

	return 0;

}
