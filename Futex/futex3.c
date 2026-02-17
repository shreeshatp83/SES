#include <err.h>
#include <errno.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#define N 5
#define loops 2

static void fwait(uint32_t *futexp);
static void fpost(uint32_t *futexp);
void *writer(void *arg);
void *reader(void *arg);

static uint32_t *writer_futex, *reader_futex;
int readers = 0;

int main(int argc, char *argv[]) {
    pthread_t writer_thread;
    pthread_t reader_thread[N];
    int reader_id[N];

    uint32_t r_futex = 1;
    uint32_t w_futex = 1;

    reader_futex = &r_futex;
    writer_futex = &w_futex;

    setbuf(stdout, NULL);

    for(int i = 0; i < N; i++) {
        reader_id[i] = i;
        pthread_create(&reader_thread[i], NULL, reader, &reader_id[i]);
    }

    pthread_create(&writer_thread, NULL, writer, NULL);

    for(int i = 0; i < N; i++) {
        pthread_join(reader_thread[i], NULL);
    }
    pthread_join(writer_thread, NULL);

    printf("Done\n");

    return 0;
}

void *writer(void *arg) {
    for(int i = 0; i < loops; i++) {
        fwait(writer_futex);

        printf("[Writer] started writing\n");
        usleep(10000 * (10 + (rand() % 40)));

        printf("[Writer] finished writing\n");
        fpost(writer_futex);
        usleep(10000 * (rand() % 100));
    }

    return NULL;
}

void *reader(void *arg) {
    int *id = (int *) arg;
    for(int i = 0; i < loops; i++) {
        fwait(reader_futex);
        __atomic_fetch_add(&readers, 1, __ATOMIC_SEQ_CST);
        if(readers == 1) {
            fwait(writer_futex);
        }
        fpost(reader_futex);

        printf("[Reader %d] started reading\n", *id);
        usleep(10000 * (10 + (rand() % 40)));

        fwait(reader_futex);
        __atomic_fetch_sub(&readers, 1, __ATOMIC_SEQ_CST);
        if(readers == 0) {
            fpost(writer_futex);
        }
        printf("[Reader %d] finished reading\n", *id);
        fpost(reader_futex);
        usleep(10000 * (rand() % 100));
    }

    return NULL;
}

static int futex(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static void fwait(uint32_t *futexp) {
    long s;
    const uint32_t one = 1;

    while(1) {
        if(atomic_compare_exchange_strong(futexp, &one, 0))
            break;

        s = futex(futexp, FUTEX_WAIT, 0, NULL, NULL, 0);
        if(s == -1 && errno != EAGAIN) {
            err(EXIT_FAILURE, "futex-FUTEX_WAIT");
        }

    }
}

static void fpost(uint32_t *futexp) {
    long s;
    const uint32_t zero = 0;

    if (atomic_compare_exchange_strong(futexp, &zero, 1)) {
        s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
        if (s == -1) {
            err(EXIT_FAILURE, "futex-FUTEX_WAKE");
        }
    }
        
}
