#include "job.h"



void createQueue(queue* job_list, int n)
{
    job_list->jobs_array = calloc(n, sizeof(job_instance)); 
    job_list->n = n;
    job_list->front = job_list->rear = 0; 
    sem_init(&job_list->queue_mutex, 0, 1); 
    sem_init(&job_list->slots, 0, n);
    sem_init(&job_list->items, 0, 0);

    for (int i = 0; i < n; ++i)
    {
        job_list->jobs_array[i] = NULL;
    }
}


void removeQueue(queue* job_list)
{
    int i = 0;

    for(i; i < job_list->n; ++i){
    
        if (job_list->jobs_array[i] != NULL)
        {
            free(job_list->jobs_array[i]->job_string);
            free(job_list->jobs_array[i]);
        }
    }
    free(job_list->jobs_array);
}


job_instance* createJob(int fd, char* cmd, uint8_t msg_type){
    job_instance* res = malloc(sizeof(job_instance));
    res->fd = fd;
    res->job_string = cmd;
    res->msg_type = msg_type;
    return res;
}


void addJob(queue* job_list, job_instance* command){
    sem_wait(&job_list->slots);
    sem_wait(&job_list->queue_mutex);

    job_list->jobs_array[++job_list->rear % job_list->n] = command;

    sem_post(&job_list->queue_mutex);
    sem_post(&job_list->items);
}


job_instance* getJob(queue* job_list){
    sem_wait(&job_list->items);
    sem_wait(&job_list->queue_mutex);

    job_instance* res = job_list->jobs_array[++job_list->front % job_list->n];

    sem_post(&job_list->queue_mutex);
    sem_post(&job_list->slots);

    return res;
}


