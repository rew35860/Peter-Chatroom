#include "server.h"
#include "protocol.h"
#include <pthread.h>
#include <signal.h>


#include<stdio.h>
#include<stdlib.h>
#include <unistd.h> // getopt 
#include <time.h> // date and time

#define USAGE_MSG   "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n"               \
                    "\n -h                  Displays this help menu, and return EXIT_SUCCESS." \
                    "\n -j N                Number of job threads. Default to 2."              \
                    "\n AUDIT_FILENAME      File to output Audit Log messages to."             \
                    "\n PORT_NUMBER         Port number to listen to.\n"                       \


int volatile killThreads = 0;

char buffer[BUFFER_SIZE];
sem_t buffer_lock;

int listen_fd;
time_t t;
struct tm tm;
FILE* audit;
sem_t audit_lock;

user* userlist = NULL;
int user_read_count = 0;
sem_t userlist_read_lock;
sem_t userlist_write_lock;

unsigned int jobThreads = 2; 

queue job_queue;

chat_room* chatRooms = NULL; 
int chatReadCount = 0;
sem_t chatRoom_read_lock;
sem_t chatRoom_write_lock;


int main(int argc, char *argv[]) {
    // Usage Statement -----------------------------------------------------------------------------------
    unsigned int port = 0; 
    int c;

    while ((c = getopt(argc, argv, "hj:")) >= 0)
    {
        switch (c)
        {
        case 'h':
            printf(USAGE_MSG);
            return EXIT_SUCCESS;
        case 'j':
            jobThreads = atoi(optarg);
            break; 
        default:
            fprintf(stderr, USAGE_MSG);
            return EXIT_FAILURE;
        }
    }
    
    t = time(NULL);
    tm = *localtime(&t);
    
    audit = fopen(argv[optind+1], "w+");
    fprintf(audit, "this should show up in the file %02d-%02d %02d:%02d:%02d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    port = atoi(argv[optind]);
    // --------------------------------------------------------------------------------------------------


    // Usage Statement Error Checking --------------------------------------------------------------------
    // Making sure the port number is not one of the system or well-known ports
    if (port < 1023) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        exit(EXIT_FAILURE);
    }

    // Making sure the file can be open or create 
    if (!audit)
    {
        fprintf(stderr, "ERROR: Audit file is not given\n");
        exit(EXIT_FAILURE);
    }
    // --------------------------------------------------------------------------------------------------
    
    
    sem_init(&buffer_lock, 0, 1);
    sem_init(&audit_lock, 0, 1);
    sem_init(&userlist_read_lock, 0, 1);
    sem_init(&userlist_write_lock, 0, 1);
    sem_init(&chatRoom_read_lock, 0, 1);
    sem_init(&chatRoom_write_lock, 0, 1);
    

    createQueue(&job_queue, jobThreads);
    create_jobThread(jobThreads);
    run_server(port);
}



// Additional functions ---------------------------------------------------------------------------------


void *process_job()
{
    petr_header msgDetails;

    //first recieve a job from the job queue
    while(!killThreads){
        job_instance* workingJob = getJob(&job_queue);

        sem_wait(&audit_lock);
        fprintf(audit, "\nRemoving Job from the buffer : 0x%x called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->msg_type, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fprintf(audit, "Working on Job of type : 0x%x called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->msg_type, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);

        switch(workingJob->msg_type)
        {
            case 0x11: ; 
                // Log out 
                logOutProcess(workingJob, msgDetails);
                break;

            case 0x20: ; 
                // Creating a room
                createRoomProcess(workingJob, msgDetails);
                break;

            case 0x21: ;
                // Remove Room 
                removeRoomProcess(workingJob,  msgDetails);
                break;

            case 0x23: ;
                // Display Room
                printRoomProcess(workingJob, msgDetails);
                break;

            case 0x24: ;
                // Join Room 
                joinRoomProcess(workingJob, msgDetails);
                break;

            case 0x25:
                // Leave the room 
                leaveRoomProcess(workingJob, msgDetails);

                break; 
            case 0x26:
                // Send messages to room 
                messageRoomProcess(workingJob, msgDetails);
                break;

            case 0x30: ;
                // Chat to a user
                messageUserProcess(workingJob, msgDetails);
                break;

            case 0x32: ;
                // print user list 
                printUserProcess(workingJob, msgDetails);
                break;
        }

        free(workingJob->job_string);
        free(workingJob);
    }

    return NULL;
}


void run_server(int server_port) {
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    pthread_t tid;

    // additional local vars 
    petr_header msgDetails;
    int readHeader;

    if (signal(SIGINT, sigint_handler) == SIG_ERR){
        fprintf(stderr, "ERROR: SIGNAL ERROR\n");
        exit(EXIT_FAILURE);
    } 

    while (!killThreads) {
        // Wait and Accept the connection from client
        //printf("Wait for new client connection\n");

        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);

        if (*client_fd < 0) {
            //printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        } else {
            //printf("Client connetion accepted\n");
            readHeader = rd_msgheader(*client_fd, &msgDetails);

            // Successfully read the header of the client 
            if(readHeader == 0){
                sem_wait(&buffer_lock);

                read(*client_fd, buffer, msgDetails.msg_len);
                char* username = malloc(msgDetails.msg_len);
                strcpy(username, buffer);
                bzero(buffer, BUFFER_SIZE);
                
                sem_post(&buffer_lock);
                
                //this is a reader write problem we we are going to add a reader
                //and as long as a reader is present (user_read_count != 0) the
                //user_write_lock will be locked
                addReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

                int userExisted = userExists(userlist, username);

                takeReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);
                
                if(userExisted){

                    sem_wait(&audit_lock); 
                    fprintf(audit, "Denied access: %s at socket %d (%02d-%02d %02d:%02d:%02d)\n", username, *client_fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                    fflush(audit);
                    sem_post(&audit_lock); 

                    msgDetails.msg_len = 0 ;
                    msgDetails.msg_type = EUSREXISTS;
                    wr_msg(*client_fd, &msgDetails, buffer); 
                    
                    close(*client_fd);
                }
                else
                {
                    sem_wait(&audit_lock); 
                    fprintf(audit, "Allowed access: %s at socket %d 0x%x (%02d-%02d %02d:%02d:%02d)\n", username, *client_fd, msgDetails.msg_type, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                    fflush(audit);
                    sem_post(&audit_lock);

                    sem_wait(&userlist_write_lock);
                    user* new_user = createUser(username, *client_fd);
                    addUser(&userlist, new_user);
                    sem_post(&userlist_write_lock);
                    
                    msgDetails.msg_len = 0 ;
                    msgDetails.msg_type = OK;
                    wr_msg(*client_fd, &msgDetails, buffer); 
                }

                
                // If successfully added a user, spawn client thread
                if(!userExisted)
                {
                    pthread_create(&tid, NULL, process_client, (void *)client_fd);
                }
            }
            else
            {
                printf("Fail to read header");
            }
        }
    }
    return;
}


//this is the client thread
void *process_client(void *clientfd_ptr) 
{   
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);

    sem_wait(&audit_lock); 
    fprintf(audit, "Client Thread created: fd: %d (%02d-%02d %02d:%02d:%02d)\n", client_fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fflush(audit);
    sem_post(&audit_lock); 


    petr_header cmdDetails;    

    int runClient = 1;

    while(runClient && !killThreads){
        int readHeader = rd_msgheader(client_fd, &cmdDetails);

        sem_wait(&audit_lock);
        fprintf(audit, "message was sent with type %d (%02d-%02d %02d:%02d:%02d)\n", cmdDetails.msg_type, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);

        char* tempCmd = malloc(cmdDetails.msg_len);
        int readSize = read(client_fd, tempCmd, cmdDetails.msg_len);
        
        
        if(readSize == 0 && cmdDetails.msg_type == 0){
            job_instance* logoutJob = createJob(client_fd, tempCmd, 0x11);
            logOutProcess(logoutJob, cmdDetails);
            free(logoutJob);
            close(client_fd);
            break;
        }

        if (validRead(cmdDetails.msg_type, cmdDetails.msg_len) == 1)
        {
            sem_wait(&audit_lock);
            fprintf(audit, "a valid message was sent with type %d (%02d-%02d %02d:%02d:%02d)\n", cmdDetails.msg_len,  tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);

            job_instance* new_job = createJob(client_fd, tempCmd, cmdDetails.msg_type);
            addJob(&job_queue, new_job);
            
            sem_wait(&audit_lock); 
            fprintf(audit, "Added Job: 0x%x from client %d (%02d-%02d %02d:%02d:%02d)\n", new_job->msg_type, client_fd,  tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);

            if(cmdDetails.msg_type == LOGOUT){
                runClient = 0;
            }
        }

        bzero(&cmdDetails, sizeof(petr_header));
    }
    return NULL;
}


int server_init(int server_port) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) 
    {
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        //printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    } 

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        //printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } 

    return sockfd;
}


void create_jobThread(int jobThreads)
{
    for(int i = 0; i < jobThreads; ++i)
    {
        pthread_t tid;

        pthread_create(&tid, NULL, process_job, NULL);
    }
}


void sigint_handler(int sig) {
    killThreads = 1;
    removeQueue(&job_queue);

    // freeing the users
    user* temp = NULL;
    while(userlist){
        free(userlist->username);
        temp = userlist;
        userlist = userlist->next;
        free(temp);
    }

    chat_room* tempRoom = NULL;
    while(chatRooms){
        free(chatRooms->room_name);

        while(chatRooms->users){
            free(chatRooms->users->username);        
            temp = chatRooms->users;
            chatRooms->users = chatRooms->users->next;
            free(temp);
        }

        tempRoom = chatRooms;
        chatRooms = chatRooms->next_room;
        free(tempRoom);
    }
    
    fclose(audit);
    close(listen_fd);
    exit(0);
}



// Process Job Functions Start here! ----------------------------------------------------------------------

void logOutProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);
                
    chat_room* temp = chatRooms;
    chat_room* prev = NULL;
    while(temp){
        char* nameCopy = malloc(sizeof(char) * (strlen(temp->room_name) + 1));
        strcpy(nameCopy, temp->room_name);
        job_instance* tempJob = createJob(workingJob->fd, nameCopy, 0x11);

        if(temp->host == workingJob->fd){
            //deleteRoom
            removeRoomProcess(tempJob,  msgDetails);                 

        }else{
            //leaveRoom 
            leaveRoomProcess(tempJob, msgDetails);
        }

        free(nameCopy);
        free(tempJob);    

        temp = temp->next_room;
    }
    
    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);
    
    // remove user 
    sem_wait(&userlist_write_lock);
    
    removeUser(&userlist, workingJob->fd);

    sem_post(&userlist_write_lock);

    //printf("ok cool! out of this\n");
    msgDetails.msg_len = 0 ;
    msgDetails.msg_type = OK;
    wr_msg(workingJob->fd, &msgDetails, buffer); 
    
    sem_wait(&audit_lock);
    fprintf(audit, "%d LOG OUT! (%02d-%02d %02d:%02d:%02d)\n", workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fflush(audit);
    sem_post(&audit_lock);

    close(workingJob->fd);
}


void createRoomProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    int exists = roomExists(chatRooms, workingJob->job_string); 
    
    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    if (exists)
    {
        msgDetails.msg_len = 0 ;
        msgDetails.msg_type = ERMEXISTS;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Existed Room can not be added : %s called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
    else
    {
        msgDetails.msg_len = 0 ;
        msgDetails.msg_type = OK;
        wr_msg(workingJob->fd, &msgDetails, buffer); 
        
        // Protect user read count 

        addRooms(&userlist_read_lock, &userlist_write_lock, &user_read_count, \
                    &chatRoom_write_lock, workingJob->fd, workingJob->job_string, \
                    userlist, &chatRooms);
        
        //printRooms(chatRooms);
        sem_wait(&audit_lock);

        fprintf(audit, "Added Room : %s called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);

        sem_post(&audit_lock);
    }
}


void removeRoomProcess(job_instance* workingJob, petr_header msgDetails)
{   
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount); 
    
    int exists = roomExists(chatRooms, workingJob->job_string); 

    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);


    if (exists)
    {
        if (exists == workingJob->fd)
        {
            user* members = roomMembers(chatRooms, workingJob->job_string); 
            while (members->fd != workingJob->fd)
            {
                msgDetails.msg_len = strlen(workingJob->job_string)+1;
                msgDetails.msg_type = RMCLOSED;
                wr_msg(members->fd, &msgDetails, workingJob->job_string); 

                sem_wait(&audit_lock);
                fprintf(audit, "Close Room %s on : %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, members->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                fflush(audit);
                sem_post(&audit_lock);

                members = members->next;
            }

            sem_wait(&chatRoom_write_lock);
            deleteRoom(&chatRooms, workingJob->job_string);
            sem_post(&chatRoom_write_lock);


            if (workingJob->msg_type != 0x11)
            {
                msgDetails.msg_len = 0;
                msgDetails.msg_type = OK;
                wr_msg(workingJob->fd, &msgDetails, buffer); 
            }

            sem_wait(&audit_lock);
            fprintf(audit, "Deleted Room : %s called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);
        }
        else
        {
            msgDetails.msg_len = 0 ;
            msgDetails.msg_type = ERMDENIED;
            wr_msg(workingJob->fd, &msgDetails, buffer); 

            sem_wait(&audit_lock);
            fprintf(audit, "Delete Room Access Denied : %s called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);
        }
        
    }
    else
    {
        msgDetails.msg_len = 0 ;
        msgDetails.msg_type = ERMNOTFOUND;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Room Not Found : %s called by %d (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, workingJob->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
}


void joinRoomProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    int exists = roomExists(chatRooms, workingJob->job_string); 

    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    if(exists){
        // Protect user read count 
        addReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

        joinRoom(workingJob->fd, workingJob->job_string, userlist, &chatRooms);

        takeReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

        msgDetails.msg_len = 0;
        msgDetails.msg_type = OK;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Added User at port %d to %s (%02d-%02d %02d:%02d:%02d)\n", workingJob->fd, workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
    else
    {
        msgDetails.msg_len = 0;
        msgDetails.msg_type = ERMNOTFOUND;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Room %s does not exist (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
}


void leaveRoomProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);
                
    int exists = roomExists(chatRooms, workingJob->job_string); 

    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    if(exists)
    {
        if (exists == workingJob->fd)
        {
            msgDetails.msg_len = 0;
            msgDetails.msg_type = ERMDENIED;
            wr_msg(workingJob->fd, &msgDetails, buffer); 

            sem_wait(&audit_lock);
            fprintf(audit, "Creater can't leave %s room (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);
        }
        else
        {
            sem_wait(&chatRoom_write_lock);
            leaveRoom(workingJob->fd, workingJob->job_string, &chatRooms);
            sem_post(&chatRoom_write_lock);
            
            if (workingJob->msg_type != 0x11)
            {
                msgDetails.msg_len = 0;
                msgDetails.msg_type = OK;
                wr_msg(workingJob->fd, &msgDetails, buffer); 
            }

            sem_wait(&audit_lock);
            fprintf(audit, "%d leave %s room (%02d-%02d %02d:%02d:%02d)\n", workingJob->fd, workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);
        }
    }
    else
    {
        msgDetails.msg_len = 0;
        msgDetails.msg_type = ERMNOTFOUND;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Room %s does not exist (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
}


void printRoomProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    char* roomListString = printListRoom(chatRooms);

    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);
    
    
    if(roomListString != NULL){
        msgDetails.msg_len = strlen(roomListString) + 1;
        
        sem_wait(&audit_lock);
        fprintf(audit, "%s (%02d-%02d %02d:%02d:%02d)", roomListString, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);

        
    }else{
        msgDetails.msg_len = 0;

        sem_wait(&audit_lock);
        fprintf(audit, "Empty room_list (%02d-%02d %02d:%02d:%02d)\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
    
    msgDetails.msg_type = RMLIST;
    wr_msg(workingJob->fd, &msgDetails, roomListString); 

    free(roomListString);    
}


void messageRoomProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    char* roomName = strtok(workingJob->job_string, "\r\n");
    int exists = roomExists(chatRooms, roomName ); 
    
    takeReader(&chatRoom_read_lock, &chatRoom_write_lock, &chatReadCount);

    if(exists){
        // Protect user read count 
        addReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

        user* members = roomMembers(chatRooms, roomName );
        char* sender = getUserName(userlist, workingJob->fd);
        exists = userExists(members, sender); 

        
        takeReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);


        if(exists)
        {
            sem_wait(&buffer_lock);
            bzero(buffer, BUFFER_SIZE);
            strcat(buffer, roomName);
            strcat(buffer, "\r\n");
            strcat(buffer, sender);
            strcat(buffer, "\r");
            strcat(buffer, roomName + strlen(roomName) + 1);
            sem_post(&buffer_lock);

            while (members)
            {
                if (members->fd != workingJob->fd)
                {
                    msgDetails.msg_len = strlen(buffer) + 1;
                    msgDetails.msg_type = RMRECV;
                    wr_msg(members->fd, &msgDetails, buffer); 

                    sem_wait(&audit_lock);
                    fprintf(audit, "User %s sends message: %s to %s room (%d) (%02d-%02d %02d:%02d:%02d)\n", sender, buffer, workingJob->job_string, members->fd, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                    fflush(audit);
                    sem_post(&audit_lock);
                }

                members = members->next;
            }

            msgDetails.msg_len = 0;
            msgDetails.msg_type = OK;
            wr_msg(workingJob->fd, &msgDetails, buffer); 
        }
        else
        {
            msgDetails.msg_len = 0;
            msgDetails.msg_type = ERMDENIED;
            wr_msg(workingJob->fd, &msgDetails, buffer); 

            sem_wait(&audit_lock);
            fprintf(audit, "%s is not in the %s room (%02d-%02d %02d:%02d:%02d)\n", sender, workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            fflush(audit);
            sem_post(&audit_lock);
        }
        
        sem_wait(&buffer_lock);
        bzero(buffer, BUFFER_SIZE); 
        sem_post(&buffer_lock);
        
        free(sender);                   
    }
    else
    {
        msgDetails.msg_len = 0;
        msgDetails.msg_type = ERMNOTFOUND;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "Room %s does not exist (%02d-%02d %02d:%02d:%02d)\n", workingJob->job_string, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
}


void messageUserProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

    char* recipient = strtok(workingJob->job_string, "\r\n");
    fprintf(audit, "the recipient is %s (%02d-%02d %02d:%02d:%02d)\n", recipient, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    int exists = userExists(userlist, recipient);
    fprintf(audit, "result of exists %d (%02d-%02d %02d:%02d:%02d)\n ", exists, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    char* sender = getUserName(userlist, workingJob->fd);

    takeReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);


    if(exists)
    {
        
        sem_wait(&buffer_lock);
        
        bzero(buffer, BUFFER_SIZE);
        strcat(buffer, sender);
        strcat(buffer, "\r");
        strcat(buffer, recipient + strlen(recipient) + 1);
        
        sem_post(&buffer_lock);
        
        msgDetails.msg_len = strlen(buffer)+1;
        msgDetails.msg_type = USRRECV;
        wr_msg(exists, &msgDetails, buffer); 

        free(sender);
        
        msgDetails.msg_len = 0;
        msgDetails.msg_type = OK;
        wr_msg(workingJob->fd, &msgDetails, buffer); 

        sem_wait(&audit_lock);
        fprintf(audit, "%s sending: %s to %s with fd: %d (%02d-%02d %02d:%02d:%02d)\n", sender, buffer, recipient, exists, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }
    else
    {
        msgDetails.msg_len = 0;
        msgDetails.msg_type = EUSRNOTFOUND;
        wr_msg(workingJob->fd, &msgDetails, buffer); 
        
        sem_wait(&audit_lock);
        fprintf(audit, "USER NOT FOUND : %s (%02d-%02d %02d:%02d:%02d)\n", recipient, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fflush(audit);
        sem_post(&audit_lock);
    }

    sem_wait(&buffer_lock);
    bzero(buffer, BUFFER_SIZE); 
    sem_post(&buffer_lock);
}
        

void printUserProcess(job_instance* workingJob, petr_header msgDetails)
{
    addReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

    char* userListString = printListUser(userlist, workingJob->fd);

    takeReader(&userlist_read_lock, &userlist_write_lock, &user_read_count);

    if (strlen(userListString) == 0)
    {
        msgDetails.msg_len = 0;
    }
    else
    {
        msgDetails.msg_len = strlen(userListString)+1;
    }
    
    msgDetails.msg_type = USRLIST;
    wr_msg(workingJob->fd, &msgDetails, userListString); 

    sem_wait(&audit_lock);
    fprintf(audit, "Current Users : %s (%02d-%02d %02d:%02d:%02d)\n", userListString, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fflush(audit);
    sem_post(&audit_lock);

    free(userListString);
}


