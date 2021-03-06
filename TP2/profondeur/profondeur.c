#define FORWARD 301
#define END_RETURN 200
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>


int rank, numprocs;

// The basic object used by MPI to determine which processes are involved in a communication
MPI_Comm graph_comm;
// Structure used by the message 
MPI_Status status;

// number of nodes
int graph_node_count = 6;
// index definition
int graph_index[6] = {2, 5, 9, 10, 12, 14}; 
// edges definition
int graph_edges[16] = {1, 2, 0, 2, 3, 0, 1, 4, 5, 1, 2, 5, 2, 4};
// allows processes reordered for efficiency
int graph_reorder = 1; 

// create the graph which corresponds to the topology above
int create_graph() {
	if (numprocs < graph_node_count) {
		return 1;
	}
	
	// Makes a new communicator to which topology information has been attached.
	MPI_Graph_create(MPI_COMM_WORLD, graph_node_count, graph_index, graph_edges, graph_reorder, &graph_comm); 

	return 0;
}

// search element in the tab of neighbors to delete it and return the counter updated by minus 1
int delete_source(int neighbors[], int counter, int element) { 
	int i, j, _bool;
	_bool = 0;
	i = 0;

	while (i < counter && _bool == 0) {
		if(neighbors[i] == element) {
			_bool = 1;
		} else {
			i ++;
		}  
	}  

	for (j = i; j < counter-1; j++){
		neighbors[j]=neighbors[j+1];
	}

	return --counter;
}

int pick_recv(int neighbors[], int neighbor_count){
	return neighbors[rand()%neighbor_count];
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

	if(create_graph() != 0) {
		printf("This program needs \"%d\" nodes.\n", graph_node_count);
		return 1;
	}

	int running = 1;
	int i, rank, neighbor_count, *neighbors;
	int participated = 0;
	int father = -1;
	int next_recv;
    
	// Determines the rank of the calling process in the communicator
	MPI_Comm_rank(graph_comm, &rank); 
	// Returns the number of neighbors of a node associated with a graph topology
	MPI_Graph_neighbors_count(graph_comm, rank, &neighbor_count);

	neighbors = malloc (neighbor_count * sizeof(int));
	// Returns the neighbors of a node associated with a graph topology
	MPI_Graph_neighbors(graph_comm, rank, neighbor_count, neighbors);

	printf("%f | node \"%d\" starts, his neighbors are ", MPI_Wtime(), rank);
	
	for(i = 0; i < neighbor_count; i++){
		printf("\"%d\" ", neighbors[i]);
	}
	
	printf("\n");

	if(rank == 0){
		participated = 1;
		next_recv=pick_recv(neighbors, neighbor_count);
		printf("%f | Master \"%d\" sends \"FORWARD\" to node \"%d\"\n", MPI_Wtime(), rank, neighbors[i]);
		// Performs a blocking send
		MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
	}

	while (running){
		// Blocking receive for a message
		MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, graph_comm, &status);

		// seeing all received messages 
		printf("%f | node \"%d\" received \"%s\" from node \"%d\"\n", MPI_Wtime(), rank, (status.MPI_TAG==FORWARD?"FORWARD":"RETURN"), status.MPI_SOURCE);
		
		switch((int)status.MPI_TAG){
			case FORWARD:
				neighbor_count = delete_source(neighbors, neighbor_count, status.MPI_SOURCE);   
				if (participated == 0 && neighbor_count != 0){
					father = status.MPI_SOURCE;
					participated = 1;
 
					printf("%f | node \"%d\", my neighbors ", MPI_Wtime(), rank);
					
					for(i = 0; i < neighbor_count; i++){
						printf("\"%d\" ", neighbors[i]);
					}
					printf("\n");

					next_recv = pick_recv(neighbors, neighbor_count);
					printf("%f | node \"%d\" sends \"FORWARD\" to node \"%d\"\n", MPI_Wtime(), rank, next_recv);
					// Performs a blocking send
					MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
				} else {
					printf("%f | node \"%d\" sends \"RETURN\" to node \"%d\"\n", MPI_Wtime(), rank, status.MPI_SOURCE);
					// Performs a blocking send
					MPI_Send(NULL, 0, MPI_INT, status.MPI_SOURCE, END_RETURN, graph_comm);
					if(participated == 0){
						running = 0;
					}
				}
				break;
                     
			case END_RETURN:
				neighbor_count = delete_source(neighbors, neighbor_count, status.MPI_SOURCE);   
				if (neighbor_count == 0){
					if(father == -1){
						printf("Master --> end of program\n");
					} else {
						printf("%f | node \"%d\" sends \"RETURN\" to node \"%d\"\n", MPI_Wtime(), rank, father);
						// Performs a blocking send
						MPI_Send(NULL, 0, MPI_INT, father, END_RETURN, graph_comm);
					}
					participated = 0;
					running = 0;
				} else {
					next_recv = pick_recv(neighbors, neighbor_count);
					printf("%f | node \"%d\" sends \"FORWARD\" to node %d\n", MPI_Wtime(), rank, next_recv);
					// Performs a blocking send
					MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
				}
				break;
		}
	}

	printf("%f | node \"%d\" --> end of task\n", MPI_Wtime(), rank);

	// Terminates MPI execution environment
	MPI_Finalize();
} 
