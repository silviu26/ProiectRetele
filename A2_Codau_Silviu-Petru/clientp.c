#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


void citim_terminal_tremitem_la_server(int socketFD);
void ce_primim_de_la_socket_printam(int socketFD);

int main() 
{

    int socketFD;
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }
  
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr=inet_addr("127.0.0.1");
    address.sin_port = htons(2049);

    if (connect(socketFD, (struct sockaddr *)&address, sizeof(struct sockaddr)) == -1)
    {
    	perror("[client]Eroare la connect().\n");
    	return errno;
    }


    pthread_t id ;
    pthread_create(&id,NULL,(void*)ce_primim_de_la_socket_printam,socketFD);

    citim_terminal_tremitem_la_server(socketFD);


    close(socketFD);

    return 0;
}


void citim_terminal_tremitem_la_server(int socketFD) 
{

    char *name = NULL;
    int nameSize= 0;
    printf("introduceti numele\n");
    int  dim = getline(&name,&nameSize,stdin);
    name[dim-1]='\0';
    //printf("%s,%d",name,strlen(name));
    int dim1 =  send(socketFD,name,strlen(name), 0);

    char *line = NULL;
    int dim_line= 0;
    //printf("acum poti scrie\n");


    char buffer[1024];

    while(true)
    {
        int  dim_char = getline(&line,&dim_line,stdin);
        line[dim_char-1]='\0';

        if(dim_char>1)// '\n' este mereu
        {
            int dim =  send(socketFD,line,strlen(line), 0);
            
            if(strcmp(line,"#exit")==0)
                break;
            
        }
    }
}

void ce_primim_de_la_socket_printam(int socketFD) 
{
    char buffer[1024];

    while (true)
    {
        int  dim = recv(socketFD,buffer,1024,0);

        if(dim>0)
        {
            buffer[dim] = '\0';
            printf("%s",buffer);
        }

        if(dim==0)
            break;
    }

    close(socketFD);
}