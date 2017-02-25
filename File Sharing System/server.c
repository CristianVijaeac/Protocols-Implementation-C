#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_CLIENTS	100
#define BUFLEN 1000

//structura ce memoreaza datele de pe useri(nume si parola)

typedef struct{
    int no_users;
	char userName[24];
	char password[24];
	}Users;

//structura ce memoreaza datele despre fiserele partajate(user-ul detinator,numele fisierului si tipul[pentru fisierele partajate 1])

typedef struct{
    int no_files;
	char owner[24];
	char name[24];
	int type;
	}SharedFiles;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

//functie ce returneaza al doilea parametru al unei comenzi(pentru cazurile noastre acesta este reprezentat de username)

void getName(char* str,char* name){
    
     char* tmp = (char*)malloc(100*sizeof(char));
     memset(tmp,0,100);
     int j = 0;

     tmp = strtok(str," ");
 
     while(tmp != NULL){

            if (j == 1){
                strncpy(name,tmp,strlen(tmp) - 1);
            }   
        
            tmp = strtok(NULL," ");
            j++;
        }
}


//functie care verifica daca un user exista in lista

int existsUser(Users* users,char* str){

	int i = 0;
	
    for (i = 0;i < users[0].no_users;i++){
		if (strncmp(users[i].userName,str,strlen(str)) == 0){ //daca intalnim numele user-ului cel putin o data,intoarcem pozitia lui
			return i;
		}

	}

	return -1; //daca am ajuns la final => nu am gasit userul,intoarcem -1


}

//functie ce face citirea din fisierul users_config si construieste structura care se ocupa de datele userilor

Users* createUsers(FILE *f){
	
    int no_users = 0;
    int i = 0;
    char *tmp = (char*)malloc(sizeof(char)*100);
    memset(tmp,0,100);
    char *str = (char*)malloc(sizeof(char)*100);
    memset(str,0,100);

    fgets(str,100,f);  //citim numarul de utilizatori din fisier
    no_users = atoi(str);

    Users* users = (Users*) malloc (no_users * sizeof(Users));

    while (fgets(str,100,f) != NULL){   //cat timp avem useri de citit
     	users[i].no_users=no_users; 
     	tmp = strtok(str, " ");
     	int j = 1;
     	
        while(tmp != NULL){
        	
            if (j == 1)
        		strcpy(users[i].userName,tmp);     // j==1 inseamna ca citim username-ul
        	else
        		strcpy(users[i].password,tmp);    //j==2 inseamna ca citim parola
        	
            tmp = strtok(NULL, " ");
    		j++;
    	}
    	
        i++;
    }
	
    return users;
}


//functie care verifica daca exista un fiser in directorul utilizatorului(daca directorul nu exista,il creaza,iar daca exista doar il acceseaza)

int existsFile(char * owner,char* filename){

	char* path = (char*)malloc(100*sizeof(char));
    memset(path,0,100);
	strcpy(path,"./");     //construim calea catre director
	strcat(path,owner);
	
	if(mkdir(path,0777) == 0){   //daca directorul nu exista,se construieste,dar este evident ca nici fisierele nu exista si intoarcem 0
		return 0;

	}else if (mkdir(path,0777) == -1 ){  //daca directorul exista,construim calea catre un anumit fisier
		strcat(path,"/");
		strcat(path,filename);

		if (access(path, F_OK) != -1){    //daca functia access intoarce o valoarea != -1 inseamna ca fisierul exista
        		return 1;
    	}else{
			return 0;
		}
	
    }
    
}

//functie care citeste din fisierul shared_files si construieste structura de date cu fisiere partajate

SharedFiles* createFiles(FILE *g,Users* users){
	
    int no_files = 0;
    int i = 0;
    char *tmp = (char*)malloc(sizeof(char)*100);
    memset(tmp,0,100);
    char *str = (char*)malloc(sizeof(char)*100);
    memset(str,0,100);
     	
    fgets(str,100,g);
    no_files = atoi(str);   //numarul de fisere partajate

    SharedFiles* files = (SharedFiles*) malloc (1000 * sizeof(SharedFiles));

    while (fgets(str,100,g) != NULL){   //cat timp avem fisiere de introdus in structura
     	
     	tmp = strtok(str,":");
     	int j = 1;
     	SharedFiles f;
     	while(tmp != NULL){
        	
        	if (j == 1){   
        		strcpy(f.owner,tmp);  //j == 1 inseamna ca citim detinatorul fisierului
        	}else{
        		strncpy(f.name,tmp,strlen(tmp)-1);    //j == 2 inseamna ca citim numele fisierului
           	}
        		
        	tmp = strtok(NULL,":");
    		j++;

    	}

    	if (existsUser(users,f.owner) != -1 && existsFile(f.owner,f.name) == 1) {  //introducem in structura numai daca user-ul si fisierul exista,daca nu trecem peste
		  strcpy(files[i].owner,f.owner);
		  strcpy(files[i].name,f.name);
		  files[i].type=1;	
		  i++;
	    }	

    }

    int k=0;

    for (k = 0; k < i;k++){
        files[k].no_files=i;
    }

    return files;
}

//functie verifica credentialele de autentificare primite de la client cu cele din "baza de date" a serverului

int checkAuthentication(int socket,char* name,Users* users,char* buffer,int* brute_force){

    Users u;

    int j = 0;

    char *tmp = (char*)malloc(sizeof(char)*100);
    memset(tmp,0,100);
    char *str = (char*)malloc(sizeof(char)*100);
    memset(str,0,100);
     
    strcpy(str,buffer);
    
    tmp = strtok(str," "); 

    while(tmp != NULL){     //extragem username-ul si parola din comanda de login primita de la client
            
            if (j == 1){
                strcpy(u.userName,tmp); //daca j == 1 inseamna ca avem username
            }else if (j == 2){
                strcpy(u.password,tmp); //daca j == 2 inseamna ca avem parola
            }
                
            tmp = strtok(NULL," ");
            j++;

    }
  
    int index=existsUser(users,u.userName); //verificam daca exista user-ul in "baza de date"(structura noastra)
  
    if (index != -1 && strcmp(users[index].password,u.password) == 0){ //daca userul SI parola sunt corecte inseamna ca se poate realiza login-ul
        strcpy(name,u.userName);
        return 1;
    }else{
        brute_force[socket]+=1;   //daca parola sau userul nu este corect intoarcem codul de eroare -3 si incrementam numarul de login-uri esuate consecutiv
        return -3;
    }
    
}

//functia care verifica credentialele primite ca parametru cu informatiile din structura de date a userilor si trimite un mesaj corespunzator clientului

int login (int socket,char* buffer,Users* users,char* name,int* brute_force){

   
    char* result = (char*)malloc(sizeof(char)*BUFLEN);
    memset(result, 0 , BUFLEN);

    int login = checkAuthentication(socket,name,users,buffer,brute_force);
    
    if (brute_force[socket]==3){
        strcpy(result, "-8 Brute-Force detected");    //daca am avut 3 login-uri esuate,intoarcem eroarea -8
    }else if (login != -3){
        char* tmp_nume=name;       //daca verificarea credentialelor nu intoarce eroare -3 inseamna ca login-ul este corect
        strcpy(result, "0");
    }else 
        strcpy(result, "-3 : User/Parola gresita\n");   //daca se intoarce -3 inseamna ca s-au gresit credentialele de autentificare

    //trimitem clientului un mesaj corespunzator
    int n = send(socket,result,strlen(result), 0);
    if (n < 0) {
        error("ERROR writing to socket");
    }
	
	return login;

}



//functie ce implementeaza comanda "getuserlist"

void getUserList(int socket,Users* users){
     
    char* result = (char*)malloc(sizeof(char) * BUFLEN);  //buffer pentru trimiterea rezultatului 
    memset(result,0,BUFLEN);

    int i = 0;
    int n = 0;
    for (i = 0;i < users[i].no_users;i++){      //pentru fiecare user existent,luam username-ul si il punem in buffer
        strcat(result,users[i].userName);
        strcat(result,"\n");
    }

    n = send(socket,result,strlen(result),0);  //trimitem buffer-ul care contine lista de useri existenti
        if (n < 0) {
            error("ERROR writing to socket");
            }
}


//functie care executa comanda  "getfilelist"

void getFileList (int socket,SharedFiles* files,Users* users,char* name){

    int i = 0;
    char* result = (char*)malloc(sizeof(char)*BUFLEN);
    memset(result, 0 , BUFLEN);
    
    //verificam daca exista user-ul pentru care se cere lista de fisiere,daca nu se intoarce eroare -11
    if (existsUser(users,name)==-1){
        strcpy(result, "-11 Utilizator inexistent\n");
    
        int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
              error("ERROR writing to socket");
            }

        return ;
    }else{

        char *path = (char*)malloc(100*sizeof(char));
        memset(path,0,100);
        strcpy(path,"./");  //construim calea catre directorul user-ului
        strcat(path,name);

        DIR *dir=opendir(path);
        struct dirent* d;   //deschidem directorul pentru citire
       
        strcat(path,"/");
        
        if (dir != NULL)
        {
            while (d = readdir(dir)){     //cat timp exista fisiere,executam citirea
                
                if (strncmp(".",d->d_name,1) == 0 || strncmp("..",d->d_name,2) == 0){      //ignoram afisarea folderelor parinte/radacina
                    continue;
                }
    
                char* path2 = (char*) malloc(sizeof(char)*100);
                memset(path2,0,100);
                     
                strcpy(path2,path);
                strcat(result,d->d_name);   
                     
                strcat(path2,d->d_name); //atasam caii noastre numele fisierului pentru a putea calcula dimensiunea lui

                FILE *fsize = fopen(path2,"rt");   //deschidem fisierul pt citire
                fseek(fsize,0L,SEEK_END);  //setam pointer-ul din fisier la sfarsitul acestuia
                int size = ftell(fsize);     //calculam distanta de la inceput la pozitia pointer-ului (finalul fisierului)
                fclose(fsize);     //inchidem fisierul

                char* buff = (char*)malloc(sizeof(char)*BUFLEN);
                sprintf(buff,"%d",size);
                strcat(result," ");
                strcat(result,buff);
                strcat(result," bytes");

                int i = 0;
                int exists = 0;
                     
                for (i = 0;i < files[0].no_files;i++){
                    if (strcmp(d->d_name,files[i].name) == 0){  //verificam daca fisierul exista in lista noastra de fisiere partajate
                        exists=1;
                        break;
                    }
                }

                if (exists==1){     //daca exista  inseamna ca este partajat
                    strcat(result," SHARED\n");
                }else{
                    strcat(result," PRIVATE\n");    //daca nu exista,nu este partajat ci privat
                }
                 free(buff);

            }

            closedir(dir);
        }         
    
    }
	
	int n = send(socket,result,strlen(result), 0);      //trimitem mesajul corespunzator clientului (eroare sau lista dorita)
        if (n < 0) {
            error("ERROR writing to socket");
        }
	
}

//functie care executa comanda "share" transformand un fisier privat intr-unul partajat

void share(int socket,char* user,SharedFiles* files,char* fileName){

    char* result = (char*)malloc(BUFLEN*sizeof(char));
    memset(result,0,BUFLEN);
    
    int i = 0;
    int exists = 0;
    SharedFiles f;

    for(i = 0;i < files[0].no_files;i++){
        if (strcmp(files[i].name,fileName) == 0 && strcmp(files[i].owner,user) == 0){   //verificam daca fisierul pe care vrem sa-l partajam este deja partajat si mai mult,daca acest fisier corespunde utilizatorului care a lansat comanda "unshare",caz in care returnam eroarea -6
            strcpy(result,"-6:Fisierul este deja partajat\n");

            int n = send(socket,result,strlen(result), 0);  //trimitem aceasta eroare clientului
            if (n < 0) {   
                error("ERROR writing to socket");
                }
                    
            return;
        }
    }

    //daca fisierul nu este partajat(nu exista in lista),verificam daca exista in folder

    char *path = (char*)malloc(100*sizeof(char));
    memset(path,0,100);
    strcpy(path,"./");
    strcat(path,user);

    DIR *dir = opendir(path);
    struct dirent* d;
    int j=0;

    //deschidem si citim fisierele existente in director

    if (dir!=NULL){
            while (d = readdir(dir)){
            
                if (j != 2){
                    j++;
                    continue;
                }
            
                if (strcmp(d->d_name,fileName) == 0){
                    exists = 1;
                    break;
                }
            }
            closedir(dir);
    } 

    if (exists==0){     //daca fisierul nu exista afisam si trimitem un mesaj corespunzator clientului

        strcpy(result,"-4 Fisier inexistent\n");
        int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }
        return;             
    
    }else{

        //daca fisierul exista inseamna ca este PRIVAT si pt a-l partaja trebuie mai intai introdus in structura noastra de fisiere partajate
        strcpy(files[files[0].no_files].owner,user);
        strcpy(files[files[0].no_files].name,fileName);
        files[files[0].no_files].type = 1;
        files[files[0].no_files].no_files = files[0].no_files;

        int k = 0;
        int size = files[0].no_files;
        
        for (k = 0; k <= size;k++){
            files[k].no_files++;        //actualizam nr de fisiere din structura
        }

       

        strcpy(result,"200 Fisierul a fost setat ca SHARED\n");     //trimitem un mesaj prin care marcam succesul conversiei private->shared
        int n = send(socket,result,strlen(result), 0);
        
        if (n < 0) {
            error("ERROR writing to socket");
        }
        
        return;
    }

}

//functia care executa comanda "unshare" prin care un fisier partajat va deveni privat

void unshare(int socket,char* user,SharedFiles* files,char* fileName){

    char* result = (char*)malloc(BUFLEN*sizeof(char));
    memset(result,0,BUFLEN);
    
    int i = 0;
    int exists = 0;

    //verificam daca fisierul exista in lista (daca este deja partajat) si mai mult,daca acest fisier corespunde utilizatorului care a lansat comanda "unshare"
    for(i = 0;i < files[0].no_files;i++){
        if (strcmp(files[i].name,fileName) == 0 && strcmp(files[i].owner,user) == 0){
            exists=1;
            break;
        }
    }

    //daca fisierul exista in lista,inseamna ca este SHARED
    if (exists==1){
            int k = 0;
            int size = files[0].no_files;
            
            //pentru a face fisierul privat,mai intai trebuie sters din lista de fisiere partajate(acest lucru se realieaza prin shiftari la stanga)
            for (k = i;k <  size - 1;k++){
                 files[k] = files[k + 1];
            }
           

            for (k = 0; k < size - 1;k++){
                files[k].no_files = size - 1;   //actualizam numarul de fisiere din lista
            }
        
            strcpy(result,"200 Fisier a fost setat ca PRIVAT\n");   //trimitem clientului un mesaj de succes
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }

        
        return;

        
    }else{  //cazul in care fisierul nu exista in lista,se verifca daca exista macar in folder,deja privat
        
        int exists2 = 0;
        char *path = (char*)malloc(100*sizeof(char));
        memset(path,0,100);
    
        strcpy(path,"./");  //construim calea catre folder
        strcat(path,user);

        DIR *dir = opendir(path);
        struct dirent* d;   //deschidem folder-ul pentru citire
        int j = 0;


        if (dir != NULL){
                while (d = readdir(dir)){
                    if (j != 2){
                        j++;
                        continue;
                    }

                    if (strcmp(d->d_name,fileName) == 0){
                        exists2 = 1;
                        break;
                    }
                }
            closedir(dir);
        } 

        //daca fisierul nu exista,anuntam clientul
        if (exists2 == 0){    
            strcpy(result,"-4 Fisier inexistent\n");
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }
                    
            return ;
    
        //daca fisierul exista,inseamna ca este deja privat si anuntam clientul
        }else{
            strcpy(result,"-7 Fisier deja privat\n");
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }

            return;
        }
    }

}

//functie care executa stergerea unui fisier din folderul utilizatorului care a lansat comanda

void delete(int socket,char* user,SharedFiles* files,char* fileName){

    char* result = (char*)malloc(BUFLEN*sizeof(char));
    memset(result,0,BUFLEN);

    char *path = (char*)malloc(100*sizeof(char));
    memset(path,0,100);
    
    strcpy(path,"./");
    strcat(path,user);  //construim calea catre director
    
    int i = 0;
    int exists = 0;

    //verificam existenta fisierului in lista de fisiere partajate si verificam daca proprietarul este acelasi cu userul care a lansat comanda
    for(i = 0;i < files[0].no_files;i++){
        if (strcmp(files[i].name,fileName) == 0 && strcmp(files[i].owner,user) == 0){
            exists = 1;
            break;
        }
    }

    //daca fisierul exista,deoarece acesta va fi sters,trebuie mai intai scos din structura de date a fisierelor partajate
    if (exists == 1){

            int k = 0;
            int size = files[0].no_files;

            //executam stergerea din lista prin shiftari
             for (k = i;k <  size - 1;k++){
                 files[k] = files[k + 1];
            }
           
            //actualizam numarul de fisiere
            for (k = 0; k < size - 1;k++){
                files[k].no_files = size - 1;
            }

            //construim calea catre fisier
            strcat(path,"/");
            strcat(path,fileName);

            //stergem fisierul de la calea mai sus construita
            unlink(path);

            //trimitem un mesaj de succes clientului care a dat comanda
            strcpy(result,"200 Fisier sters\n");
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }

        
        return;

        
    //daca fisierul nu exista in lista
    }else{
        
        int exists2 = 0;

        DIR *dir=opendir(path);
        struct dirent* d;
        int j = 0;

        //verificam daca exista macar in folder
        if (dir != NULL){
            while (d = readdir(dir)){
                if (j != 2){
                    j++;
                    continue;
                }

                if (strcmp(d->d_name,fileName)==0){
                    exists2=1;
                    break;
                }
            }
        closedir(dir);
        } 

        //daca fisierul nu exista in folder,intoarcem mesajul de eroare -4
        if (exists2 == 0){
            strcpy(result,"-4 Fisier inexistent\n");
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }

        return ;
        
        //daca exista,il stergem direct din folder
        }else{

            strcat(path,"/");
            strcat(path,fileName);

            //stergem fisierul
            unlink(path);

            //anuntam clientul de succes-ul stergerii
            strcpy(result,"200 Fisier sters\n");
            int n = send(socket,result,strlen(result), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }
    
           return;
        }
    }

}


int main(int argc, char *argv[])
{

     FILE *f=NULL,*g=NULL;
        
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     char result[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j;
     char** isConnected=(char**)calloc(100,sizeof(char*));  //vector care retine daca si ce client este autentificat la un momentdat pe un anumit socket
     char* quit=(char*)malloc(100*sizeof(char));    //buffer care preia o comanda data de la tastatura server-ului
     int brute_force[100]; //contor pentru numarul succesiv de incercari de autentificare
     int isActive[100]; //vector care retine ce socketi sunt activi

     f=fopen(argv[2],"rt");
     g=fopen(argv[3],"rt");

     Users* users = createUsers(f); //construim lista de useri
     SharedFiles* files = createFiles(g,users);     //construim lista de fisiere partajate

     fd_set read_fds;	//multimea de file descriptori pentru citire
     fd_set tmp_fds;	//multime folosita temporar 
     int fdmax;		//valoare maxima file descriptor din multimea read_fds

     //golim multimea de descriptori de citire 
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     //intializam socket-ul pe care se asculta conexiuni
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     
     //ip-ul si port-ul pe care se realizeaza conexiunea
     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(atoi(argv[1]));
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd, MAX_CLIENTS);

     //adaugam file descriptor-ul socket-ului pe care se asculta conexiuni in multimea read_fds
     FD_SET(sockfd, &read_fds);

     //adaugam descriptorul 0 in multime,descriptor asociat citirii de la tastatura in cadrul serverului
     FD_SET(0,&read_fds);
     fdmax = sockfd;

	while (1) {
       
        tmp_fds = read_fds; 
	if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
		error("ERROR in select");
	   
        //se verifica daca am primit comanda quit de la server
        if(FD_ISSET(0, &tmp_fds)) {
            fgets(quit, 100 , stdin);
            if (strncmp(quit,"quit",4) == 0){
                int j = 0;
                //daca am primit,mai intai trebuie inchise toate conexiunile active cu clientii
                for(j = 1; j <= fdmax; j++){
                    if (isActive[j] == 1){ 
                        //inchiderea unui client se realizeaza prin trimiterea mesajului "close"
                        char* exitCom="close";
                        int m = send(j,exitCom,strlen(exitCom), 0);
                        if (m < 0) {
                            error("ERROR writing to socket");
                        }
                        //inchidem conexiunea si scoatem din multimea de descriptori pe care se asteapta mesaje,descriptorul clientului inchis
                        close(j); 
                        FD_CLR(j, &read_fds);
                    }
                }
                printf("Serverul se inchide!\n");
                fflush(stdout);
                exit(0);
            }
        }

        //parcurgem lista de socketi
		for(i = 1; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
			    //daca primim ceva pe socketul inactiv(o conexiune noua),o acceptam si introducem in lista de descriptori descriprotul asoctiat socket-ului nou
				if (i == sockfd) {
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					}else {
						FD_SET(newsockfd, &read_fds);
                        isActive[newsockfd] = 1;    //actualizam vectorul de socketi activi
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;   //actualizam numarul maxim al file descriptorilor
						}
					}
                }
                
                //daca primim ceva dar nu pe socket-ul inactiv,inseamna ca am primit mesaje pe unul din socketii cu care comunic cu clientii
                else{
					memset(buffer, 0, BUFLEN);
                    memset(result, 0, BUFLEN);
					
                    //primim mesajul si mai intai detectam daca clientul mai este conectat
                    if ((n = recv(i, buffer, BUFLEN, 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							brute_force[i]=0;
							printf("Clientul de pe socket-ul %d s-a deconectat\n", i);
                            fflush(stdout);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					} 
					
                    //daca este conectat prelucram mesajul primit
					else { //recv intoarce >0

                        //cazul in care primim o comanda de autentificare
                        if (strncmp(buffer,"login",5) == 0){
                            	char* name = (char*)malloc(sizeof(char)*100);
                           	    memset(name,0,100);
                             if (login(i,buffer,users,name,brute_force) == 1) { //daca login-ul este reusit
			    	             isConnected[i] = name;   //introducem in vectorul de useri autentificat pe un socket,noul user logat
                                 brute_force[i] = 0;     //resetam contorul de brute-force
			    	          }
			             }
				
                        
                        //daca primim logout si este cineva logat pe socketul de pe care am primit logout
                        if (strncmp(buffer,"logout",6) == 0 && strncmp(isConnected[i],"",1) != 0){
                            isConnected[i] = "";
                            strcpy(result,"Deconectare cu succes!\n");
                               n = send(i,result,strlen(result), 0);    //trimitem confirmarea de deconectare
    				            if (n < 0) {
       					            error("ERROR writing to socket");
   				                 }

                        }

                        //comanda getuserlist apeleaza functia cu acelasi nume
			            if (strncmp(buffer,"getuserlist",11) == 0){	 
				            getUserList(i,users);
			             }

                        //comanda getfilelist preia mai intai numele userului ale carui fisiere vrem sa le aflam si apoi apelam functia cu acelasi nume
                        if (strncmp(buffer,"getfilelist",11) == 0){     
                            char* name = (char*)malloc(sizeof(char)*100); 
                            getName(buffer,name);   
                        
                            getFileList(i,files,users,name);
                            free (name);
                        
                        }

                        //comanda share preia mai intai numele fisierului pe care vrem sa-l partajam si apoi apeleaza functia cu acelasi nume
                        if (strncmp(buffer,"share",5) == 0){ 
                            char* name = (char*)malloc(sizeof(char)*100); 
                            memset(name,0,100);
                            getName(buffer,name);   
                        
                            share(i,isConnected[i],files,name);
                            free(name);
                        }
                        
                        //comanda unshare preia mai intai numele fisierului pe care vrem sa-l facem privat si apoi apeleaza functia cu acelasi nume
                        if (strncmp(buffer,"unshare",7) == 0){      
                            char* name = (char*)malloc(sizeof(char)*100); 
                            memset(name,0,100);

                            getName(buffer,name);   
                            
                            unshare(i,isConnected[i],files,name);
                            free(name);
                        }

                        //comanda delete care preia mai intai numele fisierului pe care vrem sa-l stergem si apoi apeleaza functia cu acelasi nume
                        if (strncmp(buffer,"delete",6) == 0){  
                            char* name = (char*)malloc(sizeof(char)*100); 
                            memset(name,0,100);
                            getName(buffer,name);   

                            delete(i,isConnected[i],files,name);
                            free(name);
                        }

			

    										
					}
				}
             
			}
		}
     }

     close(sockfd);
   
     return 0; 
}


