#ifndef JOB_H
#define JOB_H

#include <stdlib.h>
#include <semaphore.h> 
#include "protocol.h"

// Job command 
// This holds all the information of a job
typedef struct job{

    int fd;
    char* job_string;
    uint8_t msg_type;

} job_instance;


// Job queue
// This holds all of the job commands 
typedef struct jobs{

    job_instance** jobs_array;
    int n; 
    int front;
    int rear; 
    sem_t queue_mutex; 
    sem_t slots;
    sem_t items; 

} queue;


// Creating the Queue to hold all of the job_instance(s) and initializing semaphores
// This function only calls once
// Parameter : The global variable queue 
//           : Number of job threads (default 2)
void createQueue(queue* job_list, int n); 


// Removing all the job_instance(s) in the Queue 
// This function only calls once 
// Parameter : The global variable queue 
void removeQueue(queue* job_list);


// Creating a job_instance to hold all information of a protocol
// Parameter : File descriptor of the user who calls this function 
//           : Character pointer (string) of the protocol argument 
//           : A hexa decimal of the message type
// Return    : A job_inatance that contains all information of a protocol 
job_instance* createJob(int fd, char* cmd, uint8_t msg_type);


// Getting a job_instance from the queue
// Parameter : The global variable queue 
// Return    : the first job_instance in the queue
job_instance* getJob(queue* job_list);


// Adding a job_instance into the queue
// Parameter : The global variable queue 
//           : The job_instance that wants to be added 
void addJob(queue* job_list, job_instance* command);


#endif

