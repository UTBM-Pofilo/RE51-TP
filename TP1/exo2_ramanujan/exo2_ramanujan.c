#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mpi.h"
 
int *queue = NULL;
int front=0, rear=0;
 
void push(int token)
{
    queue[front]=token;
    front=front+1;
}
 
int pop()
{
    int t;
    if(front==rear)
    {
        printf("\nQueue empty\n");
        return 0;
    }
    rear=rear+1;
    t=queue[rear-1];
    return t;
}
 
long double factorial(int n){
    if (n<=0){
        return 1;
    } else {
        long  res = n ;
        while (--n>1){
            res *=n;
        }
        return res;
    }
}
 
long double calcul_n(int number){
    long double u, v, w, x;
    u = factorial(4*number);
    v = (1103+26390*number);
    w = pow(factorial(number),4);
    x = pow(396,4*number);
 
    //printf(" number=%d\n", number);
    //printf("u=%Lf, v=%Lf, w=%Lf, x=%Lf\n",u,v,w,x);
    return (factorial(4*number)*(1103+26390*number)/
        (pow(factorial(number),4)*pow(396,4*number)));
}
 
int main(int argc, char** argv){
    if(argc<2){
        printf("usage: %s <nb_iter>\n", argv[0]);
        return 1;
    }
 
    int niter =atoi(argv[1]);
    queue = malloc(niter *sizeof(int));
 
    long double count =0, result_n = 0;
    int mycount = 0;
    int myid, numprocs, proc;
    MPI_Status status;
     
    int master = 0;
    int tag_work=123;
    int tag_result=234;
    int tag_end=2;
 
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
 
//  printf("hello ! I am the process %d\n", myid);
    if(myid==0){
    //It might be the man process so, it had to queue all N receive in parameter.
        //printf("main process started \n");
        int i;
 
        // queue all elements we gonna compute
        for(i=0; i<niter; ++i){
            push(i);
        }
 
        //and finally it receive the calculation of each process 
        int value;
 
        for(proc=1; proc<numprocs; proc++){
             
            value = pop();
            MPI_Send(&value, 1, MPI_INT, proc, tag_work, MPI_COMM_WORLD);
            printf("send first work number:%d to process %d\n", value, proc);
        }
 
        while(value=pop()){
             
            MPI_Recv(&result_n, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
 
            MPI_Send(&value, 1, MPI_INT, status.MPI_SOURCE, tag_work,MPI_COMM_WORLD);
            printf("Master: received result_n = %Lf and send value : %d to child %d\n", result_n, value, status.MPI_SOURCE);
            count+=result_n;
        }
 
        printf("end while - count = %Lf\n", count);
         
        //There's no more work to be done, we receive the last calculation of each process
        for(proc=1; proc<numprocs; proc++){
            MPI_Recv(&result_n, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            count+=result_n;
        }
 
        for(proc=1; proc<numprocs; proc++){
            printf("Master: I send something\n");
            MPI_Send(0, 0, MPI_INT, proc, tag_end, MPI_COMM_WORLD);
        }
         
        long double pi = (9801/(2*sqrt(2)*count));
        printf("pi = %Lf\n", pi);
 
    } else {
    // it might be child process, so they receive number from main process and 
    // resend the result until 
 
        while(1){
            MPI_Recv(&mycount, 1, MPI_INT, master,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
             
            if(status.MPI_TAG==tag_end){
                printf(" process %d : I stop\n", myid);
                break;
 
            }else{
                printf("Child %d: receive value:%d\n", myid, mycount);
                 
                result_n = calcul_n((int)mycount);
                MPI_Send(&result_n, 1, MPI_LONG_DOUBLE, master, tag_result, MPI_COMM_WORLD);
                 
                printf("Child %d: send my computation %Lf\n", myid, result_n);
            }
        }
    }
 
    MPI_Finalize();
    free(queue);
    return 0;
} 
