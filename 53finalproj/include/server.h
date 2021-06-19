#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "room.h"
#include "job.h"

#define BUFFER_SIZE 1024
#define SA struct sockaddr


void sigint_handler(int sig);


void run_server(int server_port);
int server_init(int server_port);


void *process_client(void *clientfd_ptr);
void *process_job();


void create_jobThread(int jobThreads);


void logOutProcess(job_instance* workingJob, petr_header msgDetails);


void createRoomProcess(job_instance* workingJob, petr_header msgDetails);
void removeRoomProcess(job_instance* workingJob, petr_header msgDetails);


void printRoomProcess(job_instance* workingJob, petr_header msgDetails);
void printUserProcess(job_instance* workingJob, petr_header msgDetails);


void joinRoomProcess(job_instance* workingJob, petr_header msgDetails);
void leaveRoomProcess(job_instance* workingJob, petr_header msgDetails);


void messageRoomProcess(job_instance* workingJob, petr_header msgDetails);
void messageUserProcess(job_instance* workingJob, petr_header msgDetails);


#endif
