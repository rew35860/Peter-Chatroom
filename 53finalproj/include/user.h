#ifndef USER_H
#define USER_H

#include <stdlib.h>
#include <string.h>


// User linked list 
// This holds all the information of users 
typedef struct node{

    int fd;
    char* username;
    struct node* next;

} user;


// Dynamically allocating a space for a Node (user) struct 
// Parameters : Character pointer to username, 
//              A file descriptor number associates with the user
// Return     : a Node (user) struct of the specified username with the associated info 
user* createUser(char* username, int fd);


// Finding if the username exist in the linked list Node (user) struct
// Parameters : Linked list of the Node (user) struct / the head of the struct,
//              Character pointer to the username, 
// Return     : 0 if fail, # of the user's file descriptor if success
int userExists(user* head, char* name);


// Adding a Node (user) struct to the front of the linked list 
// Parameters : Linked list of the Node (user) struct / the head of the struct, 
//              Node (user) struct of the user that wants to be added
void addUser(user** head, user* client);


// Removing a Node (user) struct from the linked list based on the input fd
// Parameters : Linked list of the Node (user) struct / the head of the struct, 
//            : A file descriptor number associates with the user that wants to be removed
void removeUser(user** head, int fd);


// Getting the user struct associates with the input user's file descriptor 
// Parameters : Linked list of the Node (user) struct / the head of the struct, 
//            : A file descriptor number associates with the user that calls the function
// Return     : The user struct of the specified user's fd 
user* getUser(user* head, int fd);


// Getting the name (string) of the input user's file descriptor 
// Parameters : Linked list of the Node (user) struct / the head of the struct, 
//            : A file descriptor number associates with the user that calls the function
// Return     : The string of the specified user's fd
char* getUserName(user* head, int fd);  


// Creating a string that contains all the users in the server, excluding the caller 
// Parameters : Linked list of the Node (user) struct / the head of the struct, 
//            : A file descriptor number associates with the user that calls the function
// Return     : The string of all the users in the server, excluding the caller 
char* printListUser(user* head, int fd); 


#endif 


