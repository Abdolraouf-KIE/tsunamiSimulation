#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "header.h"  //refering to the header file

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP      1
#define SLEEP     10
#define TOP     0
#define BOTTOM  1
#define LEFT    2
#define RIGHT   3
#define NUM_THREADS 16

int main(int argc, char **argv)
{
    int size, my_rank, my_cart_rank, provided, flag, claimed, ierr;
    int reorder, dims[2], coord[2], wrap[2];
    int rows, cols;
    wrap[0] = 0, wrap[1] = 0;
    reorder = 1;
    int errs = 0;
    int nbr_top, nbr_bot, nbr_left, nbr_right;

    // Initialize environment
    MPI_Init_thread( &argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    //  Checking to ensure MPI_THREAD_MULTIPLE was setup correctly.

            MPI_Is_thread_main( &flag );
            if (!flag) {
                errs++;
                printf( "This thread called init_thread but Is_thread_main gave false\n" );fflush(stdout);
            }
            MPI_Query_thread( &claimed );
            if (claimed != provided) {
                errs++;
                printf( "Query thread gave thread level %d but Init_thread gave %d\n", claimed, provided );fflush(stdout);
            }
    
    // Setup Comms
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Process arguements
    if (argc == 3)
    {
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
        dims[0] = rows; /* number of rows */
        dims[1] = cols; /* number of columns */
        if ((rows * cols) != size)
        {
            if (my_rank == 0)
                printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", rows, cols, rows * cols, size);fflush(stdout);
            MPI_Finalize();
            return 0;
        }
    }
    else
    {
        rows = cols = (int)sqrt(size);
        dims[0] = dims[1] = 0;
    }

    // Create a topology
    MPI_Dims_create(size, 2, dims);
    if (my_rank == 0){
        printf("Root Rank: %d. Comm Size: %d. Grid Dimension = [%d x %d] \n", my_rank, size, dims[0], dims[1]);fflush(stdout);}

    ierr = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, wrap, reorder, &comm2D);
    if (ierr != 0)
        printf("ERROR[%d] creating CART\n", ierr); fflush(stdout);
    
    MPI_Cart_coords(comm2D, my_rank, 2, coord);
    MPI_Cart_rank(comm2D, coord, &my_cart_rank);

    MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &nbr_top, &nbr_bot);
    MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &nbr_left, &nbr_right);

    // printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). Left: %d. Right: %d. Top: %d. Bottom: %d\n", my_rank, my_cart_rank, coord[0], coord[1], nbr_left, nbr_right, nbr_top, nbr_bot);fflush(stdout);

    // Neighbor nodes
    int nbrs[4] = {nbr_top, nbr_bot, nbr_left, nbr_right};
    int num_neigh = 0;
    int j = 0;
    int nbr_rank[4]; // ranks of the neighbors
    int nbr_pos[4]={-1,-1,-1,-1}; //={TOP, BOTTOM, LEFT, RIGHT};
    
    for(int i = 0; i < 4; i++)
    {
        if(nbrs[i] < 0) {continue;}
        num_neigh++;
        nbr_rank[j] = nbrs[i];
        nbr_pos[j] = i;
        j++;
    }

    // Pthreads
    pthread_t lThread[4];
    pthread_t srThread[4];
    pthread_t rhThread[4];
    pthread_t shThread[4];
    pthread_t alertThread;

    // Alert thread
    struct listenArgs args_A;

    args_A.my_rank = my_rank;
    
    args_A.nbr_pos = nbr_pos;
    args_A.num_neighbor = num_neigh;
    printf("Rank %d, # neighbor, %d\n", my_rank, num_neigh);
    pthread_create(&alertThread, NULL, AlertBase, &args_A);

    // Threads for: Listen for request, Send request, Receive height, Send height
    struct listenArgs args_L[4], args_SR[4], args_RH[4], args_SH[4];

    for(int i = 0; i < num_neigh; i++){ //[0......16] [0-3] , [4-7], [8-11], [12-16] //TODO: delay in thread creation
        args_L[i].id = i;
        args_L[i].neighbor_rank = nbr_rank[i];
        args_L[i].my_rank = my_rank;
        args_L[i].neighbor_index = nbr_pos[i];        
                                         
        args_SR[i].id = i+4;
        args_SR[i].neighbor_rank = nbr_rank[i];
        args_SR[i].my_rank = my_rank;

        args_RH[i].id = i+8;
        args_RH[i].neighbor_rank = nbr_rank[i];
        args_RH[i].my_rank = my_rank;
        args_RH[i].neighbor_index = nbr_pos[i];
        args_RH[i].num_neighbor = num_neigh;

        args_SH[i].id = i+12;
        args_SH[i].neighbor_rank = nbr_rank[i];
        args_SH[i].my_rank = my_rank;
        args_SH[i].neighbor_index = nbr_pos[i]; 
        
        pthread_create(&lThread[i], NULL, ListenReq, &args_L[i]);
        pthread_create(&srThread[i],NULL, SendReq, &args_SR[i]);
        pthread_create(&rhThread[i], NULL, RecHeight, &args_RH[i]);
        pthread_create(&shThread[i], NULL, SendHeight, &args_SH[i]);
        
    }
    sleep(3); //change this
    printf("rank %d # neighbor: %d\n", my_rank, num_neigh);

    for(int i = 0; i < num_neigh; i++){
        printf("Rank %d, neighbor, %d\n", my_rank, nbr_rank[i]);
    }


    //Barrier here
    //MPI_Barrier(comm2D);
    // Generate height
    unsigned int seed = time(NULL) + my_rank;                            //to generate unique values per processor
    
    if (my_rank==0 || my_rank==1)
    {
        pthread_mutex_lock( &hMutex );
        height = (float)rand_r(&seed) / (float)(RAND_MAX / 500) + 6000;
        pthread_mutex_unlock( &hMutex );
        // sendFlag = 1;
        printf("Rank %d, height=%f , sending signal of >6k\n", my_rank, height);

        //for(int i = 0; i< num_neigh; i++){
        pthread_cond_broadcast(&cond1);
        //}
    }
    else
    {
        pthread_mutex_lock( &hMutex );
        height = (float)rand_r(&seed) / (float)(RAND_MAX / 500) + 5000;
        pthread_mutex_unlock( &hMutex );
        // sendFlag = 1;
        printf("Rank %d, height=%f\n",my_rank, height);
    }

    // Join
    for(int i = 0; i < num_neigh; i++){
        pthread_join(lThread[i], NULL);
        pthread_join(srThread[i],NULL);
    }

    MPI_Finalize();
    return 0;
}