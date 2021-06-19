#include "helper.h"



int validMsgType(int msg_type){
    //returns 1 if the msg_type is a
    //valid msg type, 0 otherwise
    switch (msg_type){
        case 0x11:
        case 0x20:
        case 0x21:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x30:
        case 0x32:
            return 1;
        default:
            return 0;
    }
}


int validRead(int msg_type, int msg_len){
    //returns a 1 if the msg_type is valid and there is an associated
    //message wil the msg_type RMSEND
    return validMsgType(msg_type) == 1 || (msg_type == RMSEND && msg_len != 0);    
}


void addReader(sem_t* readSem, sem_t* writeSem, int* readerCount){
    //adds 1 to the readerCount to represent a reader present, locks the
    //binary semaphore writeSem, readSem is a binary semaphore to prevent
    //a race condition on the readerCount
    sem_wait(readSem);
    *readerCount++;

    if(*readerCount == 1)
        sem_wait(writeSem);

    sem_post(readSem);
}


void takeReader(sem_t* readSem, sem_t* writeSem, int* readerCount){
    //subtracts 1 from the readerCount to represent a reader leaving, unlocks the
    //binary semaphore writeSem if readerCount == 0, readSem is a binary semaphore 
    //to prevent a race condition on the readerCount
    sem_wait(readSem);
    *readerCount--;

    if(*readerCount == 0)
        sem_post(writeSem);

    sem_post(readSem);
}


