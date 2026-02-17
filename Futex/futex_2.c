#include <stdio.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>

atomic_int futex_lock = 0;
atomic_int writer_lock = 0;

int reader_count = 0;
int counter = 0;



int futex_wait(atomic_int *addr, int expected)
{
    return syscall(SYS_futex, (int*)addr, FUTEX_WAIT, expected, NULL, NULL, 0);
}


int futex_wake(atomic_int *addr, int num)
{
    return syscall(SYS_futex, (int*)addr, FUTEX_WAKE, num, NULL, NULL, 0);
}



void lock(atomic_int *f)
{
    while(atomic_exchange(f,1)==1)
        futex_wait(f,1);
}

void unlock(atomic_int *f)
{
    atomic_store(f,0);
    futex_wake(f,1);
}



void *reader(void *arg)
{
    int id = *(int*)arg;

    while(1)
    {
        lock(&futex_lock);

        reader_count++;

        if(reader_count == 1)
            lock(&writer_lock);

        unlock(&futex_lock);


        printf("Reader %d reads counter = %d\n", id, counter);


        lock(&futex_lock);

        reader_count--;

        if(reader_count == 0)
            unlock(&writer_lock);

        unlock(&futex_lock);

        sleep(1);
    }
}



void *writer(void *arg)
{
    int id = *(int*)arg;

    while(1)
    {
        lock(&writer_lock);

        counter++;

        printf("Writer %d updates counter = %d\n", id, counter);

        unlock(&writer_lock);

        sleep(2);
    }
}


int main()
{
    pthread_t r[3], w[2];

    int rid[3] = {1,2,3};
    int wid[2] = {1,2};


    for(int i=0;i<3;i++)
        pthread_create(&r[i],NULL,reader,&rid[i]);

    for(int i=0;i<2;i++)
        pthread_create(&w[i],NULL,writer,&wid[i]);


    while(1)
        sleep(10);

    return 0;
}
