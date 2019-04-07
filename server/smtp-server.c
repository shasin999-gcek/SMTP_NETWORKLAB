#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TRUE 1
#define FALSE 1
#define USERS_DAT_FILE "users.dat"
#define MAILS_DAT_FILE "mails.dat"

struct SMTP_AUTH_CRED {
  char email[50];
  char password[50];
};

struct USER {
  char name[50];
  char email[50];
  char password[50];
};

struct STATUS_MSG {
  int status_code;
  char status_msg[50];
};

struct MAIL {
  char from[50];
  char to[50];
  char subject[50];
  char body[100];
};

int login(int client_sockfd, struct SMTP_AUTH_CRED *, struct USER *);
int create_user_and_login(int client_sockfd, struct USER *);
void send_status(int client_sockfd, int status_code, char *status_msg);

int main(int argc, char *argv[])
{
  struct  sockaddr_in server_addr, client_addr;
  struct  SMTP_AUTH_CRED auth_cred;
  struct  USER user;
  int     sockfd, client_sockfd, port, client_len, option, is_logged_in;
  FILE    *fileptr;

  if(argc != 3) {
    fprintf(stderr, "Invalid Arguments\n");
    fprintf(stderr, "./smtp-server -p <port number>\n");
    return -1;
  }

  port = atoi(argv[2]); // assign port number
  client_len = sizeof(client_addr);

  // create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("socket");
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if ( bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
    perror("bind");
    return -1;
  }
  
  if ( listen(sockfd, 5) < 0) {
    perror("listen");
    return -1;
  }
  
  while(TRUE) {
  
    if ( (client_sockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
      perror("accept");
      continue;
    }
    
    /*
      option: 
        1 LOGIN
        2 CREATE ACCOUNT
    */

    recv(client_sockfd, &option, sizeof(option), 0);

    switch (option)
    {
      case 1:
        is_logged_in = login(client_sockfd, &auth_cred, &user);
        break;
    
      case 2:
        is_logged_in = create_user_and_login(client_sockfd, &user);
        break;

      default:
        break;
    }

    if(is_logged_in == FALSE)
      break; 

    while (TRUE)
    {
      /*
        option: 
          1 COMPOSE
          2 INBOX
          3 LOGOUT
      */

      // receive option form client
      recv(sockfd, &option, sizeof(option), 0);

      switch (option)
      {
        case 1:
          store_mail(client_sockfd);
          break;
      
        case 2:
          send_inbox(client_sockfd);
          break;

        case 3:
          break;

        default:
          printf("Please choose an option !!\n");
          break;
      }

      if(option == 3) break;
    }  

    close(client_sockfd); 
  }
  
  close(sockfd);
}

int login(int client_sockfd,  struct SMTP_AUTH_CRED *auth_cred, struct USER *user)
{
  FILE   *users_dat;

  recv(client_sockfd, auth_cred, sizeof(*auth_cred), 0);

  users_dat = fopen(USERS_DAT_FILE, "rb");
  while (!feof(users_dat))
  {
    fread(user, sizeof(struct USER), 1, users_dat);
   
    if ( strcmp(user->email, auth_cred->email) == 0)
    {
      if ( strcmp(user->password, auth_cred->password) == 0)
      {
        send_status(client_sockfd, 200, "Logged In Successfully");
        fclose(users_dat);
        return TRUE;
      }
      else
      {
        send_status(client_sockfd, 403, "Password Is Incorrect");
        fclose(users_dat);
        return FALSE;
      }
    }
  
  }

  send_status(client_sockfd, 403, "Invalid Email And Password");
  fclose(users_dat);
  return FALSE;
}

int create_user_and_login(int client_sockfd, struct USER *user)
{
  FILE   *users_dat;
  
  recv(client_sockfd, user, sizeof(*user), 0);
  
  users_dat = fopen(USERS_DAT_FILE, "ab+");
  if(users_dat == NULL)
  {
    send_status(client_sockfd, 500, "Internal Server Error");
    fclose(users_dat);
    return FALSE;
  } 
 
  fwrite(user, sizeof(struct USER), 1, users_dat);
  send_status(client_sockfd, 200, "OK");
  fclose(users_dat);
  return TRUE;  
  
}


void send_status(int client_sockfd, int status_code, char *status_msg)
{
  struct STATUS_MSG status;

  status.status_code = status_code;
  strcpy(status.status_msg, status_msg);
  send(client_sockfd, &status, sizeof(status), 0);
}