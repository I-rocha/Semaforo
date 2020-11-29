#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

int (*_sem_wait)(sem_t *) = NULL;
int (*_sem_post)(sem_t*) = NULL;

typedef struct item* nodo;
typedef struct item2* vertice;

struct item{
	int type;		//0 = recurso ou 1 = Processo
	pthread_t tid;
	sem_t* recurso;
	vertice acesso;
	nodo ant, prox;
	int arestas;	//numero de arestas CHEGANDO no nodo
};

struct item2{
	nodo self;
	vertice prox;
};

int graph_tam = 0;
int i = 0;

nodo* graph;
nodo* head;

sem_t* mutex;

int alocaRecurso(nodo* head, nodo* graph, sem_t* sem);
nodo alocaNodo(nodo* head, nodo* graph, sem_t* sem, int type);

int desalocaRecurso(nodo* head, nodo* graph, sem_t* sem);
int desalocaNodo(nodo* head, nodo* graph, nodo bin);

nodo procuraNodo(nodo* head, sem_t* sem, int type);
int requisitaRecurso(nodo* head, nodo* graph, sem_t* sem);
int checkDeadlock(sem_t* sem, nodo anonimo);



// Retorna nodo do processo ja alocado
nodo procuraNodo(nodo* head, sem_t* sem, int type){
	pthread_t id = pthread_self();
	nodo percorre;
	
//	percorre = (nodo)malloc(sizeof(struct item));
	percorre = *head;
	while(percorre != NULL){
		switch(type){
			case 0: // Recurso
				if(percorre->recurso == sem){
					printf("Recurso existente\n");
					return percorre;
				}
			break;
			
			case 1: //processo
				if(percorre->tid == id){
					printf("Processo existente\n");
					return percorre;
				}
			break;

			default:
				perror("procuraNodo error: type nodo uknown\n");
				exit(0);
			break;
		}
		percorre = percorre->prox;
	}
//	printf("procuraNodo: Retorno nulo\n");
	return NULL;
}

//Deadlock será verificado apenas sobre o nodo do ultimo processo requisitando um recurso R
//Como recurso é uma área crítica, somente um processo por vez pode ter um recurso alocado
//Por outro lado um recurso pode ser requisitado por vários processos

int checkDeadlock(sem_t* sem, nodo anonimo){
	//Busca gulosa
	
	printf("\n\n");
	if(anonimo == NULL){
		printf("Sem Deadlock, o recurso foi alocado com sucesso\n");
		fflush(stdout);
		return 0;
	}
	else{
		if(anonimo->type == 0){
			if(anonimo->recurso == sem){
				//Deadlock
				printf("ALERTA: Deadlock encontrado\n");
				exit(0);
			}
		}

		if (anonimo->acesso == NULL){
			printf("Sem deadlock, o recurso foi alocado com sucesso\n");
			return 0;
		}
		else{
			checkDeadlock(sem, anonimo->acesso->self);
		}
	}
	return 0;
}



int requisitaRecurso(nodo* head, nodo* graph, sem_t* sem){
	nodo rec, proc;
	vertice auxr;

	rec = (nodo)malloc(sizeof(struct item));
	proc = (nodo)malloc(sizeof(struct item));

	//Procurando pelo recurso e processo no grafo
	rec = procuraNodo(head, sem, 0); //recurso
	
	if(rec == NULL){
		perror("requisitaRecurso: Recurso NULL but recurso must exist");
		exit(0);
	}
	
	proc =	procuraNodo(head, sem, 1); //processo
	
	if(proc == NULL){	//Processo não existe
		proc = alocaNodo(head, graph, sem, 1);
	}

	//requisitando
	auxr = (vertice)malloc(sizeof(struct item2));
	auxr->self = rec;
	auxr->prox = NULL;
	proc->acesso = auxr;
	proc->acesso->self->arestas++;	//rec->arestas++

	//Deadlock só ocorre após requisição estabelecida
	checkDeadlock(sem, rec->acesso->self);
	printf("Deadlock check finish\n");
	return 1;
}

nodo alocaNodo(nodo* head, nodo* graph, sem_t* sem, int type){

	nodo bin;
	bin = (nodo)malloc(sizeof(struct item));	
	bin->recurso = (sem_t*)malloc(sizeof(sem_t)); 
	bin->acesso = (vertice)malloc(sizeof(struct item2));
	bin->arestas = 0;

	switch(type){	
	
		case 0: //recurso
			bin->tid = -1;
			bin->recurso = sem;
		break;
		case 1:	//Processo
			bin->tid = pthread_self();
			bin->recurso = NULL;
		break;
		default:
			perror("alocaNodo error: type nodo uknown\n");
			exit(0);
		break;
	}
	
	bin->type = type;
	bin->acesso = NULL;
	bin->prox = NULL;
	bin->ant = NULL;

	if(graph_tam == 0){
		*graph = bin;
		*head = bin;
	}
	else{
		(*graph)->prox = bin;
		(*graph)->prox->ant = *graph;
		*graph = (*graph)->prox;
	}
	
	graph_tam++;
	
	return bin;
}

int alocaRecurso(nodo* head, nodo* graph, sem_t* sem){
	nodo auxp, auxr;
	vertice auxv;


	auxp = (nodo)malloc(sizeof(struct item));	
	auxr = (nodo)malloc(sizeof(struct item));	
	
	auxp->recurso = (sem_t*)malloc(sizeof(sem_t));
	auxr->recurso = (sem_t*)malloc(sizeof(sem_t));

	auxp->acesso = (vertice)malloc(sizeof(struct item2));
	auxr->acesso = (vertice)malloc(sizeof(struct item2));
	
	auxv = (vertice)malloc(sizeof(struct item2));

	auxp = procuraNodo(head, sem, 1);//processo
	auxr = procuraNodo(head, sem, 0);//recurso
	
	if(auxp == NULL){ //Processo não existe
		auxp = (nodo)malloc(sizeof(struct item));	
		auxp = alocaNodo(head, graph, sem, 1);
	}

	if(auxr == NULL){ //Recurso não existe
		auxr = (nodo)malloc(sizeof(struct item));	
		auxr = alocaNodo(head, graph, sem, 0);
	}


	//Promovendo recurso de requisitado(caso houver) para alocado
	if(auxp->acesso != NULL) auxp->acesso->self->arestas--;
	auxp->acesso = NULL;

	auxv->self = auxp;
	auxv->prox = NULL;
	auxr->acesso = auxv;
	auxr->acesso->self->arestas++; //auxp->arestas++

	return 1;
}

int sem_wait(sem_t *sem){
	int r;
	int* value;
	
	value = (int*)malloc(sizeof(int));
	
	if (!_sem_wait){ 
		_sem_wait = dlsym(RTLD_NEXT, "sem_wait");
	}
	if(!_sem_post){
		_sem_post = dlsym(RTLD_NEXT, "sem_post");
	}

	if(i == 0){
		printf("Inicialização unica\n\n");
		i=1;
		mutex = (sem_t*)malloc(sizeof(sem_t));
		sem_init(mutex,0,1);
		graph = (nodo*)malloc(sizeof(nodo));
		head = (nodo*)malloc(sizeof(nodo));
	}
	sleep(0.01);
	printf("\n\nThread: %llu\nusando mutex: %llu\n", (long long unsigned int)pthread_self(), (long long  unsigned int)mutex );
	
	_sem_wait(mutex);
	printf("\n\nZona critica sendo acessada pela Thread: %llu\ncom o semaforo: %p\n", (long long unsigned int)pthread_self(), sem);
	if(*head == NULL){
		printf("#### Inicializador1 ####\n");
		*graph = (nodo)malloc(sizeof(struct item));
		(*graph)->prox = NULL;
		(*graph)->ant = NULL;
		*head = (nodo)malloc(sizeof(struct item));
		(*head)->prox = NULL;
		(*head)->ant = NULL;
	}
		

	sem_getvalue(sem, value);
	
	if(*value == 0){ //sem == 0
		printf("Thread está bloqueada: %llu\n", (long long unsigned int)pthread_self());
		requisitaRecurso(head, graph, sem);
		printf("Thread deixando zona critica: %llu\n", (long long unsigned int)pthread_self());
	}
	_sem_post(mutex);

	r = _sem_wait(sem); 
	
	//sem == 1
	_sem_wait(mutex);
	printf("Thread está rodando: %llu\n", (long long unsigned int)pthread_self());
	alocaRecurso(head, graph, sem);
	printf("Thread deixando zona critica: %llu\n", (long long unsigned int)pthread_self());
	_sem_post(mutex);
	
	return r;
}

// nodo não possui nenhuma aresta entrando ou saindo
int desalocaNodo(nodo* head, nodo* graph, nodo bin){
	if(graph_tam == 1)
		printf("GRAFO VAZIO\n");

	if(graph_tam == 1){
		printf("Novo grafo deve ser feito\n");
		free(bin);
		*graph = NULL;
		*head = NULL;
		graph_tam = 0;
		return 1;
	}

	if(bin == *graph) *graph = (*graph)->ant;
	if(bin == *head) *head = (*head)->prox;
	if(bin->ant != NULL) bin->ant->prox = bin->prox;
	if(bin->prox != NULL) bin->prox->ant = bin->ant;

	free(bin);
	graph_tam--;
	
	return 1;
}

// Processo não vai estar requisitando nada
// Recurso não vai estar alocado por ninguém
int desalocaRecurso(nodo* head, nodo *graph, sem_t* sem){
	nodo proc, rec;
	
	rec = procuraNodo(head, sem, 0);
	proc = procuraNodo(head, sem, 1);

	if(rec == NULL) perror("function desaloca: recurso (nil) but recurso must exist\n");
	if(proc == NULL) perror("function desaloca: processo (nil) but processo must exist\n");

	// Removendo acesso de R->P
	rec->acesso = NULL;
	proc->arestas--;

	if(proc->arestas == 0){
		desalocaNodo(head, graph, proc);
	}
	if(rec->arestas == 0){
		desalocaNodo(head, graph, rec);
	}

	return 1;
}

int sem_post(sem_t *sem){
	int r;
	

	if (!_sem_post){ 
		_sem_post = dlsym(RTLD_NEXT, "sem_post");
	}
	if (!_sem_wait){ 
		_sem_wait = dlsym(RTLD_NEXT, "sem_wait");
	}

	// mutex de proteção
	_sem_wait(mutex);

	desalocaRecurso(head, graph, sem);
	r = _sem_post(sem); //sem = 1
	
	// fim do mutex
	_sem_post(mutex);

	return r;
}
