#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "thread.h"
struct task {
    // pthread_t tid;
    int tid;
    struct  {
        int in;//pipefd[0] read
        int out;//pipefd[1] write
    }pipe;
};
struct  semaphore {
    bool lock;
    int count;
    struct task thrs[1024];//queue size
    int t_idx;//tail
    void (*p) (struct semaphore *s,int tid);
    void (*v) (struct semaphore *s);

};
typedef struct semaphore *sem;
bool TestAndSet(bool *target);
void acquire(sem s,int tid);
void release(sem s);
void Enqueue(sem s,int tid);
void Dequeue(sem s);
void Destroy(sem s);
void *active(void *aegp);
static int REASONABLE_THREAD_MAX=500;



struct  semaphore global_sem= {
    .lock=false,
    .count=0,
    .p=acquire,
    .v=release,
};



int countNo;
int main(int argc, char const *argv[])
{
    int nhijos;
   threads** threadS;
   pthread_t *tid;
  
    countNo=0;
    if (argc > 1) {
        nhijos = atoi(argv[1]);
        if ((nhijos <= 0) || (nhijos > REASONABLE_THREAD_MAX)) {
            printf("invalid argument for thread count\n");
            exit(EXIT_FAILURE);
        }
        threadS=(threads**)malloc(sizeof(threads*)*nhijos);
        tid=(pthread_t*)malloc(sizeof(pthread_t)*nhijos);
        
        for (int i = 0; i < nhijos; ++i) {
            threadS[i] = (threads *)malloc(sizeof (threads));
            if(threadS[i]!=NULL) {
                threadS[i] ->no=0;
                threadS[i] ->tid=i;
            }else{
                printf("threadS=NULL\n");
                exit(1);
            }
        }
        for (int i = 0; i < nhijos; i++) {
            pthread_create(&tid[i], NULL, &active, threadS[i] );
        }
         for (int i = 0; i < nhijos; i++) {
            pthread_join( tid[i], NULL );
        }
    }
}
void *active(void *aegp)
{
    sem s=&global_sem;//pointer to variable
    threads *arg=(threads *)aegp;
    int no=arg->no;
    int tid=arg->tid;
    s->p(s,tid); //acquire
    printf("%d\n",s->count );
    //critical section
    countNo++;
    no=countNo;
    // printf("in \n");
    s->v(s); //release
    pthread_exit(0);
}
//atomic: only one cpu(thread) can do TestAndSet
bool TestAndSet(bool *target)
{
    //mutex protect
    bool rv = *target;
    *target =true;
    return rv;
}
void acquire(sem s,int tid)
{
    while(TestAndSet(&s->lock))
        ;
    if (!s->count>2) { //if only allow 2 threads to get source
        //put thread in the list
        Enqueue(s,tid);
        s->lock=false;
    } else {
        s->count--;
        s->lock=false;
    }
}
void release(sem s)
{
    while(TestAndSet(&s->lock))
        ;
    if(s->t_idx > 0) {   //if wait queue not empty
        Dequeue(s); //dequeue thread from wait queue
    } else {
        s->count++;
    }
    s->lock=false;
}
void Enqueue(sem s,int tid)
{
    char signal_t='a';

    s->thrs[s->t_idx].tid=tid;
    pipe((int *)&(s->thrs[s->t_idx].pipe));

    s->t_idx++;

    //block
    read(s->thrs[s->t_idx].pipe.in,&signal_t,1);
}
void Destroy(sem s)
{
    if (s->thrs[s->t_idx].pipe.in) {
        close(s->thrs[s->t_idx].pipe.in);
        s->thrs[s->t_idx].pipe.in = 0;
    }
    if (s->thrs[s->t_idx].pipe.out) {
        close(s->thrs[s->t_idx].pipe.out);
        s->thrs[s->t_idx].pipe.out = 0;
    }
}
void Dequeue(sem s)
{
    char signal_t='a';

    write(s->thrs[s->t_idx].pipe.out,&signal_t,1);
    Destroy(s);
    s->t_idx--;
}

//queue async
//lock set to false and then other thread can go through from test and set
/*read()  attempts  to  read up to count bytes from file descriptor fd into the
     buffer starting at buf.

     On files that support seeking, the read operation commences  at  the  current
     file  offset, and the file offset is incremented by the number of bytes read.
     If the current file offset is at or past the end of file, no bytes are  read,
     and read() returns zero.*/
/* pipe()  creates  a pipe, a unidirectional data channel that can be used
       for interprocess communication.  The array pipefd is used to return two
       file  descriptors  referring to the ends of the pipe.  pipefd[0] refers
       to the read end of the pipe.  pipefd[1] refers to the write end of  the
       pipe.   Data  written  to  the write end of the pipe is buffered by the
       kernel until it is read from the read end of  the  pipe. */
