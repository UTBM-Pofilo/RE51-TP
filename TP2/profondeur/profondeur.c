#define FORWARD 301
#define END_RETURN 200
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>
 
 
int _rank, _numprocs, _ret;
 
 
MPI_Comm graph_comm;
 
MPI_Status status;
 
/* number of nodes */
int graph_node_count = 6;
 
/* index definition */
int graph_index[6] = {2, 5, 9, 10, 12, 14};
 
/* edges definition */
int graph_edges[16] = {1, 2, 0, 2, 3, 0, 1, 4, 5, 1, 2, 5, 2, 4};
 
int graph_reorder = 1; /* allows processes reordered for efficiency */
 
 
int create_graph() {
 
    if (_numprocs < graph_node_count)
        return 1;
 
    MPI_Graph_create(MPI_COMM_WORLD, graph_node_count, graph_index, graph_edges, graph_reorder, &graph_comm); 
 
    return 0;
}
 
 
int delete_source(int neighbors[], int counter, int element){
    /** it searches "element" in the neigbors tab and delete it and return the future receiver**/
 
    int i;
    int index;
    for(i=0; i<counter;i++){
        if (neighbors[i]== element){
            index = i;
            break;
        }
    }
    for (i=index; i<counter-1; i++){
        neighbors[i]=neighbors[i+1];
    }
    return --counter;
}
 
int pick_recv(int neighbors[], int neighbor_count){
 
    return neighbors[rand()%neighbor_count];
}
 
 
int main(int argc, char *argv[]) {
 
    srand(time(NULL));
 
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &_numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
 
    if(create_graph() != 0) {
        printf("This program needs 6 nodes.\n");
        return 1;
    }
     
    int running = 1;
     
    //i iterator, neighbors - tabs of my neighbors
    int i, rank, neighbor_count, *neighbors;
     
    int participated = 0;
    int father = -1;
    int next_recv;
 
     
    MPI_Comm_rank(graph_comm, &rank);       
     
    MPI_Graph_neighbors_count(graph_comm, rank, &neighbor_count);
     
    neighbors = malloc (neighbor_count * sizeof(int));
    MPI_Graph_neighbors(graph_comm, rank, neighbor_count, neighbors);
 
    printf("node %d : start ! my neighbors : ", _rank);
        for(i=0; i<neighbor_count;i++){
            printf("%d ", neighbors[i]);
        }
    printf("\n");
 
    if(_rank == 0){
        printf("******************start algorithm***********************\n");
        participated = 1;
        next_recv=pick_recv(neighbors, neighbor_count);
        printf("Master %d : send tag : forward, to node %d\n", _rank, next_recv);
        MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
    }
 
    while (running){
        MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, graph_comm, &status);
         
        // this printf is a control to see all messages received. 
        //printf("node %d receive message tag : %s from node %d\n", _rank, (status.MPI_TAG==FORWARD?"forward":"return"), status.MPI_SOURCE);
        switch((int)status.MPI_TAG){
            case FORWARD:
                neighbor_count = delete_source(neighbors, neighbor_count, status.MPI_SOURCE);   
                if (participated==0&&neighbor_count!=0){
                    father=status.MPI_SOURCE;
                    participated=1;
 
                    printf("node %d : my neighbors : ", _rank);
                    for(i=0; i<neighbor_count;i++){
                        printf("%d ", neighbors[i]);
                    }
                    printf("\n");
 
                    next_recv = pick_recv(neighbors, neighbor_count);
                    printf("node %d : send tag : forward, to node %d\n", _rank, next_recv);
                    MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
                } else {
                    printf("node %d : send tag : return, to node %d\n", _rank, status.MPI_SOURCE);
                    MPI_Send(NULL, 0, MPI_INT, status.MPI_SOURCE, END_RETURN, graph_comm);
                    if(participated==0){
                        running=0;
                    }
                }
                break;
                                         
            case END_RETURN:
                neighbor_count = delete_source(neighbors, neighbor_count, status.MPI_SOURCE);   
                if (neighbor_count==0){
                    if(father==-1){
                        printf("Master : End of programm !\n");
                    } else {
                        printf("node %d : send tag : return, to node %d\n", _rank, father);
                        MPI_Send(NULL, 0, MPI_INT, father, END_RETURN, graph_comm);
                    }
                    participated=0;
                    running=0;
                } else {
                    next_recv = pick_recv(neighbors, neighbor_count);
                    printf("node %d : send tag : forward, to node %d\n", _rank, next_recv);
                    MPI_Send(NULL, 0, MPI_INT, next_recv, FORWARD, graph_comm);
                }
                break;
        }
    }
 
    printf("node %d : end of work\n", _rank);
    MPI_Finalize();
} 
