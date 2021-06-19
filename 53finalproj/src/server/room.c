#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "room.h"



void addRooms(sem_t* userListRead, sem_t* userListWrite, int* userReadCount, \
                sem_t* roomWrite, int hostFd, char* roomName, user* userList, \
                chat_room** chatRooms){
    
    addReader(userListRead, userListWrite, userReadCount);
    chat_room* new_room = create_room(hostFd, roomName, userList);
    takeReader(userListRead, userListWrite, userReadCount);

    sem_wait(roomWrite);
    add_room(chatRooms, new_room);
    sem_post(roomWrite);

    return;
}


chat_room* create_room(int fd, char* name, user* head){
    chat_room* new_room = malloc(sizeof(chat_room));
    new_room->host = fd;

    new_room->room_name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(new_room->room_name, name);

    new_room->users = getUser(head, fd);
    new_room->next_room = NULL;
    
    return new_room;
}


void add_room(chat_room** room_list, chat_room* new_room){
    if(*room_list == NULL){
        *room_list = new_room;
        return;
    }

    new_room->next_room = *room_list;
    *room_list = new_room;  
    return;
}


void deleteRoom(chat_room** room_list, char* name){
    chat_room* prev = NULL;
    chat_room* temp = *room_list;
    
    //traverse the room list until temp points to the
    //room that we are looking for
    while(strcmp(name, temp->room_name) != 0)
    {
        prev = temp;
        temp = temp->next_room;
    }

    if(prev != NULL){
        prev->next_room = temp->next_room;
    }
    else{
        (*room_list) = temp->next_room;
    }

    free(temp->room_name);

    //by this point temp is pointing to the target room
    //so we are going to free anything that is allcoated
    user* tempUser = temp->users;
    user* prevUser = NULL;

    while(tempUser)
    {
        free(tempUser->username);
        prevUser = tempUser;
        tempUser = tempUser->next;
        free(prevUser); 
    }

    tempUser = NULL;

    free(temp);
}


void joinRoom(int fd, char* room_name, user* head, chat_room** room_list){
    //first we want to find the user information
    user* new_user = getUser(head, fd);

    //find the room
    chat_room* temp = *room_list;
    
    while(temp != NULL && strcmp(temp->room_name, room_name))
    {
        temp = temp->next_room;
    }

    addUser(&temp->users, new_user);
}


void leaveRoom(int fd, char* room_name, chat_room** room_list)
{
    chat_room* iter = *room_list; 

    while(iter != NULL && strcmp(room_name, iter->room_name) != 0)
    {
        iter = iter->next_room; 
    }

    removeUser(&iter->users, fd);
}


int roomExists(chat_room* room_list, char* name)
{
    int res;    
    chat_room* temp = room_list;
    
    while(temp != NULL && strcmp(temp->room_name, name) != 0)
    {
        temp = temp->next_room;
    }

    if(temp == NULL)
    {
        res = 0;
    }
    else
    {
        res = (temp)->host; 
    }

    return res;
}


user* roomMembers(chat_room* room_list, char* name)
{
    chat_room* temp = room_list;
    
    while(temp != NULL && strcmp(temp->room_name, name))
    {
        temp = temp->next_room;
    }

    return temp->users;
}


char* printListRoom(chat_room* room_list)
{
    if(room_list == NULL){
        return NULL;
    }

    chat_room* room = room_list;
    user* tempUsers = room->users;
    int totalLen = 1;
    
    char* res = malloc(sizeof(char));
    bzero(res, strlen(res));
    
    while(room){
        totalLen = sizeof(char) * (totalLen + strlen(room->room_name) + 2);
        res = realloc(res, totalLen);

        strcat(res, room->room_name);
        strcat(res, ": ");
    
        tempUsers = room->users;

        while(tempUsers)
        {
            totalLen = sizeof(char) * (totalLen + strlen(tempUsers->username) + 1);
            
            res = realloc(res, totalLen);
            strcat(res, tempUsers->username);
            
            if(tempUsers->next != NULL)
            {
                strcat(res, ",");
            }
            else
            {
                strcat(res, "\n");
            }

            tempUsers = tempUsers->next;    
        }
        room = room->next_room;
    }
    return res;
}


