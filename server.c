#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
 
#define PORT 3205
#define MAX_CLIENT 100
static int client_count = 0;
static int group_count = 1;
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char phone_number[32];
	int groupid;
	
} client_t;
typedef struct{
	int g_uid;
	char name[32];
	char password[32];
	
} group_t;

client_t *clients[MAX_CLIENT];
group_t *groups[MAX_CLIENT];


void str_rem_lf (char* arr, int length) { ////Removing empty characters
  int i;
  for (i = 0; i < length; i++) { 
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}
int str_has_space (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i]==' ') {
        return 1;
    }
  }
  return 0;
}

int findGroupIdByName(char *name){ 
    int numb=0;
    for ( int i = 1; i < group_count; i++)
	{
        str_rem_lf(groups[i]->name,32);
        str_rem_lf(name,32);
		if(strcmp(groups[i]->name,name)==0){		
			numb=groups[i]->g_uid;
            break;
		}
	}	
    return numb;
    ///if the return value is 0 means. CLient is in main group.
}
void send_message(char *body, int from,int to){ /////General function for sending message.
	for(int i=0; i<MAX_CLIENT; ++i){
		if(clients[i]){
			if(clients[i]->uid != from && clients[i]->groupid==to){
                if(write(clients[i]->sockfd, body, strlen(body)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}           			
			}
		}
	}
}
void send_server_message(char *body, int to){ ///Function that sends message from the server to the unique client 
	for(int i=0; i<MAX_CLIENT; ++i){
		if(clients[i]){
			if(clients[i]->uid == to){
                if(write(clients[i]->sockfd, body, strlen(body)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}
}
int send_private_message(char *body, int from,char *to){ ///Function that sends message from the server to the unique client 
	int flag=0;
    for(int i=0; i<MAX_CLIENT; ++i){
		if(clients[i]){
			if(strcmp(clients[i]->phone_number,to)==0 && clients[i]->uid!=from){
                if(write(clients[i]->sockfd, body, strlen(body)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
                flag=1;
			}
		}
	}
    return flag;
}
char *split_message(char *str,int position,char *operand){
    
    char *ptr=ptr=strtok(str,operand);
    int cnt=0;
    while(cnt<position){
        ptr= strtok(NULL,operand);
        cnt++;
    }
    return ptr;
}
void *handling_client(void *cli){
    char message[1024]; //original received message from the client.
    char handl_commands[250]; //to determining and getting the given command.
    char phone_number[11]; ///client's phone number.
    int leave_flag = 0;//exiting the program.
    int gid=0;//determine groupid.
    char temp_mssg[1024];///to avoid distortions.
    client_count++;
    client_t *client = (client_t *)cli; 
    group_t *group_ = (group_t *)malloc(sizeof(group_t));

    if(recv(client->sockfd, phone_number, 11, 0) <= 0){
		printf("Didn't enter the phone number.\n");
		leave_flag = 1;//exiting the program.
	} else{
		strcpy(client->phone_number, phone_number);
        str_rem_lf(client->phone_number,11);
		sprintf(message, "%s has joined\n", client->phone_number);
        str_rem_lf(message, strlen(message));
		printf("%s\n", message);
	}

	bzero(message, 1024);

    while(1){
        
        if (leave_flag) {
			break;
		}
        int receive = recv(client->sockfd, message, 1024, 0); ///Message from client.
        strcpy(temp_mssg,message); ///to avoid distortions, creating temp message and using it.
        str_rem_lf(client->phone_number,11); ///str_lem_lf for deleting empty character in the char[]
        if(str_has_space(temp_mssg,strlen(temp_mssg))>0){ ///it means the client writes a command with another string.
            strcpy(handl_commands,split_message(temp_mssg,1," "));
        } ////Handling message, is it include command or not?
        else{
            strcpy(handl_commands,split_message(temp_mssg,0," "));
        }
        
        if (receive > 0){ ///receive , the client types something or not
            if(strcmp(handl_commands,"-groupname")==0){
                printf("Group name has been taken.\n");
                strcpy(group_->name,split_message(message,0," ")); ///taking group name from the client.
                //and assinging that value to group_->name.

            }
            else if(strcmp(handl_commands,"-groupnameExit")==0){
                char gname[32];  ////if command is exit. look for the second parameter which is groupname.
                strcpy(gname,split_message(message,0," ")); ////split mssg according to space to get name.
                gid=findGroupIdByName(gname);   /// find group id regarding group name.
                if(gid!=0){
                    if(client->groupid!=gid){ ///Check the client was in group or not?
                    send_server_message("You are not already in that group\n",client->uid);
                    }
                    else{ //if yes, operations for exiting that group.
                    str_rem_lf(client->phone_number, 11);
                    sprintf(message, "%s has left the group\n", client->phone_number);
                    send_message(message, client->uid,gid);
                    sprintf(message, "(Server)You have left from %s\n", gname);
                    send_server_message(message,client->uid);
                    printf("%s has exited the group: %s.\n",client->phone_number,gname);
                    client->groupid=0; ////main group id is 0.
                    }
                }
                else{
                    send_server_message("There is no group in that name.\n",client->uid);
                }       
               
            }
            else if(strcmp(handl_commands,"-createpw")==0){
                printf("Group password has been taken.\n");
                strcpy(group_->password,split_message(message,0," "));
                group_->g_uid=group_count; ///assigning values for the new group.
                groups[group_count]=group_;
                group_count++;//and also incre. g.count.
                client->groupid=group_->g_uid;  ///change the group id for the client which is just created the group.
                str_rem_lf(group_->name,11);
                printf("Group %s successfully created.\n",group_->name);
                sprintf(message,"(Server)You were directed to the %s's chat\n",group_->name);
                send_server_message("(Server)Group is successfully created.\n",client->uid);
                send_server_message(message, client->uid);
            }
            else if(strcmp(handl_commands,"-gr")==0){ ///for join operation to a group.
                char gname[32];
                strcpy(gname,split_message(message,0," "));
                gid=findGroupIdByName(gname);
            }
            else if(strcmp(handl_commands,"-jpw")==0){
                char gpassw[32];
                strcpy(gpassw,split_message(message,0," "));
                if(gid!=0){ ///to check the given groupname is exist or not?
                    if(strcmp(gpassw,groups[gid]->password)==0){ // if yes, then update client's group.
                        client->groupid=gid;  
                        printf("Correct password! %s has joined the group: %s.\n",client->phone_number,groups[gid]->name);
                        sprintf(message, "%s has joined the group.\n",client->phone_number);
                        send_message(message, client->uid,client->groupid);
                        sprintf(message,"(Server)You are now in group:%s\n",groups[gid]->name);
                        send_server_message(message,client->uid);  
                    }
                    else{
                    printf("Password was wrong! Join rejected to the group: %s\n",groups[gid]->name);
                    send_server_message("(Server)Wrong password!\n",client->uid);
                    }
                }
                else{
                    send_server_message("(Server)There is no group with that name!\n",client->uid);
                }

            }
            else if(strcmp(handl_commands,"-whoami")==0){
                sprintf(message,"(Server)You are:%s\n",client->phone_number);
                send_server_message(message,client->uid);
            }
            else if(strcmp(handl_commands,"-exit")==0){ //// it means, command is -exit with nothing.

                str_rem_lf(client->phone_number, 11);
                sprintf(handl_commands, "%s has left\n", client->phone_number);
                send_message(handl_commands, client->uid,client->groupid);
			    str_rem_lf(handl_commands, strlen(handl_commands));
                printf("%s\n", handl_commands);
			 
			    leave_flag = 1; //exiting the program.
            }
            else if(message[0]=='@'){
                char send_final[1024];
                char phone_to[11];
                memcpy(phone_to,&message[1],10);
                phone_to[10]='\0';
                memcpy(temp_mssg,&message[11],strlen(message));
                sprintf(send_final,"(Private)%s:%s",client->phone_number,temp_mssg);
                int status=send_private_message(send_final,client->uid,phone_to);
                if(status==0){
                    send_server_message("Check the number\n",client->uid);
                }
            }
            else{
				if(strlen(message) > 0){
                    if(client->groupid==0){
                        sprintf(temp_mssg,"(Main)%s",message);
                    }
                    else{
                        sprintf(temp_mssg,"(%s)%s",groups[client->groupid]->name,message);
                    }
                    
				    send_message(temp_mssg, client->uid,client->groupid);
                    str_rem_lf(client->phone_number, 11);
                    str_rem_lf(message, strlen(message));
                    bzero(temp_mssg, strlen(temp_mssg));
                    strcpy(temp_mssg,split_message(message,1,":"));
                    str_rem_lf(temp_mssg, strlen(temp_mssg));
                    if(client->groupid==0){  
                        printf("\"send\":[{\"to\":\"MAIN\",\"from\":\"%s\",\"message\":\"%s\"}]\n",client->phone_number, temp_mssg);
                    }
                    else{
                        printf("\"send\":[{\"to\":\"%s\",\"from\":\"%s\",\"message\":\"%s\"}]\n",groups[client->groupid]->name,client->phone_number, temp_mssg);
                    }
				  
				}
                
            }


        }
        else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
        bzero(handl_commands, 250);
		bzero(message, strlen(message));
		//bzero(groupName, 32);

    }

 /* Delete client from queue and yield thread */
    close(client->sockfd);
    queue_remove(client->uid);
    free(client);
    client_count--;
    pthread_detach(pthread_self());

	return NULL;

}

// Add clients to queue
void queue_add(client_t *cl){
	for(int i=0; i < MAX_CLIENT; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}	
}

// Remove clients from the queue
void queue_remove(int uid){

	for(int i=0; i < MAX_CLIENT; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}
}


int main(int argc, char **argv){

    
    int server_socket,connection_stat; ///Initialize socket and connection status.
    int option = 1; ////
    server_socket=socket(AF_INET,SOCK_STREAM,0); /// initialize socket.
    
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    pthread_t tid;

    /// socket settings.
    server_address.sin_family=AF_INET;
    server_address.sin_port=htons(PORT);
    server_address.sin_addr.s_addr=INADDR_ANY;

     // Ignoring pipe signals 
	signal(SIGPIPE, SIG_IGN);

    if(setsockopt(server_socket, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("Setsockopt failed");
    return EXIT_FAILURE;
	}

    if(bind(server_socket,(struct sockaddr*)&server_address, sizeof(server_address))<0){
        perror("Socket binding failed.");
        return EXIT_FAILURE;
    }
    if (listen(server_socket, 6) < 0) {
        perror("Socket listening failed");
        return EXIT_FAILURE;
	}
    printf("<<= WELCOME TO THE SERVER =>>\n");
    while(1){
        socklen_t clientlen = sizeof(client_address);
        connection_stat = accept(server_socket, (struct sockaddr*)&client_address,&clientlen);
        
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = client_address;
		client->sockfd = connection_stat;
		client->uid = client_count++;
		client->groupid=0; /// main group 

        // Adding client to the queue and forking the thread 
		queue_add(client);

		pthread_create(&tid, NULL, &handling_client, (void*)client);
		

		sleep(1);
    }
   
    return EXIT_SUCCESS;
}