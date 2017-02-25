#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MIN_CLIENTS 5
#define MAX_CLIENTS	10
#define BUFLEN 256

typedef struct l {			//structura de date sub forma de lista simplu inlantuita,in care tinem minte clientii(socket-ul,clientul,urmatorul client si starea in care se afla Download sau liber)

	int sockfd;
	struct sockaddr_in client;
	struct l *next;
	int itDownloads;
}Clients;

void error(char *msg)
{
    perror(msg);
    exit(1);
}


void getHost(char* command,char* host){		//functie care scoate adresa pe care ne conectam dintr-un link

	char* aux=command;
	aux=aux+7;
	strcpy(host,strtok(aux,"/"));
	
	

}

void getPath(char* command,char* host,char* path){	//functie care scoate calea absoluta catre fisier

	char* aux=command;
	aux=aux+7+strlen(host);
	strcpy(path,aux);
	path[strlen(path)-1] = '\0';

}

void makeDir(char* host,char* path_dir){	//functie care construieste ierarhia de directoarea

	char aux[100];
	memset(aux,0,100);
	int dir_path = strrchr(path_dir,'/')-path_dir;
	strncpy(aux,path_dir,dir_path);
	aux[strlen(aux)] = '\0';

	char dir[100];
	memset(dir,0,100);
	strcat(dir,"mkdir -p ./");
	strcat(dir,host);
	strcat(dir,aux);
	system(dir);
				
}

void getFile (int sockfd,FILE *f,char* command,char* host,char* path,FILE *logError,int log_file){	//functie care trimite comanda de download la client

		char *result=(char*)malloc(100*sizeof(char));
		strcpy(result,command);
		int n = send(sockfd,result,strlen(result), 0);
		if (n < 0) {
			if (log_file == 1){
     			fprintf(logError,"ERROR writing to socket");
     			fflush(logError);
     		}
        	error("ERROR writing to socket");
        }
       
}

int toDownload(Clients *clients,FILE *logFile,int log_file){	//functie care alege ce socketu nu este ocupat si poate downloada

	Clients* aux=clients;

	for (aux=clients;aux!=NULL;aux=aux->next){
		if (aux->itDownloads == 0){
			if (log_file == 1){
     			fprintf(logFile,"Downloadez pe socketul : %d\n",aux->sockfd);
     			fflush(logFile);
     		}
			printf("Downloadez pe socketul : %d\n",aux->sockfd);
			fflush(stdout);
			aux->itDownloads = 1;
			return aux->sockfd;
		}
	}
}


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     char result[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     Clients *clients = NULL;
     int n, i, j;
     int isConnected[20];
     int numClients = 0;
     for (i=0;i<20;i++)
     	isConnected[i] = 0;

     int recursive = 0;
     int everything = 0;
     int log_file = 0;
     char log_name[100] ;
     char host[100];
     char path[100];
     memset(host,0,100);
     memset(path,0,100);
     FILE *f,*logFile,*logError;



     fd_set read_fds;	
     fd_set tmp_fds;	
     int fdmax;		

     if (argc < 3) {
         fprintf(stderr,"Flag-ul '-p <port>' trebuie sa apara obligatoriu!\n");
         exit(1);
     }

     for (i=0;i<argc;i++){		//modurile de lucru ale programului
     	if (strcmp(argv[i],"-r") == 0 ) recursive=1;
     	if (strcmp(argv[i],"-e") == 0 ) everything=1;
     	if (strcmp(argv[i],"-o") == 0 ) {
     		log_file = 1;
     		char copy[100];
     		strcpy(log_name,argv[i+1]);
     		strcpy(copy,log_name);
     		logFile = fopen(strcat(log_name,".stdout"),"wt");
     		logError = fopen(strcat(copy,".stderr"),"wt");

     		i++;
     	}
     	if (strcmp(argv[i],"-p") == 0){		//extragem portul
     		 portno = atoi(argv[i+1]);
     		 i++;
     	}
     }

    
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
     	if (log_file == 1){
     		fprintf(logError,"ERROR opening socket\n");
     		fflush(logError);
     	}
        error("ERROR opening socket");
    }
     
    

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
     	if (log_file == 1){
     		fprintf(logError,"ERROR on binding\n");
     		fflush(logError);
     	}
              error("ERROR on binding");
      }
     
     listen(sockfd, MAX_CLIENTS);

     FD_SET(sockfd, &read_fds);
     FD_SET(0,&read_fds);
     fdmax = sockfd;
     


	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			if (log_file == 1){
     		fprintf(logError,"ERROR in select\n");
     		fflush(logError);
     	}
			error("ERROR in select");
		}
	
		if(FD_ISSET(0, &tmp_fds)) {		//verificam daca am primit vreo comanda de la stdin
			char command[100];
            fgets(command, 100 , stdin);
            if (strncmp(command,"status",6) == 0){		//daca am primit comanda status,afisam toti clientii logati in acel moment folosindu-ne de structura de date de mai sus
            	if (log_file == 1){
     					fprintf(logFile,"Comanda status\n");
     					fflush(logFile);
     				}
            	Clients *p = clients;
            	for (p=clients;p!=NULL;p=p->next){
            		if (log_file == 1){
     					fprintf(logFile,"Client pe socket-ul %d cu ip-ul %s si portul %d\n",p->sockfd,inet_ntoa(p->client.sin_addr), ntohs(p->client.sin_port));
     					fflush(logFile);
     				}
            		printf("Client pe socket-ul %d cu ip-ul %s si portul %d\n",p->sockfd,inet_ntoa(p->client.sin_addr), ntohs(p->client.sin_port));
            		fflush(stdout);
            	}
            	continue;
            }else if (strncmp(command,"exit",4) == 0){		//daca am primit exit
            	if (log_file == 1){
     					fprintf(logFile,"Comanda exit\n");
     					fflush(logFile);
     				}
            	int j = 0;
                for(j = 0; j <= fdmax; j++){		
                    if (isConnected[j] == 1){ 		//pentru fiecare client conectat,inchidem conexiunea cu acesta!
                   
    						if (n < 0) {
    							if (log_file == 1){
     								fprintf(logError,"ERROR writing to socket\n");
     								fflush(logError);
     							}
        						 error("ERROR writing to socket");
        						}
                       	close(j); 
                        FD_CLR(j, &read_fds);
                        FD_CLR(j, &tmp_fds);
                        isConnected[j] = 0;
                    }
                }
                if (log_file == 1){
     					fprintf(logFile,"Serverul se inchide\n");
     					fflush(logFile);
     				}
                printf("Serverul se inchide!\n");
                fflush(stdout);
                exit(0);
            }else if (strncmp(command,"download",8) ==0 ){		//daca am primit comnada download
            	if (log_file == 1){
     					fprintf(logFile,"Comanda download\n");
     					fflush(logFile);
     				}
            	int count = 0;
				for (j=0;j<20;j++){
					
					if (isConnected[j] == 1)
						count++;
					}
						
					if (count <5){		//verificam daca avem mai > de 5 clienti pentru a putea efectua comanda
						if (log_file == 1){
     						fprintf(logError,"Mai am nevoie de %d clienti pentru a putea primi comenzi\n",5-count);
     						fflush(logError);
     					}
						printf("Mai am nevoie de %d clienti pentru a putea primi comenzi\n",5-count);
						fflush(stdout);
						continue;

   							 
					}else{
						if (log_file == 1){	
     						fprintf(logFile,"Clienti suficienti pentru a incepe transferul\n");
     						fflush(logFile);
     					}
						printf ("Clienti suficienti pentru a incepe transferul\n");		//avem suficienti clienti
						fflush(stdout);
						
						getHost(strdup(command+9),host);			//scoatem numele
            			getPath(strdup(command+9),host,path);		//scoate calea

            			if (strstr(path,".html") == NULL || strstr(path,".htm") == NULL){		//verificam daca linkul este valid
            				if (log_file == 1){
     							fprintf(logError,"Link invalid.Linkul trebuie sa contina un fisier cu extensia '.html' sau '.htm'\n");
     							fflush(logError);
     						}
            				printf ("Link invalid.Linkul trebuie sa contina un fisier cu extensia '.html' sau '.htm'\n");
							fflush(stdout);
							continue;
            			}else{
             	
						makeDir(host,strdup(path));		//daca da,construim ierarhia de directoare
						char fileCreate[100];			//creeam fisierul ce trebuie downloadat
						memset(fileCreate,0,100);
						strcpy(fileCreate,"./");
						strcat(fileCreate,host);
						strcat(fileCreate,path);
						f = fopen(fileCreate,"wt");
						int whoDownloads = toDownload(clients,logFile,log_file);		//vedem cui assignam acest download
						getFile(whoDownloads,f,command,host,path,logError,log_file);	//executam comanda

						memset(host,0,100);
     					memset(path,0,100);
     				
     				}
				
            	}

        }
    }

		for(i = 1; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
			
				if (i == sockfd) {
					
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						if (log_file == 1){
     							fprintf(logError,"ERROR in accept");
     							fflush(logError);
     						}
						error("ERROR in accept");
					} 
					else {
						
						FD_SET(newsockfd, &read_fds);
						if (clients == NULL){		//daca avem un nou client care s-a conectat,trebuie sa-l adaugam in structura noastra
								clients = malloc(sizeof(Clients));	//daca structua e goala,o initializam 
								clients->sockfd = newsockfd;
								clients->client = cli_addr; 
								clients->next = NULL;
								clients->itDownloads = 0;
						}else{
							Clients* aux = NULL;		//daca nu,mergem pana la sfarsit,acolo unde facem inserarea
							for (aux=clients;aux->next!=NULL;){
								aux=aux->next;
							}
							Clients* newClient = malloc (sizeof (Clients));
							newClient->sockfd = newsockfd;
							newClient->client = cli_addr;
							aux->next = newClient;
							newClient->next = NULL;
							newClient->itDownloads = 0;
						}
						if (newsockfd > fdmax) {
							fdmax = newsockfd;
						}
					}
					if (log_file == 1){
     							fprintf(logFile,"Clientul %s,s-a conectat pe portul %d, socket-ul %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
     							fflush(logFile);
     				}
					printf("Clientul %s,s-a conectat pe portul %d, socket-ul %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
					isConnected[newsockfd] = 1;		//vector in care tinem minte pe ce socketi se afla deja clienti
				}
					
				else {
				
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							
							if (log_file == 1){
     							fprintf(logFile,"Clientul de pe socket-ul %d s-a deconectat\n", i);
     							fflush(logFile);
     						}
							printf("Clientul de pe socket-ul %d s-a deconectat\n", i);
							fflush(stdout);
							Clients *p=NULL,*ante=NULL;
							for (p=clients,ante=p;p->next!=NULL;ante=p,p=p->next){	//daca clientul s-a deconectat,trebuie scos din structura de date cu clienti conectati
								if (p->sockfd == i)
									break;
							}
							if (p == clients && p->next==NULL){		//cazul in care clientul este singurul element al listei
								free(clients);
								clients=NULL;
							}else
							if (p==clients && p->next!=NULL){		//cazul in care clientul este capul listei
								clients=clients->next;
								free(p);
								p=NULL;
							}else
							if (p->next == NULL){			//cazul in care clientul se afla la finalul listei
								
								free(ante->next);
								ante->next=NULL;
								
							}
							else {
								ante->next=p->next;		//cazul in care se afla la mijloc
								free(p);
								p=NULL;
							}
						} else {
							if (log_file == 1){
     							fprintf(logError,"ERROR in recv");
     							fflush(logError);
     						}
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds);
						isConnected[i] = 0;		//actualizam vectorul de useri conectati in acel moment
					} 
					
					else {
						
						/*if (strncmp(buffer,"Download terminat",17) != 0){	//daca nu am primit acest mesaj,inseamna ca in continuare primim bucati din pagina HTML
							
     							fprintf(f,"%s\n",buffer);
     							fflush(f);
     						
							printf("%s",buffer);
							fflush(stdout);
							//send(i,"a",1,0);
						}else{
							if (log_file == 1){			//daca am primit acel mesaj,inseamna ca am finalizat descarcarea paginii
     							fprintf(logFile,"Download terminat\n");
     							fflush(logFile);
     						}
							printf("Download terminat");
							fflush(stdout);
							fclose(f);
							//return ;
						}*/
						
   						}
					}
					//n = send(sockfd,"aa",2, 0);

				} 
			}
		
     }

     fclose(logFile);
     fclose(logError);
     close(sockfd);
   
     return 0; 
}


