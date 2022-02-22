#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// coordinators' ranks
#define C0 0
#define C1 1
#define C2 2


// print topology
void print_topology(int **topology, int rank)
{
    printf("%d -> ", rank);
    for (int i = 0; i < 3; i++) {
        printf("%d:", topology[i][0]);
        for (int j = 0; j < topology[i][1]; j++) {
            if (j == topology[i][1] - 1) {
                printf("%d ", topology[i][j + 2]);
            } else {
                printf("%d,", topology[i][j + 2]);
            }
        }
    }
    printf("\n");

}

// print the final result
void print_result(int V[], int N)
{
    printf("Rezultat: ");
    for (int i = 0; i < N; i++) {
        printf("%d ", V[i]);
    }
    printf("\n");
    
}

int min(int a, int b) 
{
    return a < b ? a : b;
}

// returns the file name corresponding to the coordinator
void generate_file_name(int rank, char *file_name)
{
    if (rank == C0) {
        strcpy(file_name, "cluster0.txt");
    }

    if (rank == C1) {
        strcpy(file_name, "cluster1.txt");
    }

    if (rank == C2) {
        strcpy(file_name, "cluster2.txt");
    }

}

// free memory
void free_topology(int **topology)
{
    for (int i = 0; i < 3; i++) {
       free(topology[i]);
    } 

}

// workers process their task
void process_array(int *V, int N, int **topology, int rank)
{
    int total_workers = topology[0][1] + topology[1][1] + topology[2][1];   
    int start = (rank - 3) * (double)N / total_workers;
    int end = min((rank - 2) * (double)N / total_workers, N);
    for (int i = start; i < end; i++) {
        V[i] *= 2;
    }

}


int main(int argc, char *argv[])
{
	int procs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // each process has access to a file for writting 
    // messages like M(x, y)
    FILE *fp = fopen("output.txt", "a+");
    fprintf(fp, "%s", "\n");

    if (rank < 3) {

        // open the file corresponding to the coordinator
        char file_name[50] = "";
        generate_file_name(rank, file_name);
        FILE *file = fopen(file_name, "r");
        
        // the current process reads his cluster from file
        int workers;
        fscanf(file, "%d", &workers);
        int *topology[3];
        topology[rank] = malloc((workers + 2) * sizeof(int));
        topology[rank][0] = rank;
        topology[rank][1] = workers;
        
        for (int i = 0; i < workers; i++) {
            // add workers in coordinator's topology
            fscanf(file, "%d", &topology[rank][i + 2]);
            
            // the coordinator sends his rank to his workers
            MPI_Send(&rank, 1, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
            fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
        }

        // the current coordinator sends his topology to the other two coordinators
        for (int i = 0; i < 3; i++) {
            if (i != rank) {
                int len = workers + 2;
                MPI_Send(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, i);
                MPI_Send(topology[rank], len, MPI_INT, i, 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, i);
            }
        }

        // the current coordinator receives the topology of the other two coordinators
        // and updates the final topology
        for (int i = 0; i < 3; i++) {
            if (i != rank) {
                MPI_Status status;
                int len_cluster;
                MPI_Recv(&len_cluster, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                topology[status.MPI_SOURCE] = malloc(len_cluster * sizeof(int));
                MPI_Recv(topology[status.MPI_SOURCE], len_cluster, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            }

        }

        // the coordinators send the final topology to their workers
        for (int i = 0; i < topology[rank][1]; i++) {   
            for (int j = 0; j < 3; j++) {
                int len = topology[j][1] + 2;
                MPI_Send(&len, 1, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
                MPI_Send(topology[j], len, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
            }
        }

        // print the final topology
        print_topology(topology, rank);

        if (rank == C0) {

            // generates the array of dimension N
            int N = atoi(argv[1]);
            int V[N];
            for (int i = 0; i < N; i++) {
                V[i] = i;
            }

            // C0 sends to the C1 and C2 the 
            // dimension N and the generated array to be processed
            for (int i = 1; i < 3; i++) {
                MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, i);
                MPI_Send(V, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, i);
            }   

            // coordinator0 sends to his workers the size N and 
            // the generated array to be processed
            for (int i = 0; i  < topology[rank][1]; i++) {
                MPI_Send(&N, 1, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
                MPI_Send(V, N, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
            }

            // receives results from workers
            // calculates the start and end of each worker process to know 
            // which part of the vector to update
            MPI_Status status;
            int V2[N];
            int total_workers = topology[0][1] + topology[1][1] + topology[2][1];   
            for (int i = 0; i < topology[rank][1]; i++) {
                MPI_Recv(V2, N, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD, &status);
                
                int start = (status.MPI_SOURCE - 3) * (double)N / total_workers;
                int end = min((status.MPI_SOURCE - 2) * (double)N / total_workers, N);
                for (int j = start; j < end; j++) {
                    V[j] = V2[j];
                }
            }

            // receives results from C1 and C2
            // calculates the start and end of each worker so
            // C1 and C2 know which part of the array to update
            int V3[N];
            for (int i = 0; i < 3; i++) {
                if (i != C0) {
                    MPI_Recv(V3, N, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    for (int j = 0; j < topology[status.MPI_SOURCE][1]; j++) {
                        int start = (topology[status.MPI_SOURCE][j + 2] - 3) * (double)N / total_workers;
                        int end = min((topology[status.MPI_SOURCE][j + 2] - 2) * (double)N / total_workers, N);
                        for (int k = start; k < end; k++) {
                            V[k] = V3[k];
                        }   
                    }
                }
            }
            
            // print the final result
            print_result(V, N);

        } else {

            // C1 and C2 receive N and the array from C0
            int N;
            MPI_Status status;
            MPI_Recv(&N, 1, MPI_INT, C0, 0, MPI_COMM_WORLD, &status);
            int V[N];
            MPI_Recv(V, N, MPI_INT, C0, 0, MPI_COMM_WORLD, &status);

            // the coordinators send the vector to their workers
            for (int i = 0; i  < topology[rank][1]; i++) {
                MPI_Send(&N, 1, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
                MPI_Send(V, N, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD);
                fprintf(fp, "M(%d,%d)\n", rank, topology[rank][i + 2]);
            }

            // the coordinators receive the processed array from their workers
            int V2[N];
            int total_workers = topology[0][1] + topology[1][1] + topology[2][1];   
            for (int i = 0; i < topology[rank][1]; i++) {
                MPI_Recv(V2, N, MPI_INT, topology[rank][i + 2], 0, MPI_COMM_WORLD, &status);
                int start = (status.MPI_SOURCE - 3) * (double)N / total_workers;
                int end = min((status.MPI_SOURCE - 2) * (double)N / total_workers, N);
                for (int j = start; j < end; j++) {
                    V[j] = V2[j];
                }
            }

            // C1 and C2 send the processed array to C0
            MPI_Send(V, N, MPI_INT, C0, 0, MPI_COMM_WORLD);
            fprintf(fp, "M(%d,%d)\n", rank, C0);
        
        }

        // free memory
        free_topology(topology);

    } else {
        
        // the workers receive the rank of their coordinator
        int coordinator_rank;
        MPI_Status status;
        MPI_Recv(&coordinator_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        
        // the workers receive the final topology from their coordinator
        int *final_topology[3];
        for (int i = 0; i < 3; i++) {
            int len;
            MPI_Recv(&len, 1, MPI_INT, coordinator_rank, 0, MPI_COMM_WORLD, &status);
            final_topology[i] = malloc(len * sizeof(int));
            MPI_Recv(final_topology[i], len, MPI_INT, coordinator_rank, 0, MPI_COMM_WORLD, &status);
        }

        // each worker prints the final topology
        print_topology(final_topology, rank);

        // the workers receive the vector from their coordinator
        int N;
        MPI_Recv(&N, 1, MPI_INT, coordinator_rank, 0, MPI_COMM_WORLD, &status);
        int V[N];
        MPI_Recv(V, N, MPI_INT, coordinator_rank, 0, MPI_COMM_WORLD, &status);

        // each worker performs the calculations corresponding to the rank
        process_array(V, N, final_topology, rank);

        // after processing, the workers send the processed vector to their coordinator
        MPI_Send(V, N, MPI_INT, coordinator_rank, 0, MPI_COMM_WORLD);
        fprintf(fp, "M(%d,%d)\n", rank, coordinator_rank);

        // free memory
        free_topology(final_topology);

    }

    // when all tasks are completed, process 0 goes through the file with 
    // M(x,y) messages and prints them at stdout
    MPI_Barrier (MPI_COMM_WORLD);
    if (rank == C0) {
        FILE *fp2 = fopen("output.txt", "r");
        char out[6];
        while(fscanf(fp2, "%s", out) != EOF) {
            printf("%s\n", out);
        }

        // I open the file in "w" mode to delete all the content
        // before running another test
        FILE *fp3 = fopen("output.txt", "w");
        fclose(fp3);
        fclose(fp2);
    }

    fclose(fp);
	MPI_Finalize();

	return 0;
}