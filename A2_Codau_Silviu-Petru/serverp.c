#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

struct client
{
    int socket;
    char name[20];
    int in_conv;
    bool online;
};

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mlock_fis_conv_off = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mlock_fis_istoric = PTHREAD_MUTEX_INITIALIZER;

FILE* mesaje_primite_offline;
FILE* istoric_mesaje;


void citim_Data(int socketFD);

int citim_nume_sau_iesire(int socketFD);

void mesaje_prim_offline(int socketFD,int client_poz1);

void scriem_mesajele_offline (FILE* fis,const char* nume_fis, char buffer[1024],int client_poz1);

void scriem_mesajele_istoric (FILE* fis,const char* nume_fis, char buffer[1024],int client_poz1);

void seehistory (const char * dest, int client_poz1,int socketFD);


struct client clienti[50];
int clientCount = 0;


int main() 
{
	
    mesaje_primite_offline=fopen("mesaje_primote_offline.txt","w");
    fclose(mesaje_primite_offline);
    istoric_mesaje=fopen("istoric_mesaje.txt","w");
    fclose(istoric_mesaje);
    
    
    int serverSocketFD; 
    if ((serverSocketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
    	perror("[server]Eroare la socket().\n");
    	return errno;
    }
    struct sockaddr_in  clientAddress ;
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.s_addr = htonl (INADDR_ANY);
    serverAddress.sin_port = htons (2049);

    
    if (bind(serverSocketFD, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr)) == -1)
    {
   	 perror("[server]Eroare la bind().\n");
  	 return errno;
    }
    else
        printf("socket bind cu succes\n");
	
    if (listen(serverSocketFD, 2) == -1)
    {
   	 perror("[server]Eroare la listen().\n");
    	return errno;
    }

    while(1)
    {   	
    	
    	int clientAddressDim = sizeof (struct sockaddr_in);
    	int clientSocketFD;
    	bzero(&clientAddress, sizeof(clientAddress));
    	if ((clientSocketFD = accept(serverSocketFD, &clientAddress, &clientAddressDim)) < 0)
        {
         	perror("[server]Eroare la accept().\n");
         	continue;
        }
    	
     
        pthread_t id;
    	pthread_create(&id,NULL,citim_Data,clientSocketFD);
    }

    //fclose(mesaje_primite_offline);
    close(serverSocketFD);
    
    
    return 0;
}



void citim_Data(int socketFD) 
{
    char buffer[1024];

    int client_poz1=citim_nume_sau_iesire(socketFD);
    if(client_poz1!=-1)//#exit
    {
    send(socketFD,"conectat\n", strlen("conectat\n"),0);
    
    mesaje_prim_offline(socketFD,client_poz1);
    
    while (true)
    {
    	bzero(buffer,1024);
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);
        bool am_folosit_comanda=false;

        if(amountReceived>0)
        {
            buffer[amountReceived] = '\0';
            
            if(strcmp(buffer,"#exit")==0)
            {	
            	am_folosit_comanda=true;
            	clienti[client_poz1].online=false;
            	clienti[client_poz1].in_conv=-1;
            	send(socketFD,"la revedere\n", strlen("la revedere\n"),0);
            	break;
            }
            
            if(strcmp(buffer,"#logout")==0)
            {	
            	am_folosit_comanda=true;
            	clienti[client_poz1].online=false;
            	clienti[client_poz1].in_conv=-1;
            	send(socketFD,"introduceti numele\n", strlen("introduceti numele\n"),0);
            	client_poz1=citim_nume_sau_iesire(socketFD);
            	if(client_poz1!=-1)//a iesit in timp ce trebuia sa introduca numele (client_poz1==-1)#exit
            	{
            		send(socketFD,"conectat\n", strlen("conectat\n"),0);
            		mesaje_prim_offline(socketFD,client_poz1);
            	}
            	///*else
            		///*break;
            }
            
            if(strcmp(buffer,"#exitconv")==0)
            {
            	am_folosit_comanda=true;
            	clienti[client_poz1].in_conv=-1;
            }
            
            
            if(strncmp(buffer,"#seehistory: ",strlen("#seehistory: "))==0)
            {
            	am_folosit_comanda=true;
            	char dest[20];
            	
            	strcpy(dest,buffer+(strlen("#seehistory: ")));
            	bool exista_utilizator=false;
            	
            	for(int i = 0 ; i<clientCount ; i++)
            	{
            		if(strcmp(clienti[i].name,dest )==0)
        		{
        			exista_utilizator=true;
        		}
            	}
            	
            	if(exista_utilizator==true)
            	{
            		pthread_mutex_lock(&mlock_fis_istoric);
            		
            		seehistory(dest,client_poz1,socketFD);
            		
            		pthread_mutex_unlock(&mlock_fis_istoric);
            	}
            	else
            	{
            		strcpy(buffer,"nu exista utilizator cu acest nume\n");
        		send(socketFD,buffer, strlen(buffer),0);
            	}
            	
            }
            
            if(strncmp(buffer,"#sendto: ",strlen("#sendto: "))==0)
            {
            	am_folosit_comanda=true;
            	 char dest[20];
            	strcpy(dest,buffer+(strlen("#sendto: ")));
            	
            	for(int i = 0 ; i<clientCount ; i++)
            	{
            		
        		if(strcmp(clienti[i].name,dest )==0)
        		{
        			//printf("1");
            			clienti[client_poz1].in_conv=i;
        		}
        	}
        	if(clienti[client_poz1].in_conv==-1)
        	{
        		strcpy(buffer,"nu exista destinatar cu acest nume\n");
        		send(socketFD,buffer, strlen(buffer),0);
        	}
        		
            }
            
            if(strncmp(buffer,"#reply: ",strlen("#reply: "))==0)
            {
            	//am_folosit_comanda=true;
            	char dest[20];
            	char nr_msg[20];
            	//int nr_mesaj=0;
            	int poz_j=0;
            	for(int i=strlen("#reply: "); i<strlen(buffer); i++)
            	{
            		if(buffer[i]=='\n')
            			break;
            		dest[poz_j]=buffer[i];
            		poz_j++;
            	}

            	dest[poz_j]='\0';
     	
            	//ca la seehistory
            	bool exista_utilizator=false;
            	
            	for(int i = 0 ; i<clientCount ; i++)
            	{
            		if(strcmp(clienti[i].name,dest )==0)
        		{
        			exista_utilizator=true;
        		}
            	}
            	
            	if(exista_utilizator==true)
            	{
            		pthread_mutex_lock(&mlock_fis_istoric);
            		
            		seehistory(dest,client_poz1,socketFD);
            		
            		pthread_mutex_unlock(&mlock_fis_istoric);
            		
            		//adaugari
            		char sfat[]="introduceti un numar.\n";
        		send(socketFD,sfat, strlen(sfat),0);
        		//bzero(buffer,0);
        		char numar_str[20];
        		int linia_selectata=0;
            		recv(socketFD,numar_str,20,0);//avem numarul
            		//sprintf (linie,"%d",numar_str);
            		
            		linia_selectata=atoi(numar_str);
            		//printf("linia este: %d \n" , linia_selectata);
            		
            		pthread_mutex_lock(&mlock_fis_istoric);
            		int line=0;
			istoric_mesaje=fopen("istoric_mesaje.txt","r");
    			fseek(istoric_mesaje,0,SEEK_SET);
            		char buf_fis[1024];
            		if(line!=linia_selectata)
            		{
            		while(fgets(buf_fis,sizeof(buf_fis),istoric_mesaje)!=NULL)
    			{	
    				if(strlen(buf_fis)>2 && buf_fis[strlen(buf_fis)-2]=='#' && buf_fis[strlen(buf_fis)-3]=='#')//strlen(buf_fis)-1='\n'
    				{
    					char nume2[20];
               			strcpy(nume2,buf_fis);
               			nume2[strlen(nume2)-3]='\0';
               			if(strcmp(nume2,clienti[client_poz1].name)==0) 
               			{
               				bzero(buf_fis,0);
               				fgets(buf_fis,sizeof(buf_fis),istoric_mesaje);
               				char nume3[20];
               				for(int i=0;i<strlen(buf_fis);i++)
               				{
               					if(buf_fis[i]!=':')
               					{
               						nume3[i]=buf_fis[i];
               					}
               					else
               					{
               						nume3[i]='\0';
               						break;
               					}
               				}
               				if(strcmp(nume3,dest)==0 )
               				{
               					line++;
               					if( line==linia_selectata)
               						break;
               				}
               			}
               			
               			
               			if( strcmp(nume2,dest)==0)
               			{
               				bzero(buf_fis,0);
               				fgets(buf_fis,sizeof(buf_fis),istoric_mesaje);
               				char nume3[20];
               				for(int i=0;i<strlen(buf_fis);i++)
               				{
               					if(buf_fis[i]!=':')
               					{
               						nume3[i]=buf_fis[i];
               					}
               					else
               					{
               						nume3[i]='\0';
               						break;
               					}
               				}
               				if(strcmp(nume3,clienti[client_poz1].name)==0 )
               				{
               					line++;
               					//send(socketFD,de_trim1, strlen(de_trim1),0);
               					
               					if( line==linia_selectata)
               						break;
               					
               				}
               			}
               			
        			}

        			bzero(buf_fis,0);
   			 }	
   			 
   			 }	
   			 fclose(istoric_mesaje);
   			 pthread_mutex_unlock(&mlock_fis_istoric);
   			 
   			 		
            		if(linia_selectata>line || linia_selectata==0 )
            		{
            			am_folosit_comanda=true;
            			char sfat[]="numar incorect.\n";
            			//return sfat;
        			send(socketFD,sfat, strlen(sfat),0);
            		}
            		if(linia_selectata==line&& linia_selectata!=0)
            		{
            			char de_trim1[1024];
               		//strcpy(de_trim1,clienti[client_poz1].name);
               		strcpy(de_trim1,"raspunde la mesajul (");
               		strcat(de_trim1,buf_fis);
               		de_trim1[strlen(de_trim1)-1]='\0';
               		strcat(de_trim1,") cu: ");
               		
            			send(socketFD,"introduceti mesajul de raspuns\n", strlen("introduceti mesajul de raspuns\n"),0);
            			char ce_rasp[500];
            			int dim_prim=recv(socketFD,ce_rasp,500,0);
            			
            			//ce_rasp[dim_prim]='\n';
            			ce_rasp[dim_prim]='\0';
               		
               		//printf("%s", ce_rasp);
               		strcat(de_trim1,ce_rasp);
               		
               		
				//printf("%s", de_trim1);
				bzero(buffer,0);
				strcpy(buffer,de_trim1);
               		//send(socketFD,de_trim1, strlen(de_trim1),0);
            		}
            		
            		
            		
            	}
            	else
            	{
            		am_folosit_comanda=true;
            		strcpy(buffer,"nu exista utilizator cu acest nume\n");
        		send(socketFD,buffer, strlen(buffer),0);
            	}
            	//pana aici ca la seehistory
            	
            }
            
            if(strncmp(buffer,"#seeusers",strlen("#seeusers"))==0)
            {
            	am_folosit_comanda=true;
            	
            	char utilizatori[1024];
            	strcpy(utilizatori,clienti[0].name);

            	for(int i=1;i<clientCount;i++)
            	{
            		strcat(utilizatori,", ");
            		strcat(utilizatori,clienti[i].name);
            	}
            	
            	strcat(utilizatori,"\n");
            	send(socketFD,utilizatori, strlen(utilizatori),0);
            }
            
            if(clienti[client_poz1].in_conv!=-1 && am_folosit_comanda==false) // && strchr(buffer,'#')==NULL)
            {
            	if(clienti[clienti[client_poz1].in_conv].online==true)
            	{
            		char trim[1024];
            		strcpy(trim,clienti[client_poz1].name);
            		strcat(trim,": ");
            		strcat(trim,buffer);
            		strcat(trim,"\n");
            		send(clienti[clienti[client_poz1].in_conv].socket,trim, strlen(trim),0);
            		
            		pthread_mutex_lock(&mlock_fis_istoric);

            		scriem_mesajele_istoric ( istoric_mesaje,"istoric_mesaje.txt",  buffer, client_poz1);
            		
            		pthread_mutex_unlock(&mlock_fis_istoric);
            		
            	}
            	else
            	{
            	      pthread_mutex_lock(&mlock_fis_conv_off);
            	      
            	      scriem_mesajele_offline ( mesaje_primite_offline,"mesaje_primote_offline.txt",  buffer, client_poz1);
            	      
            	      pthread_mutex_unlock(&mlock_fis_conv_off);
            	      
            	      pthread_mutex_lock(&mlock_fis_istoric);

            	      scriem_mesajele_istoric ( istoric_mesaje,"istoric_mesaje.txt",  buffer, client_poz1);
            		
            	      pthread_mutex_unlock(&mlock_fis_istoric);
            	      
            	}
            	
            }
            
        }
            

        if(amountReceived==0)
            break;
    }
    }
    else
    {
    	clienti[client_poz1].online=false;
        clienti[client_poz1].in_conv=-1;
        send(socketFD,"la revedere\n", strlen("la revedere\n"),0);
    }
    close(socketFD);
}

int citim_nume_sau_iesire(int socketFD)
{
    int client_poz1;
    char buffer[1024];
    while(true)
    {
    	bzero(buffer,1024);
        ssize_t  amountReceived1 = recv(socketFD,buffer,1024,0);
        buffer[amountReceived1] = '\0'; 
        if(strcmp(buffer,"#exit")==0)
        {	
            return -1;
        }
        if(strchr(buffer,'#')==NULL)
        {
    		//loock
    		pthread_mutex_lock(&mlock);
    		int poz_client;
    		bool ok=false;
    		for(poz_client=0 ; poz_client < clientCount ; poz_client++)
    		{
          		if(strcmp(clienti[poz_client].name,buffer)==0 )
          		{
          			if(clienti[poz_client].online==false)
          			{
            				clienti[poz_client].online=true;
            				//clienti[poz_client].exit=false;
            				clienti[poz_client].socket=socketFD;
            				clienti[poz_client].in_conv=-1;
            				ok=true;
            				client_poz1=poz_client;
            				break;
            			}
            			else
            			{
            				char trim[1024];
            				strcpy(trim,"utilizator deja conectat\n");
            				send(socketFD,trim, strlen(trim),0);
            				break;
            			}
          		}
    		}
    		if(poz_client==clientCount)
   		 {
    			clienti[poz_client].online=true;
        		//clienti[poz_client].exit=false;
        		clienti[poz_client].socket=socketFD;
        		clienti[poz_client].in_conv=-1;
        		strcpy(clienti[poz_client].name,buffer);
        		clientCount++;
        		client_poz1=poz_client;
        		pthread_mutex_unlock(&mlock);
        		break;
    		}
    		else
    		{
    			if(ok==true)
    			{
    				pthread_mutex_unlock(&mlock);
    				break;
    			}	
    		}
    
    	pthread_mutex_unlock(&mlock);
    }
    else
    {
    	send(socketFD,"numele nu poate contine #\n", strlen("numele nu poate contine #\n"),0);
    }
    //unloock
    }
    return client_poz1;
}




void mesaje_prim_offline(int socketFD,int client_poz1)
{
    send(socketFD,"mesajele primite:\n", strlen("mesajele primite:\n"),0);
    
    pthread_mutex_lock(&mlock_fis_conv_off);
    
    mesaje_primite_offline=fopen("mesaje_primote_offline.txt","r");
    fseek(mesaje_primite_offline,0,SEEK_SET);
    
    FILE* aux1;
    aux1=fopen("aux1.txt","w");
    fseek(aux1,0,SEEK_SET);
    char buf_fis[1024];
    bool nume_gasit=false;
    
    //printf("d\n");
    while(fgets(buf_fis,sizeof(buf_fis),mesaje_primite_offline)!=NULL)
    {
       //printf("b\n");
    	if(strlen(buf_fis)>2 && buf_fis[strlen(buf_fis)-2]=='#' && buf_fis[strlen(buf_fis)-3]=='#')//strlen(buf_fis)-1='\n'
    	{
    	        //printf("b\n");
    		char nume2[20];
               strcpy(nume2,buf_fis);
               nume2[strlen(nume2)-3]='\0';
            		  	
               if(strcmp(nume2,clienti[client_poz1].name)==0)
               {
               	//printf("a\n");
               	fgets(buf_fis,sizeof(buf_fis),mesaje_primite_offline);
               	nume_gasit=true;
               }
               else
               {
               	nume_gasit=false;
               }
        }
        
        if(nume_gasit==true)
        {
        	//printf("da");
        	send(socketFD,buf_fis, strlen(buf_fis),0);      	
        }
        else
        {
        	fwrite(buf_fis,1,strlen(buf_fis),aux1);
        }
        
        			
        bzero(buf_fis,0);
    }	
    
    fclose(mesaje_primite_offline);
    fclose(aux1);
    mesaje_primite_offline=fopen("mesaje_primote_offline.txt","w");
    fseek(mesaje_primite_offline,0,SEEK_SET);
    aux1=fopen("aux1.txt","r");
    
    while(fgets(buf_fis,sizeof(buf_fis),aux1)!=NULL)
    {
    	fwrite(buf_fis,1,strlen(buf_fis),mesaje_primite_offline);
            		  			
        bzero(buf_fis,0);
    }
    fclose(aux1);
    remove("aux1.txt");
    fclose(mesaje_primite_offline);
    pthread_mutex_unlock(&mlock_fis_conv_off);
}




void scriem_mesajele_offline (FILE* fis,const char* nume_fis, char buffer[1024],int client_poz1)
{
	//pthread_mutex_lock(&mlock_fis_conv_off);
        FILE* aux;
        fis=fopen(nume_fis,"r+");
        fseek(fis,0,SEEK_SET);
        //printf("1\n");
         char mesaj[1024];
         int line=0;
         bool am_ajuns_la_final=true;
            		
         while(fgets(mesaj,sizeof(mesaj),fis)!=NULL)
         {
               line++;
            	mesaj[strlen(mesaj)-1]='\0';
            	if(strlen(mesaj)>2 && mesaj[strlen(mesaj)-1]=='#' && mesaj[strlen(mesaj)-2]=='#')
            	{
            		char nume1[20];
            		strcpy(nume1,mesaj);
            		nume1[strlen(nume1)-2]='\0';
            		  	
            		if(strcmp(nume1,clienti[clienti[client_poz1].in_conv].name)==0)
            		{
            		  		
            		  		aux=fopen("aux.txt","w");
            		  		char de_trim_fis[1024];
            		  		
            		  		strcpy(de_trim_fis,clienti[client_poz1].name);
            		  		strcat(de_trim_fis,": ");
            		  		strcat(de_trim_fis,buffer);
            		  		strcat(de_trim_fis,"\n");
            		  		//printf("%s",de_trim_fis);
            		  		
            		  		//copiem tot de dupa numele gasit intr-un alt fisier apoi il copiem in fis initial
            		  		
            		  		char ce_trim[1024];
            		  		strcpy(ce_trim,de_trim_fis);
            		  		
            		  		bzero(de_trim_fis,0);
            		  		
            		  		while(fgets(de_trim_fis,sizeof(de_trim_fis),fis)!=NULL)
            		  		{
            		  			if(strlen(de_trim_fis)>2 && de_trim_fis[strlen(de_trim_fis)-2]=='#' && de_trim_fis[strlen(de_trim_fis)-3]=='#')
            		  			{
            		  				break;
            		  			}
            		  			
            		  			fwrite(de_trim_fis,1,strlen(de_trim_fis),aux);
            		  			
            		  			bzero(de_trim_fis,0);
            		  		}
            		  		
            		  		fwrite(ce_trim,1,strlen(ce_trim),aux);//copiem mesajul scris
            		  		
            		  		while(fgets(de_trim_fis,sizeof(de_trim_fis),fis)!=NULL)
            		  		{
            		  		
            		  			fwrite(de_trim_fis,1,strlen(de_trim_fis),aux);
            		  			
            		  			bzero(de_trim_fis,0);
            		  		}
            		  		
            		  		
            		  		fclose(aux);
            		  		
            		  		fseek(fis,0,SEEK_SET);
            		  		int line1=0;
            		  		while(fgets(de_trim_fis,sizeof(de_trim_fis),fis)!=NULL)
            		  		{
            		  		        bzero(de_trim_fis,0);
            		  			line1++;
            		  			if(line1==line)
            		  				break;
            		  			
            		  		}
            		  		
            		  		aux=fopen("aux.txt","r");
            		  		
            		  		while(fgets(de_trim_fis,sizeof(de_trim_fis),aux)!=NULL)
            		  		{
            		  			fwrite(de_trim_fis,1,strlen(de_trim_fis),fis);
            		  			
            		  			bzero(de_trim_fis,0);
            		  		}
            		  		fclose(aux);
            		  		remove("aux.txt");
            		  		
            		  		am_ajuns_la_final=false;
            		  		break;	
            		  }
            		  	            		  	
            	}
          }
            		
          if(am_ajuns_la_final==true)
          {
               //printf("2\n");
            	char de_trim_fis[1024];
               fseek(fis,0,SEEK_END);              
               strcpy(de_trim_fis,clienti[clienti[client_poz1].in_conv].name);
               strcat(de_trim_fis,"##\n");
               strcat(de_trim_fis,clienti[client_poz1].name);
               strcat(de_trim_fis,": ");
               strcat(de_trim_fis,buffer);
            	strcat(de_trim_fis,"\n");
            		  	
            		  	
            	fwrite(de_trim_fis,1,strlen(de_trim_fis),fis);
          }
            		    
          fclose(fis);
          //pthread_mutex_unlock(&mlock_fis_conv_off);


}

void scriem_mesajele_istoric (FILE* fis,const char* nume_fis, char buffer[1024],int client_poz1)
{
		
       	 fis=fopen(nume_fis,"r+");
       	 fseek(fis,0,SEEK_SET);

		char de_trim_fis[1024];
               fseek(fis,0,SEEK_END);              
               strcpy(de_trim_fis,clienti[clienti[client_poz1].in_conv].name);
               strcat(de_trim_fis,"##\n");
               strcat(de_trim_fis,clienti[client_poz1].name);
               strcat(de_trim_fis,": ");
               strcat(de_trim_fis,buffer);
            	strcat(de_trim_fis,"\n");
            		  	
            		  	
            	fwrite(de_trim_fis,1,strlen(de_trim_fis),fis);
            	
            	
            	fclose(fis);
	
}

void seehistory (const char * dest, int client_poz1,int socketFD)
{
			int line=0;
			istoric_mesaje=fopen("istoric_mesaje.txt","r");
    			fseek(istoric_mesaje,0,SEEK_SET);
            		char buf_fis[1024];
			
            		while(fgets(buf_fis,sizeof(buf_fis),istoric_mesaje)!=NULL)
    			{
      			 //printf("b\n");
       			
    				if(strlen(buf_fis)>2 && buf_fis[strlen(buf_fis)-2]=='#' && buf_fis[strlen(buf_fis)-3]=='#')//strlen(buf_fis)-1='\n'
    				{
    	        		//printf("b\n"); 
    					char nume2[20];
               			strcpy(nume2,buf_fis);
               			nume2[strlen(nume2)-3]='\0';
               			if(strcmp(nume2,clienti[client_poz1].name)==0) 
               			{
               			//printf("a\n");
               				bzero(buf_fis,0);
               				fgets(buf_fis,sizeof(buf_fis),istoric_mesaje);
               				char nume3[20];
               				for(int i=0;i<strlen(buf_fis);i++)
               				{
               					if(buf_fis[i]!=':')
               					{
               						nume3[i]=buf_fis[i];
               					}
               					else
               					{
               						nume3[i]='\0';
               						break;
               					}
               				}
               				if(strcmp(nume3,dest)==0)
               				{
               					line++;
               					char linie[20];
               					sprintf (linie,"%d",line);
               					char de_trim1[1024];
               					strcpy(de_trim1,linie);
               					strcat(de_trim1,". ");
               					strcat(de_trim1,buf_fis);
               					
               					send(socketFD,de_trim1, strlen(de_trim1),0);
               					//printf("%s",buf_fis);
               				}
               			}
               			
               			
               			if( strcmp(nume2,dest)==0)
               			{
               				bzero(buf_fis,0);
               				fgets(buf_fis,sizeof(buf_fis),istoric_mesaje);
               				char nume3[20];
               				for(int i=0;i<strlen(buf_fis);i++)
               				{
               					if(buf_fis[i]!=':')
               					{
               						nume3[i]=buf_fis[i];
               					}
               					else
               					{
               						nume3[i]='\0';
               						break;
               					}
               				}
               				if(strcmp(nume3,clienti[client_poz1].name)==0)
               				{
               					line++;
               					char linie[20];
               					sprintf (linie,"%d",line);
               					char de_trim1[1024];
               					strcpy(de_trim1,linie);
               					strcat(de_trim1,". ");
               					strcat(de_trim1,buf_fis);
               					
               					send(socketFD,de_trim1, strlen(de_trim1),0);
               				}
               			}
               			
        			}
        			
        			
        			bzero(buf_fis,0);
   			 }				
            		
            		fclose(istoric_mesaje);
}



