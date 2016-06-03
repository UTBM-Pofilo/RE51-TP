// tags corresponding to PING and PONG messages
#define PING 301
#define PONG 302

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <unistd.h>

int rank, numprocs;

int participated = 0;

// The basic object used by MPI to determine which processes are involved in a communication
MPI_Comm graph_comm;

// Structure used by the message 
MPI_Status status;

// number of nodes
int graph_node_count = 10;
// index definition
int *graph_index;
// edges definition
int *graph_edges;
// allows processes reordered for efficiency
int graph_reorder = 1;

// create the ring with the number of processes asked
int create_ring(numprocs) {
	if(numprocs < graph_node_count){
		return 1;
	}

	int i;
	graph_node_count = numprocs;

	graph_index = malloc(numprocs * sizeof(int));
	graph_edges = malloc(numprocs * sizeof(int));

	for(i = 0; i < numprocs; i++){
		graph_index[i] = i + 1;
		graph_edges[i] = (i + 1)%numprocs;
	}

	// Makes a new communicator to which topology information has been attached.
	MPI_Graph_create(MPI_COMM_WORLD, graph_node_count, graph_index, graph_edges, graph_reorder, &graph_comm); 

	return 0;
}


int loose_token(){
	int result = 0;

	if((rank == 4 && participated == 2)||(rank == 2 && participated == 4)){
		result = 1;
	}
	
	return result;
}


int main(int argc, char *argv[]) {
	// Init the random function
	srand(time(NULL));

	// Initialize the MPI execution environment
	MPI_Init(&argc, &argv);
	// Determines the size of the group associated with a communicator
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	// Determines the rank of the calling process in the communicator
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(create_ring(numprocs) != 0) {
		printf("This program needs at least \"%d\" nodes.\n", graph_node_count);
		return 1;
	}

	int running = 1;
	int i, neighbor_count, *neighbors;

	// counters
	int nbPing = 1;
	int nbPong = -1;

	int tmp = 0;
	int last_m = 0;

	// Determines the rank of the calling process in the communicator
	MPI_Comm_rank(graph_comm, &rank);       

	// Returns the number of neighbors of a node associated with a graph topology
	MPI_Graph_neighbors_count(graph_comm, rank, &neighbor_count);

	neighbors = (int*) malloc (neighbor_count * sizeof(int));
	// Returns the neighbors of a node associated with a graph topology
	MPI_Graph_neighbors(graph_comm, rank, neighbor_count, neighbors);

	if(rank == 0){
		// Performs a blocking send
		MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], PING, graph_comm);
		printf("master sends PING\n");
		
		// Performs a blocking send
		MPI_Send(&nbPong, 1, MPI_INT, neighbors[0], PONG, graph_comm);
		printf("master sends PONG\n");
	}

	while(running){
		// Blocking receive for a message
		MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, graph_comm, &status);

		printf("node \"%d\" received \"%s\" from node \"%d\"\n", rank, (status.MPI_TAG==PING)?"PING":(status.MPI_TAG==PONG)?"PONG":"UNKNOWN", status.MPI_SOURCE);

		// simulate a lost
		participated++;
		if(loose_token()){
			continue;
		}
		
		if(status.MPI_TAG == PING){
			nbPing = tmp;

			if(last_m == nbPing){
				nbPing++;
				printf("PONG lost --> regenerate it\n");
				nbPong =- nbPing;
				// Performs a blocking send
				MPI_Send(&nbPong, 1, MPI_INT, neighbors[0], PONG, graph_comm);
			} else {
				last_m = nbPing;
			}

			// Performs a blocking send
			MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], PING, graph_comm);
		} else if(status.MPI_TAG == PONG){
			nbPong = tmp;
			
			if(last_m == nbPong){
				nbPong--;
				printf("PING lost --> regenerate it\n");
				nbPing =- nbPong;
				// Performs a blocking send
				MPI_Send(&nbPing, 1, MPI_INT, neighbors[0], PING, graph_comm);
			}else{
				last_m = nbPong;
			}
    
			// Performs a blocking send
			MPI_Send(&nbPong,1, MPI_INT, neighbors[0], PONG, graph_comm);
		} else {
			printf("UNKNOWN tag");	
		}
		
		sleep(1);
	}

	// Terminates MPI execution environment
	MPI_Finalize();
}
