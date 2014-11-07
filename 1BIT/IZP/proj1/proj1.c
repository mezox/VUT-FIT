/* Subor:   proj1.c
 * Datum:   2011/11/01
 * Autor:   Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
 * Projekt: Prevod casovych udajov, projekt c. 1 pro predmet IZP
 * Popis:   Program prevadza zadane cislo (v sekundach)
            na zvolenu casovu jednotku (tyzdne, hodiny, minuty)...*/

#include <stdio.h>     // praca so vstupom/vystupom
#include <stdlib.h>    // vseobecne funkcie jazyka C
#include <string.h>    // kvoli funkcii strcmp
#include <ctype.h>     // testovanie znakov - isalpha, isdigit, atd.
#include <limits.h>    // limity ciselnych typov

/** Deklaracia premennych */
unsigned long int num,vys[5];	
int c,			// getchar
		p,i,j,pom;	// riadiace premenne
const unsigned long int CAS_SEC[4] = {604800, 86400, 3600, 60};

// Kody chyb programu
enum tecodes{
  EOK,
  ENOTDIGIT,
  ECLWRONG,
  EPCWRONG,
  EOFLOW,
};

// Stavove kody programu
enum tstates{
  CWEEK,       // -t
  CDAY,        // -d
  CHOUR,       // -h
  CMIN,        // -m
  CSEC,        // -s
  CHELP,       // Napoveda
};

/** Chybove hlasenia odpovedajuce chybovym kodom */
const char *ECODEMSG[] =
{
  [EOK] = "Vsetko v poriadku.\n",
  [ENOTDIGIT] = "Neplatny znak.\n",
  [ECLWRONG] = "Chybne parametre prikazoveho riadku!\n",
  [EPCWRONG] = "Prilis vela parametrov prikazoveho riadku!\n",
  [EOFLOW] = "Zadane cislo je mimo rozsah premennej!\n",
};  

const char *HELPMSG =
  "Program Prevod casovych udajov.\n"
  "Autor: Tomas Kubovcik (c) 2011\n"
  "Program nacita cislo zo standardneho vstupu (v sekundach)\n"
  "a prevadza ho na zadanu jednotku...\n"
  "Pouzitie: proj1 --help zobrazi napovedu\n"
  "          proj1 -t  zobrazi zadane cislo v tyzdnoch\n"
  "          proj1 -d  zobrazi zadane cislo v dnoch\n"
  "          proj1 -h  zobrazi zadane cislo v hodinach\n"
  "          proj1 -m  zobrazi zadane cislo v minutach\n"
  "          proj1 -s  zobrazi zadane cislo v sekundach\n"
;

const char *CAS_TAB[5][3] = 
{
	{"tydnu","tyden","tydny"},
	{"dni","den","dny"},
	{"hodin","hodina","hodiny"},
	{"minut","minuta","minuty"},
	{"sekund","sekunda","sekundy"},
};

typedef struct params{
  int ecode;        // Chybovy kod programu, odpovedajuci tecodes
  int state;        // Stavovy kod programu, odpovedajuci tstates
} TParams;

/** Nacitanie parametrov */
TParams getParams(int argc, char *argv[]){
  TParams result =
  {
   .ecode = EOK,
   .state = CWEEK,
  };

	if (argc == 1){				// vstup bez parametrov
		result.state = CWEEK;    
	}
	else if (argc == 2){		// vstup s 1 parametrom
		if (strcmp("--help", argv[1]) == 0){
			result.state = CHELP;		// vypis napovedy
		}
		else if (strcmp("-t", argv[1]) == 0){
			result.state = CWEEK;		// vypis v tyzdnoch
		}
		else if (strcmp("-d", argv[1]) == 0){
			result.state = CDAY;		// vypis v dnoch
		}
		else if (strcmp("-h", argv[1]) == 0){
			result.state = CHOUR;		// vypis v hodinach
		}
		else if (strcmp("-m", argv[1]) == 0){
			result.state = CMIN;		// vypis v minutach
		}
		else if (strcmp("-s", argv[1]) == 0){   
			result.state = CSEC;		// vypis v sekundach
		}
		else result.ecode = ECLWRONG;	// chybne parametre
	}
	else result.ecode = EPCWRONG; // vela parametrov
return result;      
}

/** Vypocet casovych udajov */
unsigned long int vypocet(int i){
  while (num<CAS_SEC[i] && i<=3){		/** preskoci nepotrebne vypocty */
	i++;															/** je zadane cislo mensie ako ak */
  }																	/** konstanta odpovedajuca */
  pom = i;													/** volanemu parametru */
  
  for (j = i; j <= 4; j++){
	if (num != 0 && j < 4){
		vys[j] = num/CAS_SEC[j];
		num = num %CAS_SEC[j];
	}
	else vys[4] = num %CAS_SEC[3];
  }
return num;
}

/** Vypis vypocitanych casovych udajov */
void vypis(int i){
  for (j = i; j <= 4; j++){							/** vlozi medzery, osetri vlozenie*/
	if (vys[j]>0 && j != i) printf(" ");	/** medzery pred prvu hodnotu */

	if (vys[j] == 1){
		printf("%lu %s",vys[j],CAS_TAB[j][1]);
	}
	else if (vys[j]>1 && vys[j]<5){
		printf("%lu %s",vys[j],CAS_TAB[j][2]);
	}
	else if (vys[j]>4){
		printf("%lu %s",vys[j],CAS_TAB[j][0]);
	}
  }
  if (vys[4] == 0 && p == 1) printf("%lu %s",vys[4],CAS_TAB[4][0]);
}

/** Nacitanie a spracovanie cisla z prikazoveho riadku */
TParams input(void){
  TParams result;

  while ((c=getchar()) != '\n' && result.ecode == EOK && c != EOF){
	if (isdigit(c) != 0){							// zisti ci bolo zadane cislo
		if (num>(ULONG_MAX-(c-48))/10){
			result.ecode = EOFLOW;				// pretecenie, vracia chybovy stav
		}
		else {
			num = num * 10 + c - 48;
			if (num == 0) p = 1;				// kontrola, ci bolo zadane cislo
		}
	}
	else result.ecode = ENOTDIGIT; // neplatny znak na vstupe, vracia chybovy stav
  }
return result;
}

/** Hlavny program */
int main(int argc, char *argv[]){
	TParams params = getParams(argc, argv);
	
	if (params.state == CHELP){			// vypise napovedu k programu
		printf("%s", HELPMSG);
		return EXIT_SUCCESS;
	}
	else if (params.ecode == EOK){	/** ak boli zadane spravne parametre */
		TParams params = input();			/** dojde k nacitaniu */
			if (params.ecode == EOK){		/** prevedie nacitane cislo na zvolene */
				vypocet(params.state);		/** casove jednotky ak pri vstupe */
				vypis(pom);								/** nedoslo k chybe */
			}
			else fprintf(stderr, "%s", ECODEMSG[params.ecode]);	/** vypis */
	}																												/** chybovych */
	else fprintf(stderr, "%s", ECODEMSG[params.ecode]);			/** stavov */
 return EXIT_SUCCESS;
}
