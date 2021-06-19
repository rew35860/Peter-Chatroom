#ifndef LINKEDLIST_H
#define LINKEDlIST_H


#include "helper.h"
#include "user.h"


// Chat room linked list
// This holds all the information of all the rooms
typedef struct room{

    int host;  //host represented as fd number 
    user* users;  //this is going to be a linked list
    char* room_name;
    struct room* next_room;

} chat_room;


void addRooms(sem_t* userListRead, sem_t* userListWrite, int* userReadCount, \
                sem_t* roomWrite, int hostFd, char* roomName, user* userList, \
                chat_room** chatRooms);


chat_room* create_room(int fd, char* name, user* head);


void add_room(chat_room** room_list, chat_room* new_room);


void deleteRoom(chat_room** room_list, char* name);


void joinRoom(int fd, char* room_name, user* head, chat_room** room_list);


void leaveRoom(int fd, char* room_name, chat_room** room_list);


int roomExists(chat_room* room_list, char* name);


user* roomMembers(chat_room* room_list, char* name); 


char* printListRoom(chat_room* room_list);


#endif 


