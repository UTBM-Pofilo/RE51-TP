#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
 
int main(int argc, char** argv){

	if(argc < 2){
		printf("usage: %s <nb_iter>\n", argv[0]);
		return 1;
	}

	int niter = atoi(argv[1]);
	int count = 0, count2;
	int id, numprocs, proc;

	MPI_Status status;
	int master = 0;
	int tag=123;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	srand(time(NULL));
	int i;

	// each process will do the process 
	for(i = 0; i < niter; ++i){
		double x = (double)rand()/RAND_MAX;
		double y = (double)rand()/RAND_MAX;
		double z = x*x+y*y;
		if (z<=1) ++count2; /*dans le cercle*/
	}

	if(id == 0){
		count = count2;
		// the main process get the result of all processes
		for(proc = 1; proc < numprocs; proc++){
			MPI_Recv(&count2, 1, MPI_REAL, proc, tag, MPI_COMM_WORLD, &status);
			count += count2;
		}
		// the main process calculate the value of pi
		double pi = (double)count/(niter*numprocs)*4;
		printf("nb essais= %d, estimation de Pi %g \n", niter*numprocs, pi);
	} else {
		printf("slave %d sends %d to master\n", id, count2);
		MPI_Send(&count2, 1, MPI_REAL, master, tag, MPI_COMM_WORLD);
	}

	MPI_Finalize();

	return 0;
} 
 
