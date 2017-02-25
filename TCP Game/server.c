#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int checkParity(char* m,int len){

	int i=0,j=0;
	int val=0;
	for(i=0;i<len;i++){	//pentru fiecare octet
		for(j=0;j<8;j++){	//se ia fiecare bit din el
			val+=(m[i]>>j) & 1;	//si se calculeaza numarul de biti de 1
		}
	}
	
	
	return val%2;	//se intoarce valoarea mod 2
}

void checkAckClient(msg r,msg s){
	
	//cat timp dim mesajul de ack/nack este 5 inseamna ca avem nack
	while (r.len==5){

		//retrimitem mesajul pana cand ne vine un ack
		send_message(&s);
		recv_message(&r);
		
	}

	

	return ;
}

msg checkMsg(msg r){

	msg s;
	
	//cat timp paritatea aflata in payload este diferita de valoarea calculata de functia noastra(valoarea corecta)
	while ((int)r.payload[0]!=checkParity(r.payload+1,r.len-1)){	
	
		//trimitem nack
		sprintf(s.payload,"%s","NACK");	
		s.len=strlen(s.payload)+1;
		
		send_message(&s);
		
		//asteptam sa vina mesajul corect
		recv_message(&r);
	}

	return r;
}




msg codifyHamming(msg r,char* m,int len){
		
		char tmp[2];	//variabila care retine cei 2 octeti cu biti de paritate+date
		int i=0;

		r.len=2*len;	//noua lungime
		
		for (i=0;i<len;i++){	//pentru fiecare octet din mesaj

			char byte=m[i];		//facem o copie pt ca altfel se pierde informatia(am avut o eroare destul de urata pana mi-am dat seama)

			tmp[0]=0;	//resetam cei 2 octeti
			tmp[1]=0;
				
			
			tmp[1] |= byte & 1;	//setam bitul D8		
			byte=byte>>1;
			
			tmp[1] |= (byte & 1) << 1;	//setam bitul D7
			byte=byte>>1;

			tmp[1] |= (byte & 1) << 2;	//setam bitul D6
			byte=byte>>1;

			tmp[1] |= (byte & 1) << 3;	//setam bitul D5
			byte=byte>>1;

			tmp[1] |= (byte & 1) << 5;	//stam bitul D4
			byte=byte>>1;

			tmp[1] |= (byte & 1) << 6;	//setam bitul D3
			byte=byte>>1;

			tmp[1] |= (byte & 1) << 7; 	//setam bitul D2
			byte=byte>>1;

			tmp[0] |= (byte & 1) << 1;	//setam bitul D1

			tmp[1] |=(((tmp[1] & 1) + ((tmp[1] >> 1) & 1) + ((tmp[1] >> 2) & 1) + ((tmp[1] >> 3) & 1)) % 2) << 4;	//calculam bitul P8 de paritate
	
			tmp[0] |=((tmp[1] & 1) + ((tmp[1] >> 5) & 1) + ((tmp[1] >> 6) & 1) + ((tmp[1] >> 7) & 1)) % 2;	//calculam bitul P4 pt paritate

			tmp[0] |=((((tmp[1] >> 1) & 1)+ ((tmp[1] >> 2) & 1) + ((tmp[1] >> 5) & 1) + ((tmp[1] >> 6) & 1) + ((tmp[0] >> 1) & 1)) % 2) << 2;	//calculam bitul P2 pt paritate
		
			tmp[0] |=((((tmp[1] >> 1) & 1)+ ((tmp[1] >> 3) & 1) + ((tmp[1] >> 5) & 1) + ((tmp[1] >> 7) & 1) + ((tmp[0] >> 1) & 1)) % 2) << 3;	//calculam bitul P1 pt paritate	

			r.payload[2*i]=tmp[0];	//un octet din mesaj va fi inlocuit de cei 2 octeti setati mai sus continand date si biti de paritate
			r.payload[2*i+1]=tmp[1];
		} 
	return r;
	
}

char decodifyHamming(char *m){

	char new_m=0;

	//decodificam mesajul extragand DOAR valoarea bitilor de date(D1,D2,.....,D8) si intoarcem acel mesaj

	new_m |= (m[0] >> 1) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 7) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 6) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 5) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 3) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 2) & 1;
	new_m = new_m << 1;
	new_m |= (m[1] >> 1) & 1;
	new_m = new_m << 1;
	new_m |= m[1] & 1;

	return new_m;
}

void repairHamming(char* m) {
/*	p4*8+p3*4+p2*2+p1 numar intre 4 si 12
	daca rezultatul e 3 inseamna ca d1 e problema si schimb
	daca rezultatul e 5 inseamna ca e d2
	daca e 6 e d3
	daca e 7 e d4
	daca e 9 e d5
	daca e 10 e d6
	daca e 11 e d7
	daca e 12 e d8
*/
	int P1=0,P2=0,P4=0,P8=0;

	P1=(((m[1] >> 1) & 1)+ ((m[1] >> 3) & 1) + ((m[1] >> 5) & 1) + ((m[1] >> 7) & 1) + ((m[0] >> 1) & 1)) % 2;	//calculam valoare lui P1
	P2=(((m[1] >> 1) & 1)+ ((m[1] >> 2) & 1) + ((m[1] >> 5) & 1) + ((m[1] >> 6) & 1) + ((m[0] >> 1) & 1)) % 2;	//calculam valoarea lui P2
	P4=((m[1] & 1) + ((m[1] >> 5) & 1) + ((m[1] >> 6) & 1) + ((m[1] >> 7) & 1)) % 2;	//calculam valoarea lui P4
	P8=((m[1] & 1) + ((m[1] >> 1) & 1) + ((m[1] >> 2) & 1) + ((m[1] >> 3) & 1)) % 2;	//calculam valoarea lui P8

	if (P1==((m[0]>>3) & 1)) 	//daca valoarea calculata de noi este egala cu valoarea existenta in cei 2 octeti inseamna ca bitul nu este corupt si il initializam cu 0
		P1=0;			//altfel initializam cu 1
	else P1=1;

	if (P2==((m[0]>>2) & 1)) 	//la fel pt P2
		P2=0;
	else P2=1;

	if (P4==(m[0] & 1)) 	//la fel pt P4
		P4=0;
	else P4=1;

	if (P8==((m[1]>>4) & 1)) 	//la fel pt P8
		P8=0;
	else P8=1;

	if (P1==0 && P2==0 && P4==0 && P8==0)	//daca toti bitii sunt 0,fisierul nu este corupt
		return ;

	int sum_correct=P8*8+P4*4+P2*2+P1; 	//calculam suma pt determinarea bitului care a cauzat eroarea
	
	if (sum_correct==3){	
		m[0] ^= 1 << 1;	//daca suma e 3,bitul D1 este vinovat si ii facem flip
	}
	if (sum_correct==5){
		m[1] ^= 1 << 7;	//daca suma e 5,bitul D2 este vinovat si ii facem flip
	}
	if (sum_correct==6){
		m[1] ^= 1 << 6;	//daca suma e 6,bitul D3 este vinovat si ii facem flip
	}
	if (sum_correct==7){
		m[1] ^= 1 << 5;	//daca suma e 7,bitul D4 este vinovat si ii facem flip
	}
	if (sum_correct==9){
		m[1] ^= 1 << 4;	//daca suma e 9,bitul D5 este vinovat si ii facem flip
	}
	if (sum_correct==10){
		m[1] ^= 1 << 3;	//daca suma e 10,bitul D6 este vinovat si ii facem flip
	}
	if (sum_correct==11){
		m[1] ^= 1 << 2;	//daca suma e 11,bitul D7 este vinovat si ii facem flip
	}
	if (sum_correct==12){
		m[1] ^= 1 << 1;	//daca suma e 12,bitul D8 este vinovat si ii facem flip
	}

	return;
		
	
}


void binarySearchSimple(msg r,msg s,char* m){

	int start=0;
	int end=999;
	int middle=(start+end)/2;
	
	//trimitem primul numar

	sprintf(m,"%d",middle);
	memcpy(s.payload,m,strlen(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim primul indiciu

	recv_message(&r);

	//cat timp nu primim succes

	while(strncmp(r.payload,"success",7)!=0){
	
		//in functie de indiciul primit intram pe unul din cazuri
	
		if (strncmp(r.payload,"bigger",6)==0){
			start=middle;
			middle=(start+end)/2;			
			sprintf(m,"%d",middle);
			
		}
		if (strncmp(r.payload,"smaller",7)==0){
			end=middle;
			middle=(start+end)/2;
			sprintf(m,"%d",middle);
			
		}

		//trimitem noul numar		

		memcpy(s.payload,m,strlen(m));
		s.len=strlen(s.payload)+1;
		send_message(&s);

		//primim noul indiciu

		recv_message(&r);
		printf("%s",r.payload);	

	}

	return ;
}

void binarySearchACK(msg r,msg s,char* m){

	int start=0;	
	int end=999;
	int middle=(start+end)/2;
	
	//trimitem primul numar

	sprintf(m,"%d",middle);
	memcpy(s.payload,m,strlen(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim ack

	recv_message(&r);

	//primim indiciul

	recv_message(&r);

	//cat timp nu primim succes

	while(strncmp(r.payload,"success",7)!=0){

		//trimitem ack	

		strcpy(m,"ACK");
		memcpy(s.payload,m,strlen(m));
		s.len=strlen(s.payload)+1;

		send_message(&s);
	
		//alegem unul din cazuri in functie de indiciu

		if (strncmp(r.payload,"bigger",6)==0){
			start=middle;
			middle=(start+end)/2;			
			sprintf(m,"%d",middle);
		}

		if (strncmp(r.payload,"smaller",7)==0){
			end=middle;
			middle=(start+end)/2;
			sprintf(m,"%d",middle);
			
		}		

		//trimitem noul numar

		memcpy(s.payload,m,strlen(m));
		s.len=strlen(s.payload)+1;
		send_message(&s);

		//primim ack

		recv_message(&r);

		//primim noul indiciu

		recv_message(&r);
		printf("%s",r.payload);	


	}

	return ;
}


void binarySearchParity(msg r,msg s){

	
	int start=0;
	int end=999;
	int middle=(start+end)/2;
	
	//trimitem primul numar + bitul de paritate	

	sprintf(s.payload+1,"%d",middle);
	s.len=strlen(s.payload+1)+2;	
	s.payload[0]=checkParity(s.payload+1,s.len-2);
	
	send_message(&s);

	//primim ack/nack si eventual facem o retrimitere

	recv_message(&r);
	checkAckClient(r,s);
	
	//primim si verificam mesajul care contine primul indiciu

	recv_message(&r);
	r=checkMsg(r);
	printf("%s",r.payload+1);

	//cat timp nu avem succes
	while(strncmp(r.payload+1,"success",7)!=0){

		//trimitem ack
		sprintf(s.payload,"%s","ACK");
		s.len=strlen(s.payload)+1;

		send_message(&s);
	
		//intram pe unul din cazuri in functie de indiciul primit
		if (strncmp(r.payload+1,"bigger",6)==0){		
			start=middle;
			middle=(start+end)/2;			
			
		}
		if (strncmp(r.payload+1,"smaller",7)==0){	
			end=middle;
			middle=(start+end)/2;
			
		}		

		//trimitem noul numar + bitul de payload
		sprintf(s.payload+1,"%d",middle);
		s.len=strlen(s.payload+1)+2;
		s.payload[0]=checkParity(s.payload+1,s.len-2);
		send_message(&s);

		//primim ack/nack si eventual retrimitem mesajul
		recv_message(&r);
		checkAckClient(r,s);

		//primim si verificam mesajul cu noul indiciu
		recv_message(&r);
		r=checkMsg(r);
		printf("%s",r.payload+1);	

	}

	return ;
	
}


void binarySearchHamming(msg r,msg s){

	int start=0;
	int end=999;
	int middle=(start+end)/2;
	char m[100];
	char n[100];
	int j=0;

	//trimitem primul numar codificat

	sprintf(m,"%d",middle);
	s=codifyHamming(s,m,strlen(m)+1);
	
	send_message(&s);

	//primim ack + eventuala recorectare

	recv_message(&r);
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);
	}

	//primim primul indiciu + recorectarea mesajului
	//variabila n contine mesajul corectat si decodificat aflat initial in payload

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		
		repairHamming(r.payload+2*j);
		printf("%c",decodifyHamming(r.payload+2*j));
		n[j]=decodifyHamming(r.payload+2*j);
	}
	printf("\n");

	//cat timp nu intalnim succes
	while(strncmp(n,"success",7)!=0){

		//trimitem ack
		strcpy(m,"ACK");
		codifyHamming(s,m,strlen(m));
		send_message(&s);
	
		//intram pe unul din cazuri in functie de indiciul primit
		if (strncmp(n,"bigger",6)==0){		
			start=middle;
			middle=(start+end)/2;			
			
		}
		if (strncmp(n,"smaller",7)==0){	
			end=middle;
			middle=(start+end)/2;
	
		}		

		//codificam si trimitem noul numar

		sprintf(m,"%d",middle);
		s=codifyHamming(s,m,strlen(m)+1);
		send_message(&s);
	
		//primim ack + recorectare

		recv_message(&r);
		for (j=0;j<r.len/2;j++){
			repairHamming(r.payload+2*j);
		}

		//primim noul indiciu pe care il recorectam si apoi decodificam

		recv_message(&r);
		for (j=0;j<r.len/2;j++){
			repairHamming(r.payload+2*j);
			printf("%c",decodifyHamming(r.payload+2*j));
			n[j]=decodifyHamming(r.payload+2*j);
		}
		printf("\n");
	}
	return ;
}



void simpleProg(msg r,msg s){

	char m[100];	//buffer pentru mesaje
	
	//primim mesajul cu "Hello"

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem Hello
		
	strcpy(m,"Hello");
	memcpy(s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim un mesaj care ne anunta ca urmeaza sa primim alte 2 mesaje

	recv_message(&r);
	printf("%s",r.payload);

	//primim primul mesaj

	recv_message(&r);
	printf("%s",r.payload);

	//primim al doilea mesaj care contine alte instructiuni

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem YEY

	strcpy(m,"YEY");
	memcpy(s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//trimitem OK	

	strcpy(m,"OK");
	memcpy(s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim mesajul cf caruia incepem jocul

	recv_message(&r);
	printf("%s",r.payload);

	//functie care gaseste solutia jocului

	binarySearchSimple(r,s,m);

	//primim un nou mesaj

	recv_message(&r);
	printf("%s",r.payload);	

	//trimitem exit pt a iesii

	strcpy(m,"exit");
	memcpy(s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim ultimul mesaj

	recv_message(&r);
	printf("%s",r.payload);

	return ;
}

void ackProg(msg r,msg s){

	char m[100];	//buffer pentru mesaj

	//receptionam mesajul de trimis "Hello"

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);
	
	//trimitem Hello

	strcpy(m,"Hello");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim ack

	recv_message(&r);

	//primim urmatorul mesaj care spune ca urmeaza sa primim inca 2 alte mesaje

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack	

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//primim I mesaj

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//primim al doilea mesaj

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//trimitem YEY

	strcpy(m,"YEY");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//primim ack

	recv_message(&r);
	
	//trimitem OK

	strcpy(m,"OK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//primim ack

	recv_message(&r);
	
	//primim mesajul cf caruia incepe jocul

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//functie care gaseste solutia jocului utilizand cautarea binara

	binarySearchACK(r,s,m);

	//trimitem ack ca am primit succes

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//primim un nou mesaj

	recv_message(&r);
	printf("%s",r.payload);

	//trimitem ack

	strcpy(m,"ACK");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);

	//iesim din joc

	strcpy(m,"exit");
	memcpy(&s.payload,&m,sizeof(m));
	s.len=strlen(s.payload)+1;

	send_message(&s);
	
	//primim ack

	recv_message(&r);

	//primim ultimul mesaj

	recv_message(&r);
	printf("%s",r.payload);
	
	return ;
}

void parityProg(msg r,msg s){

	//primim si verificam mesajul cu hello

	recv_message(&r);
	
	r=checkMsg(r);

	printf("%s",r.payload+1);
		
	//trimitem ack

	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//trimitem mesajul Hello+paritatea acestuia

	sprintf(s.payload+1,"%s","Hello");
	s.len=strlen(s.payload)+1;
	s.payload[0]=checkParity(s.payload+1,s.len-1);
	
	send_message(&s);

	//verificam daca am primit ack sau nack si eventual retrimitem mesajul

	recv_message(&r);
	
	checkAckClient(r,s);

	//primim noul mesaj care ne anunta ca vom primii inca 2 mesaje+verificam mesajul pt corectitudine

	recv_message(&r);

	r=checkMsg(r);

	printf("%s",r.payload+1);

	//trimitem ack
		
	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim primul mesaj + facem verificari

	recv_message(&r);

	r=checkMsg(r);

	printf("%s",r.payload+1);

	//trimitem ack

	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim al doilea mesaj cu noi instructiuni + facem verificari

	recv_message(&r);

	r=checkMsg(r);

	printf("%s",r.payload+1);

	//trimitem ack
	
	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//trimitem mesajul YEY

	sprintf(s.payload+1,"%s","YEY");
	s.len=strlen(s.payload)+1;
	s.payload[0]=checkParity(s.payload+1,s.len-1);

	send_message(&s);

	//primim ack/nack si eventual retrimitem mesajul

	recv_message(&r);

	checkAckClient(r,s);

	//trimitem OK

	sprintf(s.payload+1,"%s","OK");
	s.len=strlen(s.payload)+1;
	s.payload[0]=checkParity(s.payload+1,s.len-1);
	
	send_message(&s);

	//primim ack/nack si eventual retrimitem mesajul

	recv_message(&r);

	checkAckClient(r,s);

	//primim mesajul cf caruia incepem jocul+verificarea acestuia
	
	recv_message(&r);
	
	r=checkMsg(r);

	printf("%s",r.payload+1);

	//trimitem ack
	
	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//functie care calculeaza solutia jocului

	binarySearchParity(r,s);

	//trimitem ack ca am primit succes

	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);

	//primim un nou mesaj

	recv_message(&r);
	r=checkMsg(r);
	printf("%s",r.payload+1);

	//trimitem ack

	sprintf(s.payload,"%s","ACK");
	s.len=strlen(s.payload)+1;
	
	send_message(&s);	

	//trimitem exit

	sprintf(s.payload+1,"%s","exit");
	s.len=strlen(s.payload)+1;
	s.payload[0]=checkParity(s.payload,s.len);

	send_message(&s);

	//primim ack/nack si eventual retrimitem mesajul

	recv_message(&r);
	checkAckClient(r,s);

	//primim ultimul mesaj

	recv_message(&r);
	r=checkMsg(r);	
	printf("%s",r.payload+1);

	

	return ;
}

void hammingProg(msg r,msg s){
	
	int j=0;
	char m[100];	//buffer pentru mesaje

	//primim mesajul,il decodificam si il corectam de eventualele erori

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));

	send_message(&s);
	
	//trimitem Hello

	strcpy(m,"Hello");
	s=codifyHamming(s,m,strlen(m));
	
	send_message(&s);

	//primim ack+corectarea eventualelor erori

	recv_message(&r);
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);
	}	
	
	//primim un nou mesaj care ne spune ca urmeaza sa primim alte 2 mesaje

	recv_message(&r);

	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));

	send_message(&s);

	//primim primul mesaj+eventuala corectare

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));
	
	send_message(&s);

	//al doilea mesaje cu noi instructiuni+eventuala corectie de erori

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));
	
	send_message(&s);

	//trimitem YEY

	strcpy(m,"YEY");
	s=codifyHamming(s,m,strlen(m));

	send_message(&s);

	//primim ack + eventuala corectare

	recv_message(&r);
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
	}
	
	//trimitem OK

	strcpy(m,"OK");
	s=codifyHamming(s,m,strlen(m));

	send_message(&s);

	//primim ack + eventuala corectare de erori

	recv_message(&r);
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
	}
	
	//primim mesajul cf caruia incepem sa jucam jocul

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){	
		repairHamming(r.payload+2*j);
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));
	send_message(&s);

	//functie care gaseste solutia jocului folosind cautarea binara

	binarySearchHamming(r,s);

	//trimitem ack pentru succes

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));
	send_message(&s);

	//primim un nou mesaj + eventualele corectari

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	//trimitem ack

	strcpy(m,"ACK");
	s=codifyHamming(s,m,strlen(m));
	send_message(&s);

	//trimitem exit

	strcpy(m,"exit");
	s=codifyHamming(s,m,strlen(m));
	send_message(&s);

	//primim ack	+ corectarea eventualelor erori

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
	}
	printf("\n");

	//primim ultimul mesaj+corectarea lui

	recv_message(&r);
	
	for (j=0;j<r.len/2;j++){
		repairHamming(r.payload+2*j);	
		printf("%c",decodifyHamming(r.payload+2*j));
	}
	printf("\n");

	return ;
}


int main(int argc,char *argv[])
{
	msg r,s;
	
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	//daca nu exista argumente inseamna ca trebuie rulat modul 1 de lucru:simplu
	if (argc==1)
		simpleProg(r,s);
	else{
		if (strcmp(argv[1],"ack")==0)	//ack-modul 2 de lucru:confirmare trimitere-primire mesaje
			ackProg(r,s);
		if (strcmp(argv[1],"parity")==0)	//parity-modul 3 de lucru:verificarea paritatii intregului String
			parityProg(r,s);
		if (strcmp(argv[1],"hamming")==0)	//hamming-modul 4 de lucru:verificarea si corectarea erorilor utilizand codul hamming
			hammingProg(r,s);
	}
		
	return 0;
}
