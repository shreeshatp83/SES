#include<stdio.h>
#include<sys/syscall.h>
#include<linux/futex.h>
#include<stdatomic.h>
#include<ctype.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>

#define max_count 10
int counter=0;


int futex_wait(int *addr, int expected)
{
	return syscall(SYS_futex, addr, FUTEX_WAIT,expected,NULL, NULL, 0);
}


int  futex_wake(int *addr, int num){
	return syscall(SYS_futex,addr,FUTEX_WAKE,num , NULL, NULL, 0);
}

atomic_int futex = 0;

void lock(){

	while(atomic_exchange(&futex,1)==1){
//	while(futex == 1){
		futex_wait(&futex,1);
	}
}

void unlock(){
	atomic_store(&futex,0);
//	futex = 0;
	futex_wake(&futex,1);
}

void *upcounter(void *arg){
int i;
        lock(&futex);
        for(i=0;i<max_count;i++){
                counter++;
                printf("upcounter: %d\n",counter);
                }
        unlock(&futex);
        pthread_exit(NULL);
}



void *downcounter(void *arg){

int i;
	lock(&futex);
        for(i=0;i<max_count;i++){
                counter--;
                printf("downcounter: %d\n",counter);
        }
        unlock(&futex);
        pthread_exit(NULL);
}


int main(){

pthread_t rthread, cthread;
printf("Counter\n");
pthread_create(&rthread,NULL,upcounter,NULL);
pthread_create(&cthread,NULL,downcounter,NULL);
pthread_join(rthread,NULL);
pthread_join(cthread,NULL);
return 0;
}

