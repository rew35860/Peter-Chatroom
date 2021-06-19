#ifndef HELPER_H
#define HELPER_H


#include "protocol.h"
#include <semaphore.h>


//when passed an integer msg_type, the function
//returns 1 if the type is a valid type and 0 if not
int validMsgType(int msg_type);


//when passed a msg_type and msg_len, function returns
//1 if the msg_type is valid and if msg_len is non zero
//when msg_type is valid
int validRead(int msg_type, int msg_len);


//by passing the semaphore readSem and writeSem by reference, 
//the function will enable the readSem which will protect 
//incrementing readerCount and lock the writeSem if readerCount > 0
void addReader(sem_t* readSem, sem_t* writeSem, int* readerCount);


//by passing the semaphore readSem and writeSem by reference, 
//the function will enable the readSem which will protect 
//decrementing readerCount and unlock the writeSem if readerCount == 0
void takeReader(sem_t* readSem, sem_t* writeSem, int* readerCount);


#endif
