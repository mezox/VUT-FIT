/* Datum:   2011/12/14
 * Subor:   proj4.c
 * Autor:   Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
 * Projekt: Ceske radenie, projekt c. 4 pre predmet IZP
 * Popis:   Program nacita postupnost slov zo vstupneho suboru a 
            zoradi ich podla ceskej abecedy. */

#include <locale.h>	// lokalizacia
#include <wchar.h>	// wide char
#include <stdio.h>	// praca so vstupom/vystupom
#include <stdlib.h>	// vseobecne funkcie jazyka C
#include <string.h>	// kvoli funkcii strcmp
#include <ctype.h>	// testovanie znakov - isalpha, isdigit, atd.
#include <errno.h>	
#include <assert.h>	

#define ROVNE 0
#define MENSIE -1
#define VACSIE 1

/** Kody chyb programu */
enum tecodes
{
    EOK,	// Vsetko OK
    ENOTDIGIT,	// Zadana hodnota nie je cislica
    ECLWRONG,	// Chybny parameter
    EPCWRONG,	// Chybny pocet parametrov
    EFFAILED,	// Chyba pri otvarani suboru
    EOUTOFMEM,	// Nedostatok pamati
	EBADCODING, // Zly parameter volby kodovania
    EUNKNOWN,	// Neznama chyba
};

/** Stavove kody programu */
enum tstates
{
    HELP,   	// Napoveda
    DEFAULT,	// Standardny stav
};

/** Chybove hlasenia odpovedajuce chybovym kodom */
const char *ECODEMSG[] =
{
    [EOK] = "Vsetko v poriadku.\n",
    [ENOTDIGIT] = "Zadany parameter obsahuje neplatny znak vstupu.\n",
    [ECLWRONG] = "Chybne parametre prikazoveho riadku!\n",
    [EPCWRONG] = "Chybny pocet parametrov prikazoveho riadku!\n",
    [EFFAILED] = "Nastala chyba pri otvarani suboru!\n",
    [EOUTOFMEM]	= "Nedostatok pamati!\n",
	[EBADCODING] = "Nespravny parameter formatu kodovania!\n",
    [EUNKNOWN] = "Nastala nepredvidatelna chyba!\n",
};
  
/**	Napoveda */
const char *HELPMSG =
    "Program Ceske radenie.\n"
    "Autor: Tomas Kubovcik (c) 2011\n"
    "Program nacita postupnost slov zo vstupneho suboru\n"
    "a zoradi ich podla ceskej abecedy.\n"
    "Pouzitie:\n"
    "proj4 -h                                     - zobrazi napovedu\n"
    "proj4 [--loc LOCALE] subor1.txt subor2.txt   - nepovinny parameter LOCALE\n"
    "                                               specifikuje kodovanie vstup.\n"
    "                                               suboru.\n"
;

/* Tabulky pre 1. a 2. priechod */
const int firstTable[] = {
  	['\0'] = 0,
  	[L'A'] = 1, [L'a'] = 1, [L'\u00c1'] = 1, [L'\u00e1'] = 1,				// ['A'],['a'],['Á'],['á']
  	[L'B'] = 2, [L'b'] = 2,													// ['B'],['b']
  	[L'C'] = 3, [L'c'] = 3, 												// ['C'],['c']
  	[L'\u010c'] = 4, [L'\u010d'] = 4,										// ['Č'],['č']
  	[L'D'] = 5, [L'd'] = 5, [L'\u010e'] = 5, [L'\u010f'] = 5,				// ['D'],['d'],['Ď'],['ď']
  	[L'E'] = 6, [L'e'] = 6, [L'\u00c9'] = 6, [L'\u00e9'] = 6, 				// ['E'],['e'],['É'],['é']
	[L'\u011a'] = 6, [L'\u011b'] = 6,										// ['É'],['é']
  	[L'F'] = 7, [L'f'] = 7,													// ['F'],['f']
  	[L'G'] = 8, [L'g'] = 8,													// ['G'],['g']
  	[L'H'] = 9, [L'h'] = 9,													// ['H'],['h']
  	[L'©'] = 10, [L'@'] = 10,												// ['CH'],['ch']
  	[L'I'] = 11, [L'i'] = 11, [L'\u00cd'] = 11, [L'\u00ed'] = 11,			// ['I'],['i'],['Í'],['í']
  	[L'J'] = 12, [L'j'] = 12,												// ['J'],['j']
  	[L'K'] = 13, [L'k'] = 13,												// ['K'],['k']
  	[L'L'] = 14, [L'l'] = 14,												// ['L'],['l']
  	[L'M'] = 15, [L'm'] = 15,												// ['M'],['m']
  	[L'N'] = 16, [L'n'] = 16, [L'\u0147'] = 16, [L'\u0148'] = 16,			// ['N'],['n'],['Ň'],['ň']
  	[L'O'] = 17, [L'o'] = 17, [L'\u00d3'] = 17, [L'\u00f3'] = 17,			// ['O'],['o'],['Ó'],['ó']
  	[L'P'] = 18, [L'p'] = 18,												// ['P'],['p']
  	[L'Q'] = 19, [L'q'] = 19,												// ['Q'],['q']
  	[L'R'] = 20, [L'r'] = 20, 												// ['R'],['r']
  	[L'\u0158'] = 21, [L'\u0159'] = 21,										// ['Ř'],['ř']
  	[L'S'] = 22, [L's'] = 22, [L'\u0160'] = 22, [L'\u0161'] = 22,			// ['S'],['s'],['Š'],['š']
  	[L'T'] = 24, [L't'] = 24, [L'\u0164'] = 24, [L'\u0165'] = 24,			// ['T'],['t'],['Ť'],['ť']
  	[L'U'] = 25, [L'u'] = 25, [L'\u00da'] = 25, [L'\u00fa'] = 25,			// ['U'],['u'],['Ú'],['ú']
	[L'\u016e'] = 25, [L'\u016f'] = 25,										// ['Ů'],['ů']
  	[L'V'] = 26, [L'v'] = 26,												// ['V'],['v']
  	[L'W'] = 27, [L'w'] = 27,												// ['W'],['w']
  	[L'X'] = 28, [L'x'] = 28,												// ['X'],['x']
  	[L'Y'] = 29, [L'y'] = 29, [L'\u00dd'] = 29, [L'\u00fd'] = 29,			// ['Y'],['y'],['Ý'],['ý']
  	[L'Z'] = 30, [L'z'] = 30, 												// ['Z'],['z']
  	[L'\u017d'] = 31, [L'\u017e'] = 31,										// ['Ž'],['ž']
  	[L' '] = 42
};

const int secondTable[] = {
	['\0'] = 0,
	[L'A'] = 1, [L'a'] = 1,						// ['A'],['a']
	[L'\u00c1'] = 2, [L'\u00e1'] = 2,			// ['Á'],['á']
	[L'B'] = 3, [L'b'] = 3,						// ['B'],['b']
	[L'C'] = 4, [L'c'] = 4, 					// ['C'],['c']
	[L'\u010c'] = 5, [L'\u010d'] = 5,			// ['Č'],['č']
	[L'D'] = 6, [L'd'] = 6,						// ['D'],['d']
	[L'\u010e'] = 7, [L'\u010f'] = 7,			// ['Ď'],['ď']
	[L'E'] = 8, [L'e'] = 8,						// ['E'],['e']
	[L'\u00c9'] = 9, [L'\u00e9'] = 9,			// ['É'],['é']
	[L'\u011a'] = 10, [L'\u011b'] = 10,			// ['É'],['é']
	[L'F'] = 11, [L'f'] = 11,					// ['F'],['f']
	[L'G'] = 12, [L'g'] = 12,					// ['G'],['g']
	[L'H'] = 13, [L'h'] = 13,					// ['H'],['h']
  	[L'I'] = 15, [L'i'] = 15,					// ['I'],['i']
  	[169] = 14,									// ['CH'],['ch']
  	[L'\u00cd'] = 16, [L'\u00ed'] = 16,			// ['Í'],['í']
  	[L'J'] = 17, [L'j'] = 17,					// ['J'],['j']
  	[L'K'] = 18, [L'k'] = 18,					// ['K'],['k']
  	[L'L'] = 19, [L'l'] = 19,					// ['L'],['l']
  	[L'M'] = 20, [L'm'] = 20,					// ['M'],['m']
  	[L'N'] = 21, [L'n'] = 21,					// ['N'],['n']
  	[L'\u0147'] = 22, [L'\u0148'] = 22,			// ['Ň'],['ň']
  	[L'O'] = 23, [L'o'] = 23,					// ['O'],['o']
  	[L'\u00d3'] = 24, [L'\u00f3'] = 24,			// ['Ó'],['ó']
  	[L'P'] = 25, [L'p'] = 25,					// ['P'],['p']
  	[L'Q'] = 26, [L'q'] = 26,					// ['Q'],['q']
	[L'R'] = 27, [L'r'] = 27, 					// ['R'],['r']
	[L'\u0158'] = 28, [L'\u0159'] = 28,			// ['Ř'],['ř']
	[L'S'] = 29, [L's'] = 29,					// ['S'],['s']
	[L'\u0160'] = 30, [L'\u0161'] = 30,			// ['Š'],['š']
	[L'T'] = 31, [L't'] = 31,					// ['T'],['t']
	[L'\u0164'] = 32, [L'\u0165'] = 32,			// ['Ť'],['ť']
  	[L'U'] = 33, [L'u'] = 33, 					// ['U'],['u']
  	[L'\u00da'] = 34, [L'\u00fa'] = 34,			// ['Ú'],['ú'],
  	[L'\u016e'] = 35, [L'\u016f'] = 35,			// ['Ů'],['ů']
  	[L'V'] = 36, [L'v'] = 36,					// ['V'],['v']
  	[L'W'] = 37, [L'w'] = 37,					// ['W'],['w']
  	[L'X'] = 38, [L'x'] = 38,					// ['X'],['x']
  	[L'Y'] = 39, [L'y'] = 39,					// ['Y'],['y']
  	[L'\u00dd'] = 40, [L'\u00fd'] = 40,			// ['Ý'],['ý']
  	[L'Z'] = 41, [L'z'] = 41, 					// ['Z'],['z']
  	[L'\u017d'] = 42, [L'\u017e'] = 42,			// ['Ž'],['ž']
  	[L' '] = 53, 
};

/* Definovanie typov a struktur */
typedef struct 
{
    int ecode;      // chybovy kod programu
    int state;      // stavovy kod programu
    char* in;		// vstupny subor
    char* out;		// vystupny subor
} TParams;

/* Linearne viazany zoznam */
typedef struct item titem;
struct item {
	wchar_t* riadok;
	titem *next;
};

/*	Nacitanie parametrov */
TParams getParams(int argc, char *argv[])
{
    TParams result =
    {
        .ecode = EOK,
    };

/** Priradenie prislusnych stavov v zavislosti od zadanych parametrov, */
/** osetrenie neplatnych vstupov pre parametre prikazoveho riadku, */
/** konverzia parametrov na vhodne udajove typy */

    if(argc == 2)
    {
	if(strcmp("-h", argv[1]) == 0)
    	result.state = HELP;
	else result.ecode = ECLWRONG;
    }
    else if(argc == 3)
    {
    	setlocale(LC_ALL, "");
    	
        result.state = DEFAULT;
        result.in = argv[1];
        result.out = argv[2];
    }
    else if(argc == 5)
    {	
	if(strcmp("--loc", argv[1]) == 0)
	{
		if(setlocale(LC_ALL, argv[2]) == NULL)
			result.ecode = EBADCODING;
		
	    result.state = DEFAULT;
	    result.in = argv[3];
	    result.out = argv[4];
	}		
	else result.ecode = ECLWRONG;
    }
    else result.ecode = EPCWRONG;

return result;      
}

/* Porovnanie stringov */
int cmpstr(wchar_t* loaded, wchar_t* saved)
{
	int poradie = ROVNE,
		dlzka = 0,
		dlzka1 = 0,
		dlzka2 = 0;
					
	// zisti dlzku nacitaneho retazca
	while(loaded[dlzka1] != L'\0') dlzka1++;
	
	// zisti dlzku aktualneho retazca
	while(saved[dlzka2] != L'\0') dlzka2++;
		
	if(dlzka1<dlzka2) dlzka = dlzka1;
	else dlzka = dlzka2;
	
	int i = 0, j = 0; 
	
	// prvy priechod
		
	while(firstTable[loaded[i]] == firstTable[saved[i]] && loaded[i] != L'\0')
	{
		i++;	//zisti kolko znakov je rovnakych
	}
	
	if(firstTable[loaded[i]] > firstTable[saved[i]])	// porovna nasledujuci znak
		poradie = MENSIE;
	else poradie = VACSIE;		

	if(i == dlzka2 && dlzka1 == dlzka2) poradie = ROVNE;
	
	// druhy priechod, vacsi doraz na diakritiku
	if(poradie == ROVNE)
	{
		while(j < dlzka)
		{
			if(secondTable[loaded[j]] < secondTable[saved[j]])
			{
				poradie = VACSIE; break;
			}
			else if(secondTable[loaded[j]] > secondTable[saved[j]])
			{
				poradie = MENSIE; break;
			}
			else j++;	
		}
	}
	return poradie;	 
}

/* Nacitanie riadku */
int readLine(wchar_t **pLine, unsigned int *bufsize, FILE* in, wint_t c)
{
  	const int BUFINC = 16;

  	if(*pLine == NULL)
  	{
    	*bufsize = BUFINC;
    	*pLine = malloc(*bufsize*sizeof(wchar_t));		// alokuje pociatocnu pamat
    
    	if (pLine == NULL)
      		*bufsize = 0;
  	}
  	
	unsigned int i = 0;
	(*pLine)[i] = c;
	i++;
	
  	while((c = fgetwc(in)) != L'\n')
  	{
    	if(i == *bufsize-1)
    	{ 
      		*bufsize <<= 1;
      		wchar_t *rebuf = realloc(*pLine, *bufsize*sizeof(wchar_t));
      
      		if(rebuf == NULL)
      		{
        		free(*pLine);
        		*pLine = NULL;
        		*bufsize = 0;
      		}
      		else
        	*pLine = rebuf;
    	}
    	
    	if((*pLine)[i-1] == L'C' && c == L'h') // osetrenie pre Ch
			(*pLine)[i-1] = L'©';
		else if((*pLine)[i-1] == L'c' && c == L'h') // osetrenie pre ch
			(*pLine)[i-1] = L'@';
		else {
    		(*pLine)[i] = c;
    		i++;
    	}	
  	}
  	assert(*bufsize >= i);
  	(*pLine)[i] = L'\0';
 	
 	return i;
}

/* Insert sort */
titem *insert(wchar_t *data, titem *zoznam)
{
 	titem *p;
 	titem *q;

 	p = (titem *)malloc(sizeof(titem));
 	p->riadok = data;

 	if(zoznam == NULL || cmpstr(data, zoznam->riadok) > 0)
 	{
  		p->next = zoznam;
  		return p;
 	} 
 	else 
 	{  
  		q = zoznam;
  
  		while(q->next != NULL && cmpstr(data, q->next->riadok) < 0)
   			q = q->next;
   			
  		p->next = q->next;
  		q->next = p;
  
  	return zoznam;
 	}
}

/* Nacitanie suboru */
void readFile(TParams p, titem **z)					
{
	FILE* in = fopen(p.in,"r");

	if(!in)
		fprintf(stderr,"Chyba pri otvarani suboru!\n");
  
  	wint_t c;

  	while((c = fgetwc(in)) != WEOF)
  	{
  		wchar_t *line = NULL;
  		unsigned int buflen = 0;
		
		readLine(&line, &buflen, in, c);
		*z = insert(line, *z);
  	}
	if(errno == EILSEQ && c != WEOF)
		fprintf(stderr,"Neplatny znak!");

 	fclose(in);
}

/* Vypis chybovych stavov */
void printerror(int ecode)
{
    if (ecode < EOK || ecode > EUNKNOWN)
  		ecode = EUNKNOWN;
    fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/* Vypis zoznamu */
void vypis(titem *p, TParams param)
{
	FILE* out = fopen(param.out,"w");

	if(!out)
		fprintf(stderr,"Chyba pri otvarani suboru!\n");

	while(p != NULL)
	{
		int i = 0;
		while(p->riadok[i] != L'\0')
		{
		    if(p->riadok[i] == L'©')	// osetrenie pre ch
			{
				fputwc(L'C', out);
				fputwc(L'h', out);
			}
			else if(p->riadok[i] == L'@')	// osetrenie pre ch
			{
				fputwc(L'c', out);
				fputwc(L'h', out);
			}
			else fputwc(p->riadok[i], out);
			i++;
		}
		fputwc(L'\n', out);
	    p = p->next;
	}
	
	fclose(out);
}

/* Dealokacia zoznamu */
void destroy_list(titem *list)
{
 	titem *p;

	while(list != NULL)
	{
  		p = list->next;
  		free(list);
  		list = p;
 	}
}

/* Hlavny program */
int main(int argc, char *argv[])
{
    TParams params = getParams(argc, argv);
	
	titem *z; 
	z = NULL;
	
    if(params.ecode == 0)
    {
		switch(params.state)
		{
	    	case HELP:
				fprintf(stdout,"%s",HELPMSG);
				break;
	    	case DEFAULT:
	    		readFile(params, &z);
	    		vypis(z,params);
	    		destroy_list(z);
				break;
		}
    }
    else
    { 
    	printerror(params.ecode);
		fprintf(stdout,"%s",HELPMSG);
	}
	
    return EXIT_SUCCESS;
}
