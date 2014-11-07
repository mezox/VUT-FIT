/* Subor:   proj2.c
 * Datum:   2011/11/15
 * Autor:   Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
 * Projekt: Iteracne vypocty, projekt c. 2 pro predmet IZP
 * Popis:   Program aproximuje funkcie logaritmus a arcus sinus pomocou
 *					iteracnych vypoctov, taktiez pocita dlzku lomenej ciary
 *          a dlzku lomenej ciary s chybou ...*/

#define MYABS(x) (x<0 ? -x : x)

#include <math.h> 		 // matematicke funkcie
#include <stdio.h>     // praca so vstupom/vystupom
#include <stdlib.h>    // vseobecne funkcie jazyka C
#include <string.h>    // kvoli funkcii strcmp
#include <ctype.h>     // testovanie znakov - isalpha, isdigit, atd.
#include <limits.h>    // limity ciselnych typov
#include <errno.h>

const double IZP_E = 2.7182818284590452354;        // e
const double IZP_PI = 3.14159265358979323846;      // pi
const double IZP_2PI = 6.28318530717958647692;     // 2*pi
const double IZP_PI_2 = 1.57079632679489661923;    // pi/2
const double IZP_PI_4 = 0.78539816339744830962;    // pi/4

// Kody chyb programu
enum tecodes{
  EOK,
  ENOTDIGIT,
  ECLWRONG,
  EPCWRONG,
  EOFLOW,
	EUNKNOWN,
};

// Stavove kody programu
enum tstates{
  ARCSIN,      // -t
  LBL,         // -d
  LOGAX,       // -h
  LBLE,        // -m
  HELP,        // Napoveda
};

/** Chybove hlasenia odpovedajuce chybovym kodom */
const char *ECODEMSG[] =
{
  [EOK] = "Vsetko v poriadku.\n",
  [ENOTDIGIT] = "Zadany parameter obsahuje neplatny znak vstupu.\n",
  [ECLWRONG] = "Chybne parametre prikazoveho riadku!\n",
  [EPCWRONG] = "Chybny pocet parametrov prikazoveho riadku!\n",
  [EOFLOW] = "Zadane cislo je mimo rozsah premennej!\n",
  [EUNKNOWN] = "Nastala nepredvidatelna chyba!\n",
};  

const char *HELPMSG =
"Program Iteracne vypocty.\n"
"Autor: Tomas Kubovcik (c) 2011\n"
"Program nacita cislo zo standardneho vstupu (v sekundach)\n"
"a prevadza ho na zadanu jednotku... \n"
"Pouzitie:\n"
"proj2 -h               - zobrazi napovedu\n"
"proj2 --arcsin sigdig  - vypocita hodnotu arcsin s presnostou sigdig\n"
"proj2 --logax sigdig a - pocita logaritmus pri zaklade a s presnostou sigdig\n"
"proj2 --lbl            - vypocita priebeznu dlzku lomenej ciary\n"
"proj2 --lble ERR       - vypocita priebeznu dlzku lomenej ciary kde ERR je\n"
"	 	                      absolutní chyba měření souřadnic.\n"
;

typedef struct params{
  int ecode;        	// Chybovy kod programu, odpovedajuci tecodes
  long int presnost;  // Parameter sigdig z prikazoveho riadku
  double zaklad;    	// Zaklad logaritmu
  double chyba;  			// Chyba pre lomenu ciaru
  int state;        	// Stavovy kod programu, odpovedajuci tstates
} TParams;

/** Nacitanie parametrov */
TParams getParams(int argc, char *argv[]){
  TParams result =
  {
   .ecode = EOK,
   .presnost = 0,
   .zaklad = 0,
	 .chyba = 0,
  };

  char *ctrl, *ctrl1;
	errno = 0;	

/** Priradenie prislusnych stavov v zavislosti od zadanych parametrov, */
/** osetrenie neplatnych vstupov pre parametre prikazoveho riadku, */
/** konverzia parametrov na vhodne udajove typy */

	if(argc == 2){
  	if (strcmp("-h", argv[1]) == 0)
			result.state = HELP;
		else if (strcmp("--lbl", argv[1]) == 0)
			result.state = LBL;
		else result.ecode = ECLWRONG;								
	}
	else if(argc == 3){
    if (strcmp("--arcsin", argv[1]) == 0){
			result.state = ARCSIN;
			result.presnost = strtol(argv[2],&ctrl,10);

      if(ispunct(*ctrl) || isalpha(*ctrl))			
      	result.ecode = ENOTDIGIT;

			if(result.presnost < 0)	
				result.ecode = ECLWRONG;
		}
		else if (strcmp("--lble", argv[1]) == 0){
			result.state = LBLE;
			result.chyba = strtod(argv[2],&ctrl);

			if(result.chyba < 0)	
				result.ecode = ECLWRONG;

      if(isalpha(*ctrl) || ispunct(*ctrl))  
      	result.ecode = ENOTDIGIT;
		}
		else result.ecode = ECLWRONG;									 
	}
	else if(argc == 4 && strcmp("--logax", argv[1]) == 0){		
		result.state = LOGAX;
    result.presnost = strtol(argv[2],&ctrl,10);
			
		if(result.presnost < 0)	
			result.ecode = ECLWRONG;
        
    if(isalpha(*ctrl) || ispunct(*ctrl)) 
      result.ecode = ENOTDIGIT;
		
		result.zaklad = strtod(argv[3],&ctrl1);

    if(isalpha(*ctrl1))
    	result.ecode = ENOTDIGIT;
    if (errno == ERANGE)
    	result.ecode = EUNKNOWN;
	}
	else result.ecode = EPCWRONG;
return result;      
}

/** Vypocet prirodzeneho logaritmu */
double ln(double x, long int sdig){
  double y, krok, n = 1, stary_vysl, medzivyp, sucet = 0, eps;
  int pom = 0, i = 0;
	
	eps = pow(10,(-1)*(sdig));
 
  if(x<1 && x > 0){
    while((x/(IZP_E)) < 1){					 				// uprava x pre zrychlenie vypoctu
			pom = 1;
    	x = x*IZP_E;
      i++;
    }
	}
	else while((x/IZP_E) >= 1){
		pom = 2;		
		x = x/IZP_E;
    i++;
  }

  y = (x - 1)/(x + 1);
  krok = y;
  n += 2;
  sucet = krok;
	medzivyp = pow(y,3.0);

  do   {																				// iteracny vypocet
		stary_vysl = krok;
  	krok = medzivyp/n;
  	n += 2;
  	sucet += krok;
		medzivyp *= pow(y,2.0);
  }
  while (fabs(stary_vysl - krok) >= eps);
    
	sucet = 2 * sucet;

	if(pom == 0)
	  return sucet+i;
  else return sucet-i;
}

/** Logaritmus pri zaklade a z cisla x */
/** vypocet + osetrenie vstupov */ 
double logax(double x, long int sdig, double zaklad){
	if((zaklad == 0 && x == 0) || (x < 0) || (x == -INFINITY) || (zaklad < 0))
    return NAN;
  else if(zaklad == 0)
    return 0;
  else if((x == 0) && (zaklad > 1))
    return -INFINITY;
  else if(((x == 0) && (zaklad < 1)) || x == INFINITY)
    return INFINITY;
  else if((zaklad < 1) && x == INFINITY)
    return -INFINITY;
  else return ln(x, sdig)/ln(zaklad, sdig);
}

/** Arcus Tangens */
double arctan(double x, long int sdig){
	int zmensi = 0,
			pom = 0;

	if(x >= 1){																																		// heuristika
		x = 1/x;
		pom = 1;
	}

	if(x > (2 - sqrt(3))){	
		x = (sqrt(3)*x-1)/(sqrt(3)+x);
		zmensi = 1;
	}

  double sucet = x, 
				 krok = sucet, 
				 p = 1, q = 3, 
				 eps = pow(10,(-1)*(sdig));

    x = -x*x;
																																								// eps*MYABS(sucet)) == relativna presnost
    while (MYABS(krok)>(eps*MYABS(sucet))){
        krok = krok * x * p/q;
        p = p + 2.0;
				q = q + 2.0;
        sucet = sucet + krok;
    }
	
	if(zmensi == 1 && pom == 1) return IZP_PI_2 - (sucet + (IZP_PI/6));
		else if(zmensi == 0 && pom == 1) return IZP_PI_2 - sucet;
		else if(zmensi == 1 && pom == 0) return sucet + (IZP_PI/6);
	return sucet;
}

/** Arcus sinus */
double arcsin(double x, long int sdig){	
	
	if(x == 1.0) return IZP_PI/2;																									// hranicne hodnoty funkcie arcsin
	else if(x == -1.0) return -IZP_PI/2;
		
 	if(x<0){																																			// vyuzijeme arctan(-x) = -arctan(x)
		x = -x;
		return -arctan(x/(sqrt(1-pow(x, 2.0))),sdig);																											
	}
return arctan(x/(sqrt(1-pow(x, 2.0))),sdig);
}

/** Vypocet dlzky lomenej ciary */
double lineLength(double x1, double y1, double x2, double y2){ 
	return sqrt((pow(x2-x1,2.0))+(pow(y2-y1,2.0)));
}

/** Vypocet max. dlzky lomenej ciary s chybou*/
double LineError(double x1, double y1, double x2, double y2, double e, char type){
	double ymax = fabs(y1-y2)+2*e;																								/* deklaracia premennych pre maximalne */
	double xmax = fabs(x1-x2)+2*e;																								/* a minimalne vzdialenosti suradnic x,y, */
																																								/* pouzitych na vypocet max/ min vzdialenosti */
	double ymin = fabs(y1-y2)-2*e;																								/* pomocou pytagorovej vety */
	double xmin = fabs(x1-x2)-2*e;

	if(type == '+')
		return sqrt(pow(xmax,2.0)+pow(ymax,2.0));
	
	if(type == '-')
		if(ymin < 0 && xmin < 0)																										// ak dojde k prieniku "stvorcov"
			return 0;
		if(ymin < 0) 																																// pripad kedy sa x-suradnice prekryvaju
			return xmin;
		if(xmin < 0) 																																// pripad kedy sa y-suradnice prekryvaju
			return ymin;
		else return sqrt(pow(xmin,2.0)+pow(ymin,2.0));
	
return 0;
}

/** Vstup pre lomenu ciaru + lomenu ciaru s chybou*/
void inputLBL(double chyba, unsigned int error){																// parameter error nastavuje vstup pre LBL alebo LBLE
	double x,c,dlzkaMax = 0,dlzkaMin = 0,coor[4] ={0,0,0,0};											// premenne na nacitanie
	short i = 0, j = 0;																																	

	while ((c = scanf("%lf",&x)) != EOF){																					// nacitanie suradnic
		if(c == 0){ 
			scanf("%*s");
			x = NAN;
			printf("%lf\n",x); 
		}
		else {
		coor[i] = x;
		j++;	
		if (i == 1 && error == 1){
			printf("%.10e\n", dlzkaMin);																							// vypis pre prvu dvojicu suradnic
			printf("%.10e\n", dlzkaMax);							 																						
		}
		else if (i == 1 && error == 2)
			printf("%.10e\n", dlzkaMax);			

		if(i>2){
			if(error == 1){																														
				dlzkaMin += LineError(coor[0],coor[1],coor[2],coor[3],chyba,'-');				/* volanie funkcie na vypocet */
				printf("%.10e\n", dlzkaMin);																						/* maximalnej */
																																								/* a */
				dlzkaMax += LineError(coor[0],coor[1],coor[2],coor[3],chyba,'+');				/* minimalnej dlzky lomenej ciary*/
				printf("%.10e\n", dlzkaMax);
			}
			else {
				dlzkaMax += lineLength(coor[0],coor[1],coor[2],coor[3]);
				printf("%.10e\n", dlzkaMax);
			}
		}
		if (i == 3){																																/* zameni druhy par suradnic */
			i = 1; 																																		/* za prvy a pokracuje v nacitavani */
			coor[0] = coor[2];																												
			coor[1] = coor[3];
		}
		i++;
		}   
	}
	if(j%2 != 0) printf("%lf\n",NAN);
}

/** Vstup pre arcsin a logax */
double inputASINLN(long int sdig, double zaklad, int type){
	double x,c;

	while ((c = scanf("%lf",&x)) != EOF){
		if(c == 0){																																	// ak nebolo nacitane cislo
			c = scanf("%*s");
			x = NAN;
			printf("%.10e \n", x);
		}
		else if(type == 2) 																													// type 1 == arcsin
			printf("%.10e \n", arcsin(x,sdig));
		else if(type == 1) printf("%.10e \n", logax(x,sdig,zaklad));								// type 2 == logax
		}
	
return 0;		
}

/** vypis error kodov*/
void printerror(int ecode){
	if (ecode < EOK || ecode > EUNKNOWN)
  	ecode = EUNKNOWN;
	fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/** Hlavny program */
int main(int argc, char *argv[]){
	TParams params = getParams(argc, argv);

 if(params.ecode == EOK){
	switch(params.state){
		case HELP: printf("%s", HELPMSG);
							break;
		case LBL: inputLBL(params.chyba,2);
							break;
		case LBLE: inputLBL(params.chyba,1);
							break;
		case LOGAX: inputASINLN(params.presnost, params.zaklad,1);
							break;
		case ARCSIN: inputASINLN(params.presnost, params.zaklad,2);
							break;
	}
 }
 else printerror(params.ecode);

return EXIT_SUCCESS;
}
