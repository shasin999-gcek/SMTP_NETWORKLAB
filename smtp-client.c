#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TRUE 1
#define FALSE 0

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


void login_or_create(int *option);
void get_credintials(struct SMTP_AUTH_CRED *);
void send_cred_to_server(int sockfd, struct SMTP_AUTH_CRED *);
void get_user_info(struct USER *);
void send_user_info(int sockfd, struct USER *);
int  status(int sockfd);
void show_main_menu(int *option);
void compose_mail(int sockfd);
void show_inbox(int sockfd);

int main(int argc, char *argv[])
{
  struct  sockaddr_in server_addr;
  struct  SMTP_AUTH_CRED auth_cred;
  struct  USER user;
  int     sockfd, port, option, is_logged_in;


  if(argc != 3) 
  {
    fprintf(stderr, "Invalid Arguments\n");
    fprintf(stderr, "./smtp-server -p <port number>\n");
    return -1;
  }

  port = atoi(argv[2]); // assign port number

  // create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) 
  {
    perror("socket");
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if ( connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 )
  {
    perror("connect");
    return -1;
  }
  
  while(TRUE) 
  {
    
    /*
      option: 
        1 LOGIN
        2 CREATE ACCOUNT
    */

    login_or_create(&option);

    send(sockfd, &option, sizeof(option), 0);

    switch (option)
    {
      case 1:
        get_credintials(&auth_cred);
        send_cred_to_server(sockfd, &auth_cred);
        is_logged_in = status(sockfd);
        break;
    
      case 2:
        get_user_info(&user);
        send_user_info(sockfd, &user);
        is_logged_in = status(sockfd);
        break;

      default:
        printf("Please choose an option !!\n");
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

      show_main_menu(&option);

      // send option to server
      send(sockfd, &option, sizeof(option), 0);

      switch (option)
      {
        case 1:
          compose_mail(sockfd);
          status(sockfd);
          break;
      
        case 2:
          show_inbox(sockfd);
          break;

        default:
          printf("Please choose an option !!\n");
          break;
      }

      if(option == 3) break;
    }  

  }

  close(sockfd);    
  return 1;
}

void login_or_create(int *option)
{
  printf("_______________________START MENU_____________________\n");
  printf("1. LOGIN \n");
  printf("2. CREATE ACCOUNT \n");
  printf("Choose an option => ");
  scanf("%d", option);
}

void get_credintials(struct SMTP_AUTH_CRED *auth_cred)
{
  printf("EMAIL => ");
  scanf("%s", auth_cred->email);
  printf("PASSWORD => ");
  scanf("%s", auth_cred->password);
}

void send_cred_to_server(int sockfd, struct SMTP_AUTH_CRED *auth_cred)
{
  send(sockfd, auth_cred, sizeof(*auth_cred), 0);
}

void get_user_info(struct USER *user)
{
  printf("ENTER USER NAME => ");
  scanf("%s", user->name);
  printf("ENTER EMAIL ADDRESS => ");
  scanf("%s", user->email);
  printf("ENTER PASSWORD => ");
  scanf("%s", user->password);
}

void send_user_info(int sockfd, struct USER *user)
{
  send(sockfd, user, sizeof(*user), 0);
}

int status(int sockfd)
{
  struct STATUS_MSG status;
  recv(sockfd, &status, sizeof(status), 0);

  printf("%s", status.status_msg);

  if(status.status_code == 500) {
     return FALSE;
  }  
   
  return TRUE;
}

void show_main_menu(int *option)
{
  printf("____________________MAIN MENU_________________\n");
  printf("1. COMPOSE\n");
  printf("2. INBOX\n");
  printf("3. LOGOUT\n");
  printf("CHOOSE OPTION => ");
  scanf("%d", option);
}

void compose_mail(int sockfd)
{
  struct MAIL mail;

  printf("____________________COMPOSE MAIL_________________\n");
  printf("FROM => ");
  scanf("%s", mail.from);
  printf("TO => ");
  scanf("%s", mail.to);
  printf("SUBJECT => ");
  scanf("%s", mail.subject);
  printf("BODY => ");
  fgets(mail.body, sizeof(mail.body), stdin);

  // send mail to server
  send(sockfd, &mail, sizeof(mail), 0);

  // waiting for server to respond with a status
  status(sockfd);

}
