#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFLEN 256
#define MAX_CLIENTS 10
#define MAXLEN 500

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void getHost(char* command,char* host){   //functie care ne extrage adresa de host

    char* aux=command;
    aux=aux+7;
    strcpy(host,strtok(aux,"/"));
    
    

}

void getPath(char* command,char* host,char* path){   //functie care ne extrage calea catre fisierul pe care-l dorim

    char* aux=command;
    aux=aux+7+strlen(host);
    strcpy(path,aux);
    path[strlen(path)-1] = '\0';

}

ssize_t Readline(int sockHTTP, void *vptr, size_t maxlen) {   //functie care citeste pagina de tip html linie cu linie
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ ) {    
    if ( (rc = recv(sockHTTP, &c, 1,0)) == 1 ) {
        *buffer++ = c;
        if ( c == '\n' )
        break;
    }
    else if ( rc == 0 ) {
        if ( n == 1 )
        return 0;
        else
        break;
    }
    else {
        if ( errno == EINTR )
        continue;
        return -1;
    }
    }

    *buffer = 0;
    return n;
}

void send_command(int sockHTTP, char sendbuf[],FILE *f) { //functie care trimite comanda si descarca informatiile desprea acea pagina http

  


  char recvbuf[MAXLEN];
  int nbytes;
  
  printf("Trimit: %s", sendbuf);
  fflush(stdout);
  send(sockHTTP, sendbuf, strlen(sendbuf),0);

   nbytes = Readline(sockHTTP, recvbuf, MAXLEN - 1);
   fprintf(f,"%s",recvbuf);
   fflush(f);

  while (nbytes>=0){
    nbytes = Readline(sockHTTP, recvbuf, MAXLEN - 1);
     fprintf(f,"%s",recvbuf);
     fflush(f);
     printf("%s",recvbuf);
     fflush(stdout);
  }
}


void getFile (int sockfd,FILE *f,char* command,char* host,char* path){  //functie care creeaza un socket intre client si server http pentru a putea face rost de pagina dorita

  int sockHTTP;
  struct sockaddr_in servaddr ;
  int i;

  struct hostent *he;
  he=gethostbyname(host);     //extragem adresa ip pornind de la nume
  struct in_addr **addr_list;
  addr_list=(struct in_addr **)he->h_addr_list;

  printf("IP address: %s\n", inet_ntoa(**addr_list));
  fflush(stdout);


  if ( (sockHTTP = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {   //construim un socket folosindu-ne de adresa ip de mai sus
    printf("Eroare la  creare socket.\n");
    exit(-1);
  }  


  

 
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(80);    //portul pe care ruleaza httpul
  servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));   //ipul hostului


  
  if (connect(sockHTTP, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {  //conectarea la server
    printf("Eroare la conectare\n");
    exit(-1);
  }


  char buffer[100];
  sprintf(buffer,"GET %s HTTP/1.1\nHost:%s\n\n",path,host);   //construirea si trimiterea comenzii

  send_command(sockHTTP, buffer,f);  //apelarea metodei care trimite 

}


int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFLEN];
    char result[BUFLEN];

    char host[100];
    char path[100];
    memset(host,0,100);
    memset(path,0,100);

    char ipAddr[32] ;
    int portno ;

    int log_file=0;
    char log_name[100];
    FILE *logFile,*logError;
    char* pid = (char*)malloc(100*sizeof(char));
    sprintf(pid,"%d",getpid());
    int i=0;

    if (argc < 5) {
         fprintf(stderr,"Flag-urile '-p <port>' si '-a <IP_Server>'trebuie sa apara obligatoriu!\n");   //aceste flag-uri sunt obligatorii
         exit(1);
     }

     for (i=0;i<argc;i++){
        if (strcmp(argv[i],"-o") == 0 ) {     //daca avem fisier de log construim fisierele
            log_file = 1;
            char copy[100];

            strcpy(log_name,argv[i+1]);
            strcat(log_name,"_");
            strcat(log_name,pid);
            strcpy(copy,log_name);
            logFile = fopen (strcat(log_name,".stdout"),"wt");
            logError = fopen (strcat(copy,".stderr"),"wt");
            i++;
        }
        if (strcmp(argv[i],"-p") == 0){
             portno = atoi(argv[i+1]);
             i++;
        }

        if (strcmp(argv[i],"-a") == 0){
             strcpy(ipAddr,argv[i+1]);
             i++;
        }
     }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      if (log_file == 1){
          fprintf(logError,"ERROR opening socket");
          fflush(logError);
      }
        error("ERROR opening socket");
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    inet_aton(ipAddr, &serv_addr.sin_addr);
    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) {
      if (log_file == 1){
          fprintf(logError,"ERROR connecting");
          fflush(logError);
      }
        error("ERROR connecting");   
        } 
    
    while(1){
        
        memset(buffer, 0 , BUFLEN);
        memset(result, 0 , BUFLEN);
       

        if ((n = recv(sockfd, result, BUFLEN, 0)) <= 0) {
            if (n == 0) {
                if (log_file == 1){
                  fprintf(logFile,"Conexiunea cu socket-ul %d s-a inchis\n", sockfd);
                  fflush(logFile);
                }
                
                printf("Conexiunea cu socket-ul %d s-a inchis\n", sockfd);
                fflush(stdout);
                } else {
                    if (log_file == 1){
                       fprintf(logError,"ERROR connecting");
                      fflush(logError);
                    }
                    error("ERROR in recv");
                }
              
        }else { 
          if (strncmp(result,"download",8) == 0){   //daca am primit comanda de download
            FILE *f = fopen("aux","wt");    //ne luam un fisier auxiliar in care sa retinem ceea ce primi de la serverul http

             if (log_file == 1){
                       fprintf(logFile,"%s",result);
                      fflush(logFile);
                    }

            printf ("%s",result);
            fflush(stdout);

            getHost(strdup(result+9),host);     //scoatem hostul
            getPath(strdup(result+9),host,path);    //scoatem calea
            

            if (log_file == 1){
                       fprintf(logFile,"HOST:%s\n",host);
                      fflush(logFile);
                    }
            printf("HOST:%s\n",host);
            fflush(stdout);

            if (log_file == 1){
                       fprintf(logFile,"PATH:%s\n",path);
                      fflush(logFile);
                    }

            printf("PATH:%s\n",path);
            fflush(stdout);

            

           getFile(sockfd,f,strdup(result+9),host,path);    //trimitem comanda catre server-ul http

            fclose(f);
           
        

       /*  FILE *g = fopen ("./aux","rt");

            while (fgets(buffer,sizeof(char),g) != NULL){    //cat timp mai putem sa citim di fisier

              if (log_file == 1){
                       fprintf(logFile,"%s",buffer);
                      fflush(logFile);
                    }
              printf("%s",buffer);
              fflush(stdout);
               n = send(sockfd,buffer,BUFLEN, 0);     //trimitem datele la server care,la randul sau,le memoreaza intr-un fisier
               if (n < 0) {
                if (log_file == 1){
                       fprintf(logError,"ERROR writing to socket");
                      fflush(logError);
                    }
                error("ERROR writing to socket");
              }
           //   recv(sockfd,result,BUFLEN, 0);
            }
            fclose(g);

        strcpy(buffer,"Download terminat");     //daca am terminat trimiterea,inseamna ca clientul si-a indeplinit sarcinsa si trimite un mesaj la server
        if (log_file == 1){
                       fprintf(logFile,"%s",buffer);
                      fflush(logFile);
                    }
        
        n = send(sockfd,buffer,strlen(buffer), 0);
        if (n < 0) {
          if (log_file == 1){
                       fprintf(logError,"Error writing to socket");
                      fflush(logError);
                    }
             error("ERROR writing to socket");
           }*/
         }

       }
    }
    close(sockfd);
    fclose(logFile);
    fclose(logError);
    return 0;
}



