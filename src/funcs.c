#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "header.h"

// conditional variables for the send request threads
// initialization of thread condition variable
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER; //signal to allow height recieve to continue.
pthread_cond_t startAlert = PTHREAD_COND_INITIALIZER;



// initializing mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nhMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rCountMutex = PTHREAD_MUTEX_INITIALIZER;

// Global variables
MPI_Comm comm2D;
float height = -1.0;
int towait_flag[4] = {-1, -1, -1, -1};
float neighbor_h[4] = {-1,-1,-1,-1}; // stores the values of height from neghbours.
int recvCount=0;
// int sendFlag = 0;


// function definitions (threads are stored in a seperate)
int ProtectedReadflag(int nbr_pos){
    pthread_mutex_lock(&fMutex);
    int x = towait_flag[nbr_pos];
    pthread_mutex_unlock(&fMutex);
    return x;
}

float ProtectedReadHeight(){
    pthread_mutex_lock(&hMutex);
    float x = height;
    pthread_mutex_unlock(&hMutex);
    return x;
}