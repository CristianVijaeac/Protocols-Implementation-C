#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFLEN 256



void error(char *msg)
{
    perror(msg);
    exit(0);
}

//functie care ia al doilea argument(pt comenzile noastre->username-ul)

void getName(char* str,char* name){
	
     char* tmp = (char*)malloc(100*sizeof(char));
     int j = 0;

     tmp = strtok(str," ");
 
     while(tmp != NULL){
            
            if (j == 1){
                strncpy(name,tmp,strlen(tmp));
            }   
            tmp = strtok(NULL," ");
            j++;

        }
}


int main(int argc, char *argv[])
{
    
    int sockfd, n;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    char result[BUFLEN];

    int some1Connected = 0; //1-daca exista un user autentificat deja,0-daca nu exista
    char* name = (char*)malloc(100*sizeof(char));
    memset(name,0,100);
   
    char* pidw = (char*)malloc(100*sizeof(char));
    sprintf(pidw,"%d",getpid());
   
    //creeam/deschidem fisierul de log al clientului 
    char* filename = (char*)malloc(100*sizeof(char));
    strcpy(filename,"client_");
    strcat(filename,pidw);
    FILE* log=fopen(filename,"wt"); 
    
    //construim un socket TCP si il identificam cu ajutorul descriptorului
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    //setam parametrii necesari conexiunii cu serverul
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    //realizam conexiunea cu serverul
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    while(1){
  		//citesc de la tastatura
    	memset(buffer, 0 , BUFLEN);    //buffer pentru citire
	    memset(result, 0 , BUFLEN);    //buffer in care se privesc mesajele de la server
    	fgets(buffer, BUFLEN-1, stdin);

        //daca exista o sesiune deja activa (un utilizator deja logat),se intoarce un mesaj de eroare fara a se mai trimite comanda la server
        if (strncmp(buffer,"login",5) == 0 && some1Connected == 1){
		        printf ("-2 Sesiune deja activa\n");
                fflush(stdout); 
                fprintf (log,"%s",buffer);
                fprintf (log,"-2 Sesiune deja activa\n");               
		        printf ("%s",name);
                fflush(stdout);		

        //daca nu exista o sesiune deschis,adica niciun utilizator nu este autentificat insa se dau comenzi,se intoarce un mesaj de eroare fara a se mai trimit comanda la server
	}else if ((strncmp(buffer,"delete",6) == 0 || strncmp(buffer,"logout",6) == 0 || strncmp(buffer,"getuserlist",11) == 0 || strncmp(buffer,"getfilelist",11) == 0 || strncmp(buffer,"share",5) == 0 || strncmp(buffer,"unshare",7) == 0) && some1Connected == 0){
		        printf ("-1 Clientul nu e autentificat\n");
                fflush(stdout);
                fprintf (log,"%s",buffer);
                fprintf(log,"-1 Clientul nu e autentificat\n");
	}else{

    	//trimit mesaj la server
    	n = send(sockfd,buffer,strlen(buffer), 0);
    	if (n < 0) 
        	 error("ERROR writing to socket");

    //daca am citit de la tastatura "quit",inseamna ca vom inchide conexiunea dintre clientul pe care ne aflam si serverul
	if (strncmp(buffer,"quit",4) == 0 ) {
        fprintf (log,"%s",buffer);
        break;
    }

    //primi mesaj de la server
	if ((n = recv(sockfd, result, BUFLEN, 0)) <= 0) {
		if (n == 0) {
		
		//conexiunea s-a inchis
		printf("Conexiunea socket-ului %d s-a inchis\n", sockfd);
        fflush(stdout);
		} else {
			error("ERROR in recv");
		}
	} 

    //conexiunea este inca activa				
	else { //recv intoarce >0

            //daca comanda "login" intoarce de la server mesajul "0",inseamna ca ne-am logat cu succes
            if (strncmp(result,"0",1)==0){
	           	some1Connected=1;  //memoram faptul ca cineva s-a conectat
                fprintf (log,"%s",buffer);  
	            getName(buffer,name);
		        printf ("%s",strcat(name,">"));   //construim prompt-ul format din "numele user-ului autentificat"+">"
                fflush(stdout);
                fprintf (log,"%s",name);    //afisam promptul

            //daca primim mesajul "Deconectare",inseamna ca s-a realizat cu succes logout-ul
	    }else if (strncmp(result,"Deconectare",11)==0){
                some1Connected=0;   //memoram faptul ca nu mai este nimeni conectat
		        printf ("%s",result);
		        fflush(stdout);	
                fprintf (log,"%s",buffer);
                fprintf (log,"%s",result);     

            //pentru comenzile "delete","getuserlist","getfilelist","share","unshare",afisam ceea ce returneaza(chiar si erori) si realizam si scrierea in fisier
		}else if (strncmp(buffer,"delete",6)==0 || strncmp(buffer,"getuserlist",11)==0 || strncmp(buffer,"getfilelist",11)==0 || strncmp(buffer,"share",5)==0 || strncmp(buffer,"unshare",7)==0){
			    printf("%s",result);
                fflush(stdout);
                fprintf (log,"%s",buffer);
                fprintf (log,"%s",result);
			    printf ("%s",name);
                fflush(stdout);
                fprintf (log,"%s",name);

            //daca primim mesajul "close",inseamna ca serverul s-a inchis iar conexiunea actuala a clientului trebuie si ea inchis
         } else if (strncmp(result,"close",5)==0 )  {
            printf("Conexiune inchisa de server!\n");
            fflush(stdout);
            fprintf (log,"Conexiune inchisa de server\n");
            close(sockfd);
            fclose(log);
            exit(0);

            //daca primim eroarea "-8 Brute-force detected" inseamna au fost >3 login-uri nereusit si trebuie sa inchidem clientul
         } else if (strncmp(result,"-8",2)==0)  {
            printf("%s\nConexiune inchisa de server!\n",result);
            fflush(stdout);
            fprintf (log,"%s\nConexiune inchisa de server!\n",result);
            close(sockfd);
            fclose(log);
            exit(0);
        
            //pentru alte mesaje receptionate se va face direct afisarea
        }else{
	    printf("%s",result);
	    fflush(stdout);
            fprintf (log,"%s",buffer);
            fprintf (log,"%s",result);
		}
           
	}
	}
    }
    close(sockfd);
    fclose(log);

    return 0;
}

