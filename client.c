#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 3205
int exit_flag=0;
int server_socket;
char phonenumber[32];

int str_has_space (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i]==' ') {
        return 1;
    }
  }
  return 0;
}
void str_rem_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}
char* split_message(char *str,int position){
    
    char *ptr=malloc(1);
    ptr=strtok(str," ");  
    int cnt=0;
    while(cnt<position){
        ptr= strtok(NULL," ");
        cnt++;
    }
    return ptr;
}
void catch_exit(int signal) {
    exit_flag = 1;
}
void sending_message(){

    char message[2048];
    char handl_commands[30];
    char temp_mssg[1024];
    char buffer[2048];
    char command_cont[256];
    while(1) {

       
        fgets(message, 2048, stdin); ///getting input
        
        strcpy(temp_mssg,message);  ///Copying to temp for do some changes on the message. (Keep original)
      
        strcpy(handl_commands,split_message(temp_mssg,0)); // to get which command is entered.   
        str_rem_lf(handl_commands,strlen(handl_commands));
        
        if(strcmp(handl_commands,"-exit")==0){
            if(str_has_space(message,strlen(message))==1){
                strcpy(temp_mssg,message);
                strcpy(command_cont,split_message(temp_mssg,1));
                send(server_socket,strcat(command_cont," -groupnameExit"), strlen(command_cont)+100, 0); ///Sending groupName info to server.   
            }
            else{
                send(server_socket,handl_commands, strlen(handl_commands)+100, 0); 
                break;
            }       
        }
        else if(strcmp(handl_commands,"-gcreate")==0){

            if(str_has_space(message,strlen(message))==0){
                printf("You did not enter a group name\n");
            }
            else{
                strcpy(temp_mssg,message);
                strcpy(command_cont,split_message(temp_mssg,1));///phonenumber+groupname 
                send(server_socket,strcat(command_cont," -groupname"), strlen(command_cont)+100, 0); ///Sending groupName info to server.   
                printf("Enter your password:\n");
                fgets(message, 2048, stdin);   
                send(server_socket,strcat(message," -createpw"), strlen(message)+100, 0); ///Sending pasword info to server.
            }
            
        }
        else if(strcmp(handl_commands,"-join")==0){
            if(str_has_space(message,strlen(message))==0){
                printf("You did not enter a group name\n");
            }
            else{
                strcpy(command_cont,split_message(message,1)); ///keeping groupName
                send(server_socket,strcat(command_cont," -gr"), strlen(command_cont)+100, 0); ///sendind groupname for join to server.
                printf("Enter the group password:\n");
                fgets(message, 2048, stdin);
                send(server_socket,strcat(message," -jpw"), strlen(message)+100, 0); ///sendind groupname for join to server.  
            }
          
        }
        else if(strcmp(handl_commands,"-whoami")==0){
            send(server_socket,handl_commands, strlen(handl_commands)+100, 0);
        }
        else if(message[0]=='@'){
            printf("@ e girdi\n");
            send(server_socket,message, strlen(message)+100, 0);
        }
        else{
            str_rem_lf(phonenumber,strlen(phonenumber));
            str_rem_lf(message,strlen(message));
            sprintf(buffer, "%s: %s\n", phonenumber, message);
            send(server_socket, buffer, strlen(buffer), 0);
        }
        ////Cleaning buffers.    
        bzero(command_cont, 256);
    	bzero(message, 2048);
        bzero(handl_commands,30);
        bzero(temp_mssg, 1024);
        bzero(buffer, 2048);
    }
    catch_exit(2);
}
void receiving_message(){

    char message[1024];

    while (1) {
		int receive = recv(server_socket, message, 1024, 0);
        if (receive > 0) {
            printf("%s", message);
        }
        else if (receive == 0) {
			break;
        }
		memset(message, 0, sizeof(message));
    }
}

int main(int argc, char **argv){

    signal(SIGINT, catch_exit);

    struct sockaddr_in server_address;
    printf("Please enter your phone number(exp:533444555): ");
    fgets(phonenumber, 12, stdin);
    str_rem_lf(phonenumber, strlen(phonenumber));

	if (strlen(phonenumber) > 10 || strlen(phonenumber) < 10){
		printf("Phone Number must be 10 character\n");
		return EXIT_FAILURE;
	}
   

	/* Socket settings */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    //

     // Connecting to the Server
    int err = connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}
    send(server_socket, phonenumber,10, 0);
    printf("<<= WELCOME TO THE MAIN ROOM =>>\n");
    printf("<--Commands-->\n(-gcreate) helps for creating your own group. Usage= -gcreate groupname\n(-join) helps for joining an existent group. Usage= -join groupname\n");
    printf("(-whoami) Shows your own phone number information.\n(-exit) for exiting from a group or the main program. Usage=-exit or -exit groupname\nOtherwise,what you wrote goes a message to the group you are in.(Default:main group)\n");
    printf("(@) sends a private message to given client's phone number. Usage=@phone_number yourmessage (example: @5456172148 hey)\n");
    //threads for sending and receiving data.
    pthread_t sending_mssg_t,receiving_mssg_t;

    if(pthread_create(&sending_mssg_t, NULL, (void *) sending_message, NULL) != 0){
		printf("Pthread error has been occurred\n");
        return EXIT_FAILURE;
	}
    if(pthread_create(&receiving_mssg_t, NULL, (void *) receiving_message, NULL) != 0){
		printf("Pthread error has been occurred\n");
		return EXIT_FAILURE;
	}
    while(1){
        if(exit_flag){
            printf("\nBye!\n");
            break;
        }
    }


    close(server_socket);

	return EXIT_SUCCESS;

}