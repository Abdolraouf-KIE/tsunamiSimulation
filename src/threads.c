#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "header.h"

//thread defenition
void *ListenReq(void *pArg){ //[0..3]
    struct listenArgs *args = pArg;
    if(args->my_rank == 1 && args->neighbor_rank == 0){printf("\t\tHERE %d\n", args->neighbor_index);}
    //printf("\t\tRank %d made LR Thread [%d] for neighbor %d\n", args->my_rank, args->id, args->neighbor_rank);
    int msg_received = -1;
    // //printf("Thread %d listens to: %d\n",(args->id),(args->neighbor_rank));
    // if((args->my_rank) == 0){
    printf("Rank %d, Thread %d,ListenReq, waiting for request from %d\n", args->my_rank, args->id, args->neighbor_rank);fflush(stdout);
    MPI_Recv(&msg_received, 1, MPI_INT, args->neighbor_rank, 0, comm2D, MPI_STATUS_IGNORE);
    printf("Rank %d, Thread %d,ListenReq, received request from %d\n", args->my_rank, args->id, msg_received);fflush(stdout);
    //}
    //sending signal so that the RecHeight threads start receiving the height.
    // printf("#### Rank %d, Thread %d,ListenReq, send broadcast for SH.\n",args->my_rank, args->id);fflush(stdout);
    //protection
    
    pthread_mutex_lock( &fMutex );
    towait_flag[args->neighbor_index] = 1;
    pthread_mutex_unlock( &fMutex );

    pthread_cond_broadcast(&cond2); /// hits/unlocks send to 3 before 0 because 3 is Bottom, 0 is Right [--,3,--,0]
    
    printf("Rank %d, Thread %d,ListenReq, DONE\n",args->my_rank, args->id);fflush(stdout);
    return NULL;
}

void *SendReq(void *pArg){  //[4..7]
    struct listenArgs *args = pArg;
    // for (int i = 0; i < 10; i++)
    // {
    //     printf("\t\tRank %d (%d)made SR Thread [%d] for neighbor %d\n", args->my_rank, i,args->id, args->neighbor_rank);
    //     sleep(1);
    // }
    
    // printf("\t\tRank %d made SR Thread [%d] for neighbor %d\n", args->my_rank, args->id, args->neighbor_rank);

    //if(args->my_rank == 0)
    pthread_mutex_lock(&lock);
    

    //waiting for signal
    printf("Rank %d, Thread %d, SendReq, 1/5 waiting for signal of > 6k.\n", args->my_rank, args->id);fflush(stdout);
    // pthread_mutex_lock(&h);                              
    // pthread_cond_wait(&cond1, &lock);
    
    while(ProtectedReadHeight() < 6000) 
    {
        pthread_cond_wait(&cond1, &lock);
        pthread_mutex_unlock(&lock);
    }
    printf("Rank %d, Thread %d, SendReq, 2/5 recieved signal for > 6k.\n", args->my_rank, args->id);fflush(stdout);

    printf("Rank %d, Thread %d, SendReq, 3/5 sending request to %d.\n",args->my_rank, args->id, args->neighbor_rank);fflush(stdout);
    MPI_Send(&args->my_rank, 1, MPI_INT, args->neighbor_rank, 0, comm2D);
    printf("Rank %d, Thread %d, SendReq, 4/5 request sent to %d.\n",args->my_rank, args->id, args->neighbor_rank); fflush(stdout);

    printf("Rank %d, Thread %d, SendReq, 5/5 DONE.\n",args->my_rank, args->id);fflush(stdout);
    return NULL;
}

void *SendHeight(void *pArg){ //[12..16]
    // struct listenArgs *args = pArg;
    // printf("\t\tRank %d made SH Thread [%d] for neighbor %d\n", args->my_rank, args->id, args->neighbor_rank);
    struct listenArgs *args = pArg;
    float height2send;

    if(args->my_rank == 1 && args->neighbor_rank == 0) {printf("\t\tHERE 2: %d\n", args->neighbor_index);}

    // wait for condition placed inside the sending request.
    pthread_mutex_lock( &lock2 );
    printf("Rank %d, Thread %d, SendHeight, 1/5 waiting for signal to send height.\n",args->my_rank,args->id);fflush(stdout);
    
    while(ProtectedReadflag(args->neighbor_index) < 1)pthread_cond_wait(&cond2, &lock2);

    printf("Rank %d, Thread %d, SendHeight, 2/5 recieved signal to send height.\n",args->my_rank,args->id);fflush(stdout);
    
    //protect reading height (lock, hMutex)
    pthread_mutex_lock( &hMutex );
    height2send = height;
    pthread_mutex_unlock( &hMutex );
    
    // while(sendFlag != 1){
    //     pthread_cond_wait(&condSend);         /////////////////////////////////////////////////
    // }


    printf("Rank %d, Thread %d, SendHeight, 3/5 sending height to %d.\n",args->my_rank, args->id, args->neighbor_rank);fflush(stdout);
    MPI_Send(&height2send, 1, MPI_FLOAT, args->neighbor_rank, 1, comm2D);
    printf("Rank %d, Thread %d, SendHeight, 4/5 height sent to %d.\n",args->my_rank, args->id, args->neighbor_rank);fflush(stdout);

    pthread_mutex_unlock( &lock2 );
    //unlcok
    
    printf("Rank %d, Thread %d, SendHeight, 5/5 DONE.\n",args->my_rank, args->id);fflush(stdout);
    return NULL;
}

void *RecHeight(void *pArg)  //[8..11]
{
    // struct listenArgs *args = pArg;
    // printf("\t\tRank %d made RH Thread [%d] for neighbor %d\n", args->my_rank, args->id, args->neighbor_rank);
    struct listenArgs *args = pArg;
    float recieved_height = -1;

    
    printf("Rank %d, Thread %d, RecHeight, 1/4 waiting to recieve height from %d\n", args->my_rank, args->id, args->neighbor_rank);fflush(stdout);
    MPI_Recv(&recieved_height, 1, MPI_FLOAT,args->neighbor_rank, 1, comm2D, MPI_STATUS_IGNORE);
    printf("Rank %d, Thread %d, RecHeight, 2/4 recieved height %f from %d.\n",args->my_rank, args->id, recieved_height ,args->neighbor_rank);fflush(stdout);

    pthread_mutex_lock(&nhMutex);
    neighbor_h[args->neighbor_index] = recieved_height;
    pthread_mutex_unlock(&nhMutex);
    
    //lock counter
    printf("Rank %d, Thread %d, RecHeight, 3/4 counting recieved height.\n",args->my_rank, args->id);fflush(stdout);
    pthread_mutex_lock(&rCountMutex);
    recvCount++;
    if(recvCount >= (args->num_neighbor))
    {
        pthread_mutex_unlock(&rCountMutex);
        pthread_cond_signal( &startAlert);
        printf("Rank %d, Thread %d: Sending Alert signal.\n",args->my_rank, args->id);fflush(stdout);

    }

    printf("Rank %d, Thread %d, RecHeight, 4/4 DONE.\n",args->my_rank, args->id);fflush(stdout);
    return NULL;
    
}

void *AlertBase(void *pArg)
{
    struct listenArgs *args = pArg;

    //wait lock
    pthread_mutex_lock(&rCountMutex);
    while(recvCount < args->num_neighbor){
        pthread_cond_wait(&startAlert,&rCountMutex);
    }
    recvCount=0; //reset

    pthread_mutex_unlock(&rCountMutex);
    
    //does computation and communication.
    //protect
    int num_match=0;
    
    for(int i =0; i < args->num_neighbor; i++){
        pthread_mutex_lock(&nhMutex);pthread_mutex_lock(&hMutex);
        if( abs(neighbor_h[args->nbr_pos[i]] - height) <= 200){
            num_match++;
        }
        pthread_mutex_unlock(&nhMutex);pthread_mutex_unlock(&hMutex);

    }
    //if matches >= 2
    //send alert to base station
    if (num_match>=2)
    {
        printf("Rank %d, Thread alert, Number of matches: %d", args->my_rank, num_match);
    }
    // MPI's coordinate, rank, height
    // Adj MPI's coordinate, rank, height
    // num_matches
    //Start timer (before MPI Send)


    return NULL;
}
