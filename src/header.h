#ifndef __SHAF__H__
# define __SHAF__H__

// conditional variables for the send request threads
// Declaration of thread condition variable
extern pthread_cond_t cond1;
extern pthread_cond_t cond2;
extern pthread_cond_t startAlert;



// declaring mutex
extern pthread_mutex_t lock;
extern pthread_mutex_t lock2;
extern pthread_mutex_t hMutex;
extern pthread_mutex_t fMutex;
extern pthread_mutex_t nhMutex;
extern pthread_mutex_t rCountMutex;

//  Structs
struct listenArgs{
    int id;
    int neighbor_rank;
    int my_rank;
    int neighbor_index;
    int num_neighbor;
    int* nbr_pos;
};

// Function Prototype
void *ListenReq(void *pArg);
void *SendReq(void *pArg); //send rank
void *RecHeight(void *pArg);
void *SendHeight(void *pArg);
void *AlertBase(void *pArg);
int ProtectedReadflag(int nbr_pos);
float ProtectedReadHeight();


// Global variables
extern MPI_Comm comm2D;
extern float height;
extern int towait_flag[4];
extern float neighbor_h[4];
extern int recvCount;
// int sendFlag = 0;

// thread declararion
void *ListenReq(void *pArg);
void *SendReq(void *pArg);
void *SendHeight(void *pArg);
void *RecHeight(void *pArg);
void *AlertBase(void *pArg);

//function decalaration
int ProtectedReadflag(int nbr_pos);
float ProtectedReadHeight();

#endif 