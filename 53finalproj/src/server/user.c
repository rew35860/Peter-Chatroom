#include "user.h"



user* createUser(char* username, int fd)
{
    user* newUser = malloc(sizeof(user));
    newUser->fd = fd;
    newUser->username = malloc(sizeof(char) * (strlen(username) + 1));  // Plus one for NULL terminator 
    strcpy(newUser->username, username);
    newUser->next = NULL;
    
    return newUser;
}


int userExists(user* head, char* name)
{
    if(head == NULL)
    {
        return 0;
    }

    user* temp = head;

    while(temp != NULL && strcmp(temp->username, name) != 0)
    {
        temp = temp->next; 
    }

    if(temp == NULL)
    {
        return 0;
    }
    else
    {
        return temp->fd;
    }
}


void addUser(user** head, user* client)
{
    if(*head == NULL){
        *head = client;
        return;
    }

    client->next = *head;
    *head = client;
    return;
}


void removeUser(user** head, int fd)
{
    if (*head != NULL)
    {
        if ((*head)->fd == fd)
        {
            free((*head)->username); 
            user* trash = *head;
            *head = trash->next; 
            free(trash);
        }

        user* temp = *head;
        user* prev = NULL;
        
        while(temp != NULL)
        {
            if (temp->fd != fd)
            {
                prev = temp;
                temp = temp->next;    
            }
            else
            {
                prev->next = temp->next;
                free(temp->username);
                free(temp);
                break;
            }
        }
    }
    return;
}


user* getUser(user* head, int fd)
{
    if(head == NULL)
    {
        return NULL;
    }

    user* temp = head;

    while(temp != NULL && temp->fd != fd) 
    {
        temp = temp->next; 
    }

    if(temp == NULL)
    {
        return NULL;
    }
    else
    {
        // Creating a copy, so it can be deleted when removing the user without effecting the user linkedlist 
        char* name = malloc(strlen(temp->username));
        strcpy(name, temp->username);

        return createUser(name, fd);
    }
}


char* getUserName(user* head, int fd){
    if(head == NULL)
    {
        return NULL;
    }

    user* temp = head;

    while(temp != NULL && temp->fd != fd) 
    {
        temp = temp->next; 
    }

    if(temp == NULL)
    {
        return NULL;
    }
    else
    {
        char* name = malloc(sizeof(char) * (strlen(temp->username) + 1));
        strcpy(name, temp->username);

        return name;
    }
}


char* printListUser(user* head, int fd){
    char* res = malloc(sizeof(char));
    bzero(res, strlen(res));

    user* tempUsers = head;
    int totalLen = 1;

    while(tempUsers)
    {
        if (tempUsers->fd != fd)
        {
            totalLen = sizeof(char) * (totalLen + strlen(tempUsers->username) + 2);
            res = realloc(res, totalLen);
            strcat(res, tempUsers->username);
            strcat(res, "\n");
        }
        tempUsers = tempUsers->next;    
    }

    return res;
}


