/* Subor:   proj3.c
 * Datum:   2011/12/04
 * Autor:   Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
 * Projekt: Maticove operacie, projekt c. 3 pre predmet IZP
 * Popis:   Program pracuje s maticami a vykonava s nimi zakladne operacie:
						sucet, sucin, transpozicia a ich pripadnu kombinaciu
						Taktiez pocita dlzku vodneho toku a drahu gulecnikovej gule ...*/

#include <math.h> 		 // matematicke funkcie
#include <stdio.h>     // praca so vstupom/vystupom
#include <stdlib.h>    // vseobecne funkcie jazyka C
#include <string.h>    // kvoli funkcii strcmp
#include <ctype.h>     // testovanie znakov - isalpha, isdigit, atd.
#include <limits.h>    // limity ciselnych typov
#include <errno.h>

// Kody chyb programu
enum tecodes{
  EOK,									// Vsetko OK
  ENOTDIGIT,						// Zadana hodnota nie je cislica
  ECLWRONG,							// Chybny parameter
  EPCWRONG,							// Chybny pocet parametrov
	EFORMAT,							// Neplatny format matice
	ENOTALL,							// Neuplna matica, chybajuce hodnoty
	EFFAILED,							// Chyba pri otvarani suboru
	EFARWRONG,						// Chybny parameter s nazvom suboru
	EFOPEN,								// Chyba pri otvarani suboru
	EMSIZEVALUE,					// Chyba pri nacitani rozmerov matice
	EMSIZEFAIL,						// Nespravne rozmery matice
	EOUTOFMEM,						// Nedostatok pamati
	EITEMREAD,						// Chyba pri nacitani prvku matice
	EMATRIXOFLOW,					// Subor obsahuje viac prvkov ako su rozmery matice
	EMALLOC,							// Nedostatok pamati
	EFALSE,								// Chybny format maticep pre danu operaciu
	EBADPOS,							// Zadany bod ako parameter lezi mimo matice
	EUNKNOWN,							// Neznama chyba
};

// Stavove kody programu
enum tstates{
  HELP = 0,        			// Napoveda
  SUM,     							// Sucet matic
  MULT,         				// Sucin matic
  TRANSP,       				// Transpozicia
  TEST,									// Test formatu matice
  EXPR, 								// Vypocet zadaneho vyrazu
  WATER, 								// Simulacia vodneho toku
  POOL,     						// Simulacia pohybu gulecnikovej gule
};

/** Chybove hlasenia odpovedajuce chybovym kodom */
const char *ECODEMSG[] =
{
  [EOK] = "Vsetko v poriadku.\n",
  [ENOTDIGIT] = "Zadany parameter obsahuje neplatny znak vstupu.\n",
  [ECLWRONG] = "Chybne parametre prikazoveho riadku!\n",
  [EPCWRONG] = "Chybny pocet parametrov prikazoveho riadku!\n",
  [EFORMAT] = "Chybny format matice!\n",
  [ENOTALL] = "Nekompletna matica!\n",	
	[EFFAILED] = "Nastala chyba pri otvarani suboru!\n",
	[EFARWRONG] = "Chybny parameter s nazvom suboru!\n",
	[EFOPEN] = "Chyba pri otvarani suboru!\n",
	[EMSIZEVALUE] = "Chybna pri nacitani rozmerov matice!\n",
	[EMSIZEFAIL] = "Nespravne rozmery matice!\n",
	[EOUTOFMEM]	= "Nedostatok pamati!\n",
	[EITEMREAD] = "Chyba pri nacitani prvkov matice!\n",
	[EMATRIXOFLOW] = "Zdrojovy subor obsahuje neplatny pocet prvkov matice!\n",
	[EMALLOC] = "Chyba pri alokacii pamati!\n",
	[EFALSE]	=	"false\n",
	[EBADPOS]	=	"Zvoleny bod lezi mimo matice!\n",
  [EUNKNOWN] = "Nastala nepredvidatelna chyba!\n",
};  

const char *HELPMSG =
"Program Iteracne vypocty.\n"
"Autor: Tomas Kubovcik (c) 2011\n"
"Program pracuje s maticami a prevadza s nimi zakladne operacie sucet,\n"
"sucin, transpozicia, vyraz A*BT+A. Program taktiez simuluje vodny tok \n"
"a pohyb gulecnikovej gule... \n"
"Pouzitie:\n"
"proj3 -h                            - zobrazi napovedu\n"
"proj3 --test data.txt               - otestuje format vstupnych udajov\n"
"proj3 --add A.txt B.txt             - scita dve matic\n"
"proj3 --mult A.txt B.txt            - vynasobi dvoch matic\n"
"proj3 --trans A.txt                 - vykona transpoziciu matice\n"
"proj3 --expr A.txt B.txt            - vypocita hodnotu vyrazu\n"						// doplnit tvar vyrazu
"proj3 --water r s M.txt             - simulacia vodneho toku\n"
"proj3 --carom r s dir power M.txt   - simulacia pohybu gulecnikovej gule\n"
;

typedef struct {
  int ecode;        	// Chybovy kod programu, odpovedajuci tecodes
  int state;        	// Stavovy kod programu, odpovedajuci tstates
	int r;
	int s;
	char* dir;
	int power;
} TParams;

typedef struct {
  int r;							// pocet riadkov (rows)
  int c;							// pocet stlpcov (collumns)
	int **matica;				// data ulozene v matici
} matrix;

typedef struct {
  int r;							// pocet riadkov (rows)
  int s;							// pocet stlpcov (collumns)
	int val;						// hodnota
} neighbour;

/*
	Vytvorenie 2-rozmerneho pola + alokacia
*********************************************/
int **make(int row, int col){
	int **ppmem,i;

	ppmem = (int **) malloc(row*sizeof(int*));
	for(i = 0;i<row;i++)
		ppmem[i] = (int *) malloc(col*sizeof(int));
return (ppmem);
}

/*
	Dealokacia
*********************************************/
void deMalloc(matrix* m){
	if(m->matica != NULL)
		free(m->matica);
}

/*
	Nacitanie parametrov
*********************************************/
TParams getParams(int argc, char *argv[]){
  TParams result =
  {
   .ecode = EOK,
  };

	char *endptr;

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
    if(strcmp("--test", argv[1]) == 0)
			result.state = TEST;
		else if(strcmp("--trans", argv[1]) == 0)
			result.state = TRANSP;
		else result.ecode = ECLWRONG;
	}		
	else if(argc == 4)
	{	
		if(strcmp("--add", argv[1]) == 0)
			result.state = SUM;
		else if(strcmp("--mult", argv[1]) == 0)
			result.state = MULT;
		else if(strcmp("--expr", argv[1]) == 0)
			result.state = EXPR;
		else result.ecode = ECLWRONG;
	}
	else if(argc == 5)
	{
		if(strcmp("--water", argv[1]) == 0)
		{
			result.state = WATER;

			result.r = strtoul(argv[2], &endptr, 10);
			result.s = strtoul(argv[3], &endptr, 10);
		}
		else result.ecode = ECLWRONG;
	}
	else if(argc == 7)
	{
		if(strcmp("--carom", argv[1]) == 0)
		{
			result.state = POOL;

			result.r = strtoul(argv[2], &endptr, 10);
			result.s = strtoul(argv[3], &endptr, 10);
			result.dir = argv[4];
			result.power = strtoul(argv[5], &endptr, 10);
		}
		else result.ecode = ECLWRONG;
	}
	else result.ecode = EPCWRONG;

return result;      
}

/*
	VYPIS MATICE 
*********************************************/
int writeMat(matrix* out){																																		
	if(out->matica == NULL)
		return EXIT_FAILURE;			// nealokovana matica
	else				
	{
		fprintf(stdout,"%d %d\n",out->r,out->c);

		for(int r = 0; r < out->r; r++)
		for(int s = 0; s < out->c; s++)
			if(s == (out->c)-1)
				fprintf(stdout, "%d\n", out->matica[r][s]);
			else fprintf(stdout, "%d ", out->matica[r][s]);
	}
return EXIT_SUCCESS;
}

/*
	NACITANIE A TESTOVANIE SUBORU 
*********************************************/
int readMat(char* argv[], matrix* test, unsigned int n){
	int x = 0;
  
	FILE *file;
  file = fopen(argv[n], "r");
  
	if (file == NULL)														
		return EFOPEN;																															// subor neexistuje
	else if(fscanf(file,"%d%d",&test->r,&test->c) != 2)															// nacitanie rozmerov matice
		return EMSIZEVALUE;																												// chyba pri nacitani										
	else if(test->c <= 0 || test->r <= 0)
		return EMSIZEFAIL;																												// neplatne rozmery matice
	else
	{
		test->matica = make(test->r,test->c);	
		if(test->matica == NULL)																									/** alokovanie pamati pre maticu test */	
			return EOUTOFMEM;
		else 
		{
			for(int r = 0; r < test->r; r++)
			for(int s = 0; s < test->c; s++)
				if((fscanf(file, "%d", &test->matica[r][s]) != 1))
					return EITEMREAD;																						
				
			if(fscanf(file,"%d",&x) == 1)
				return EMATRIXOFLOW;
		}
  	fclose(file);
	}
	return EXIT_SUCCESS;
}

/*
	SUCET MATIC A a B
*********************************************/
int add(matrix* output, matrix* a, matrix* b){
	if(a->c != b->c || a->r != b->r)		/* testuje, ci maju matice vhodny */
		return EFALSE;											/* format pre danu operaciu */
	else
	{
		output->r = a->r;
		output->c = a->c;
		output->matica = make(output->r, output->c);		// alokacia pamati
		
		if (!output->matica)
			return EMALLOC;
		else 
			for(int r = 0; r < output->r; r++)
			for(int s = 0; s < output->c; s++)
				output->matica[r][s] = a->matica[r][s] + b->matica[r][s];
	}
	return EXIT_SUCCESS;
}

/*
	SUCIN MATIC A a B
*********************************************/
int multiply(matrix* output, matrix* a, matrix* b){
	if(a->c != b->r)															/* test formatu matic pre */
		return EFALSE;															/* nasobenie [m][n]*[n][p], */
	else																					/* vysl. matica = [m][p] */
	{
		output->r = a->r;
		output->c = b->c;
		output->matica = make(output->r, output->c); //alokacia pamati 
		
		if(!output->matica)
			return EMALLOC;
		else 
		{
			for (int i = 0; i < a->r; i++)
				for (int j = 0; j < b->c; j++)
				{
					output->matica[i][j] = 0;
					for (int k = 0; k < a->c; k++)
						output->matica[i][j] += a->matica[i][k]*b->matica[k][j];
				}
		}
	}
	return EXIT_SUCCESS; 
}

/*
	TRANSPOZICIA MATICE A
*********************************************/
int trans(matrix* output, matrix* a){
	output->r = a->c;
	output->c = a->r;
	output->matica = make(output->r, output->c);
		
	if (!output->matica)
		return EMALLOC;
	else
	{
		for(int r = 0; r < output->r; r++)
		for(int s = 0; s < output->c; s++)
			output->matica[r][s] = a->matica[s][r];
	}
	return EXIT_SUCCESS;
}

/*
 VYPOCET VYRAZU A*BT+A
*********************************************/
int expr(matrix* output, matrix* output1, matrix* a, matrix* b){
	trans(output1, b);
	multiply(b, a, output1);
	add(output, b, a);

	return EXIT_SUCCESS;
}

/*
 FUNKCIA PRE VYPIS CHYBOVYCH STAVOV
*********************************************/
void printerror(int ecode){
	if (ecode < EOK || ecode > EUNKNOWN)
  	ecode = EUNKNOWN;
	fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/*
	1	2
0	 x	3
	5	4

 OSETRENIE SUSEDNYCH BODOV PRAMENA MIMO MATICE
*********************************************/
int waterNeigh(matrix* input , int r, int s, neighbour surr[]){
	int k = 0;

	for(int i = -1; i < 2; i++)
	for(int j = -1; j < 2; j++)
		if((r%2 == 0 && ((i == 1 && j == 1) || (i == -1 && j == 1))) || (r%2 != 0 && ((i == -1 && j == -1) || (i == 1 && j == -1))) || (i==0 && j==0))
		{} 
		else if((r+i >= 0 && r+i < input->r) && (s+j >= 0 && s+j < input->c))
		{
			surr[k].val = input->matica[r+i][s+j];
			surr[k].r = r+i;
			surr[k].s = s+j;
			k++;
		}
	return k;
}

/*
	VODNY TOK
*********************************************/
int waterFlow(matrix* output, matrix* input, int sR, int sC){
	output->r = input->r;
	output->c = input->c;

	if(sR < 0 || sC < 0 || sR > input->r-1 || sC > input->c-1)
		return EBADPOS;

	int n = 0;
	neighbour surr[6];			// pole struktur pre uchovanie hod. a adresy susedov
		
	if(output->matica == NULL)
	{
		output->matica = make(output->r, output->c);	// alokacia pamati
		
		if(!output->matica)
			return EMALLOC;		
													
		for(int r = 0; r < output->r; r++)            /* naplnenie vystupnej */
		for(int s = 0; s < output->c; s++)						/* matice jednotkami */
			output->matica[r][s] = 1;
	}

		output->matica[sR][sC] = 0;								// na sucasnu poziciu ulozi 0
		n = waterNeigh(input , sR, sC, surr);			// pocet najdenych susedov
		int min = input->matica[sR][sC];

		for(int j = 0; j<n; j++)									// hlada hodnotu minima
			if(surr[j].val <= min)
				min = surr[j].val;
	
		for(int j = 0; j<n; j++)
			if(surr[j].val == min && (output->matica[surr[j].r][surr[j].s] != 0))
				waterFlow(output, input, surr[j].r, surr[j].s);
	return EXIT_SUCCESS;
}

/*
	POHYB GULECNIKOVEJ GULE
*********************************************/
void carom(matrix* input, int r, int s, char* dir, int power){
	printf("%d ",input->matica[r][s]);
	
	for(int i = 1; i < power; i++){
		if(strcmp(dir,"V") == 0)
			if(s < input->c-1)
				s++;
			else {
				dir = "Z";
				s--;
			}
		
		else if(strcmp(dir,"JV") == 0)
			if(r%2 == 0){
				if(r < input->r-1)
					r++;
				else if(r == input->r-1 && s == input->c-1){
					dir = "SZ";
					r--;
					s--;
				}
			}
			else if(r < input->r-1 && s < input->c-1){
					s++;
					r++;
				}
				else {
					dir = "JZ";
					r++;
				}

		else if(strcmp(dir,"JZ") == 0){
			if(r%2 == 0)
				if(r < input->r-1 && s > 0){
					r++;
					s--;
				}
				else if(r == input->r-1 && s == 0){
					dir = "SV";
					r--;
				}
				else { 
					r++;
					dir = "JV";
				}
			else if(r == input->r-1){
				r--;
				s--;
				dir = "SZ";
			}
			else r++;
			}

			else if(strcmp(dir,"Z") == 0){
				if(s>0)
					s--;
				else if(s == 0){
					dir = "V";
					s++;
				}
			}

			else if(strcmp(dir,"SV") == 0){
				if(r%2 == 0)
					if(r > 0)
						r--;
					else if(r != 0){
						dir = "JZ";
						r++;
						s--;
					}
					else {
						r++;
						dir = "JV";
					}
				else if(s == input->c-1){
					dir = "SZ";
					r--;
				}
				else {
					s++;
					r--;
				}
			}

			else if(strcmp(dir,"SZ") == 0){
				if(r%2 == 0)
					if(r>0){
						r--;
						s--;
					}
					else if(r == 0 && s != input->c-1){
						dir = "JZ";
						r++;
						s--;
					}
					else {
						dir = "Z";
						s--;
					}
				else r--;
			}	
		printf("%d ",input->matica[r][s]);
		}
}

/*
	FUNKCIA POCITANIE A PRIRADENIE SPRAVNYCH HODNOT
*********************************************/
void action(char* argv[], int argc){
	matrix A,B,res,res1;			// vysledna matica pri operaciach, docasna
	int err = 0;
	res.matica = NULL;
	res1.matica = NULL;
	A.matica = NULL;
	B.matica = NULL;

	TParams params = getParams(argc, argv);
	
	if(params.ecode == 0)
	{
		switch(params.state)
		{
			case HELP:
				fprintf(stdout,"%s",HELPMSG);
				break;
			case TEST:
				if((err = readMat(argv, &A, 2)) == 0)
					writeMat(&A);
				break;
			case TRANSP:
				if((err = readMat(argv, &A, 2)) == 0)
					err = trans(&res, &A);
				break;
			case SUM:
				if((err = readMat(argv,&A,2)) == 0 && (err = readMat(argv,&B,3)) == 0)
					err = add(&res, &A, &B);
				break;
			case MULT:
				if((err = readMat(argv,&A,2)) == 0 && (err = readMat(argv,&B,3)) == 0)
					err = multiply(&res, &A, &B);
				break;
			case EXPR:															
				if((err = readMat(argv,&A,2)) == 0 && (err = readMat(argv,&B,3)) == 0)
 					err = expr(&res, &res1, &A, &B);
				break;
			case WATER:
				if((err = readMat(argv,&A,4)) == 0)
					err = waterFlow(&res, &A, params.r, params.s);
				break;
			case POOL:
				if((err = readMat(argv,&A,6)) == 0)
				 carom(&A,params.r,params.s, params.dir, params.power);
				break;
		}
	if(res.matica != NULL)
		writeMat(&res);
	}
	else printerror(params.ecode);
	
	if(err != 0) 
		printerror(err);

	deMalloc(&A);
	deMalloc(&B);
	deMalloc(&res1);
	deMalloc(&res);
}

/** Hlavny program */
int main(int argc, char *argv[]){
	action(argv, argc);
return EXIT_SUCCESS;
}
