#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mpi.h"

int *list = NULL;
int head=0, tail=0;

/* functions for the list*/
void push(int token){
	list[head] = token;
	head = head + 1;
}

int pop(){
	int t;
	if(head == tail) {
		printf("\nlist empty");
		return 0;
	}
	tail = tail + 1;
	t = list[tail - 1];

	return t;
}

/* function for the calculus */
long double factorial(int n){
	if (n <= 0){
		return 1;
	} else {
		long  res = n ;
		while (--n > 1){
			res *= n;
		}
	}

	return res;
}

// write the calculus of the ramanujan formula
long double calcul_n(int number){
	long double u, v, w, x;
	u = factorial(4*number);
	v = (1103+26390*number);
	w = pow(factorial(number),4);
	x = pow(396,4*number);
	return (factorial(4*number)*(1103+26390*number)/(pow(factorial(number),4)*pow(396,4*number)));
}


int main(int argc, char** argv){
	if(argc < 2){
		printf("usage: %s <nb_iter>\n", argv[0]);
		return 1;
	}

	int niter = atoi(argv[1]);

	list = malloc(niter *sizeof(int));
	long double count =0, result_n = 0;
	int count2 = 0;
	int id, numprocs, proc;

	MPI_Status status;

	int master = 0;
	int tag_work = 123;
	int tag_result = 234;
	int tag_end = 2;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	if(id == 0){
		int i;
		// list all elements we need to compute
		for(i = 0; i < niter; ++i){
			push(i);
		}

		// receive the result of each process
		int value;
		for(proc = 1; proc < numprocs; proc++){
			value = pop();
			MPI_Send(&value, 1, MPI_INT, proc, tag_work, MPI_COMM_WORLD);
			printf("send task \"%d\" to process \"%d\"\n", value, proc);
		}

		while(value = pop()){
			MPI_Recv(&result_n, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Send(&value, 1, MPI_INT, status.MPI_SOURCE, tag_work,MPI_COMM_WORLD);
			printf("master received result_n \"%Lf\" and send value \"%d\" to child \"%d\"\n", result_n, value, status.MPI_SOURCE);
			count += result_n;
		}

		// receive last result of each process
		for(proc = 1; proc < numprocs; proc++){
			MPI_Recv(&result_n, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			count += result_n;
		}

		for(proc = 1; proc < numprocs; proc++){
			MPI_Send(0, 0, MPI_INT, proc, tag_end, MPI_COMM_WORLD);
		}

		long double pi = (9801/(2*sqrt(2)*count));
		printf("--> pi = \"%Lf\"\n", pi);
	} else {
		// process task
		while(1){
			MPI_Recv(&count2, 1, MPI_INT, master,MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			if(status.MPI_TAG == tag_end){
				printf(" process \"%d\" finished\n", id);
				break;
			} else {
				printf("child \"%d\" received \"%d\"\n", id, count2);
				result_n = calcul_n((int)count2);
				MPI_Send(&result_n, 1, MPI_LONG_DOUBLE, master, tag_result, MPI_COMM_WORLD);
				printf("child \"%d\" sends \"%Lf\"\n", id, result_n);
			}
		}
	}

	MPI_Finalize();

	// memory leak
	free(list);

	return 0;
} 
