#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <unistd.h>

int _rank, _numprocs, _ret;

int participated = 0;

// tags corresponding to PING and PONG messages
int ping = 301;
int pong = 302;


/* a communicator where the topology is defined */
MPI_Comm graph_comm;

MPI_Status status;

/* number of nodes */
int graph_node_count = 10;

/* index definition */
int *graph_index;

/* edges definition */
int *graph_edges;

int graph_reorder = 1; /* allows processes reordered for efficiency */


/*** a function to create a ring depending on the number of processes ***/
int create_ring(_numprocs) {
	if(_numprocs<10){
		return 1;
	}

	int i;
	graph_node_count = _numprocs;

	graph_index = malloc(_numprocs * sizeof(int));
	graph_edges = malloc(_numprocs * sizeof(int));

	for(i=0; i<_numprocs; i++){
		graph_index[i]=i+1;
		graph_edges[i]=(i+1)%_numprocs;
	}

	MPI_Graph_create(MPI_COMM_WORLD, graph_node_count, graph_index, graph_edges, graph_reorder, &graph_comm); 

	return 0;
}


int lost_token(){
	int result=0;

	if((_rank==4 && participated ==2 )||(_rank==2 && participated==4)){
		result=1;
	}
	return result;
}


int main(int argc, char *argv[]) {
	srand(time(NULL));

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &_numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&_rank);

	if(create_ring(_numprocs) != 0) {
		printf("This program needs at least 10 nodes.\n");
		return 1;
	}

	int running = 1;

	//i iterator, participated counter, neighbors - tabs of my neighbors
	int i, neighbor_count, *neighbors;

	// counters
	int nbPing=1;
	int nbPong=-1;

	int tmp=0;
	int last_m=0;

	/* define the ring topology */
	MPI_Comm_rank(graph_comm, &_rank);       

	/* each process now its neighbors in one way */
	MPI_Graph_neighbors_count(graph_comm, _rank, &neighbor_count);

	neighbors = (int*) malloc (neighbor_count * sizeof(int));
	MPI_Graph_neighbors(graph_comm, _rank, neighbor_count, neighbors);

	if(_rank == 0){
		MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], ping, graph_comm);
		printf("[%f] master - send Ping\n", MPI_Wtime());
		
		MPI_Send(&nbPong, 1, MPI_INT, neighbors[0], pong, graph_comm);
		printf("[%f] master - send Pong\n", MPI_Wtime());
	}

	while(running){
		MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, graph_comm, &status);

		printf("[%f] node %d : receive %s from node %d\n", MPI_Wtime(), _rank, (status.MPI_TAG==ping)?"ping":(status.MPI_TAG==pong)?"pong":"unknow", status.MPI_SOURCE);

		//simulate a lost
		participated++;
		if(lost_token()){
			continue;
		}
		if(status.MPI_TAG==ping){
			nbPing=tmp;
			// printf("here -----------------1\n");

			if(last_m==nbPing){
				nbPing++;
				printf("[%f] PONG lost ! - regenerate it !\n", MPI_Wtime());
				nbPong=-nbPing;
				MPI_Send(&nbPong, 1, MPI_INT, neighbors[0], pong, graph_comm);
			}else{
				last_m = nbPing;
			}

			MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], ping, graph_comm);
       
		} else if(status.MPI_TAG==pong){
            
			nbPong=tmp;
			//printf("here -------------------2\n");
			if(last_m==nbPong){
				nbPong--;
				printf("[%f] PING lost ! - regenerate it !\n", MPI_Wtime());
				nbPing=-nbPong;
				MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], ping, graph_comm);
			}else{
				last_m=nbPong;
			}
    
			MPI_Send(&nbPong,1, MPI_INT, neighbors[0], pong, graph_comm);
    
		} else {
			printf("unkown tag");	
		}
		sleep(1);
	
	}

	MPI_Finalize();
}
