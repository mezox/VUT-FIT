/* 
 * Subor: 	readerWriter.c
 * Autor: 	Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
 * Datum: 	02/05/2012
 * Predmet: IOS, projekt 2
 * Popis: 	praca s procesmi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

typedef struct shared {
	int value;				// hodnota ktoru zapisuju writeri
	unsigned int index;		// citac akcii
	unsigned int w_count;	// poradie writera
	unsigned int r_count;	// poradie readera
	const char *shm_name;	// premenna pre pomenovanie zdielanej pamati
} t_shared;

typedef struct sem {
	sem_t *sem_r;			// chrani operaciu citania
	sem_t *sem_w;			// chrani operaciu pisania
	sem_t *mutex1;			// chrani premennu w_count
	sem_t *mutex2;			// chrani premennu r_count
	sem_t *mutex3;			// reader vs reader
	sem_t *inc;				// chrani operaciu inkrementacie scitaca
} t_semf;

typedef struct params {
	int error;
	unsigned int writers;	// pocet writerov
	unsigned int readers;	// pocet readerov
	unsigned int cycles;	// pocet cyklov
	unsigned int sw;		// rozsah pre simulaciu spracovania writerom [ms]
	unsigned int sr;		// rozsah pre simulaciu spracovanía readerom [ms]
	char * out;				// vystupny subor
} t_params;

//--Vypis-chyby--------------
void die(const char *msg)	{

    perror(msg);
    exit(1);
}

//--Vypise-napovedu-ak-bol-zadany-parameter -h alebo --help ----------------
void writeHelp(void)	{
	const char * HELPMSG = 
	" Program: Readers-writers problem.\n"
	" Pouzitie: readerWriter W R C SW SR OUT\n"
	" W   : pocet pisarov\n"
	" R   : pocet citatelov\n"
	" C   : počet cyklov\n"
	" SW  : rozsah pre simulaciu spracovania pisarom [ms]\n"
	" SR  : rozsah pre simulaciu spracovanía citatelom [ms]\n"
	" OUT : nazov vystupneho suboru, do ktoreho sa budu ukladat\n" 
	" 	    generovane informacie. Pokial sa miesto nazvu uvedie znak -, \n"
	"	    budu sa informacie vypisovat na standardny vystup.\n";

	printf("%s", HELPMSG);
}

//--Nacitanie-parametrov-------------------------
t_params getparams(int argc, char * argv[])	{
	
	t_params loaded;
	char * endptr;
	loaded.error = 0;

	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 )
		writeHelp();

	if(argc == 7)	{
		loaded.writers = strtoul(argv[1],&endptr,10);
		loaded.readers = strtoul(argv[2],&endptr,10);
		loaded.cycles = strtoul(argv[3],&endptr,10);
		loaded.sw = strtoul(argv[4],&endptr,10);
		loaded.sr = strtoul(argv[5],&endptr,10);
		loaded.out = argv[6];
	}
	else {
		fprintf(stderr,"CHYBA: Nespravny pocet parametrov\n");
		loaded.error = 1;
	}

	return loaded;	
}

// Funkcia pre readera
void reader(t_shared * memo, t_semf sem, int i,unsigned int time,FILE *out)	{
  int val;

  do	{
        sem_wait(sem.mutex3);
        	sem_wait(sem.sem_r);
        		sem_wait(sem.mutex1);
           			(memo->r_count)++;
           			if(memo->r_count == 1) sem_wait(sem.sem_w);
        		sem_post(sem.mutex1);
        	sem_post(sem.sem_r);
        sem_post(sem.mutex3);
	
		sem_wait(sem.inc);
			fprintf(out,"%d: reader: %d: ready\n",(memo->index)++,i);
		sem_post(sem.inc);

		sem_wait(sem.inc);
			fprintf(out,"%d: reader: %d: reads a value\n",(memo->index)++,i);
			fprintf(out,"%d: reader: %d: read: %d\n",(memo->index)++,i,memo->value);
			val = memo->value;
		sem_post(sem.inc);

		// reader simuluje cakanie
		usleep((random() % (time + 1))*1000);

  		sem_wait(sem.mutex1);
     		(memo->r_count)--;
     		if(memo->r_count == 0) sem_post(sem.sem_w);
  		sem_post(sem.mutex1);

  } while(val);
  exit(0);
}

// Funkcia pre writera
void writer(unsigned int c,t_shared * memo, t_semf sem,unsigned int i,unsigned int time,FILE *out)	{

	for(unsigned int k = 0; k < c; k++)	{

		// writer pise
        sem_wait(sem.mutex2);
           (memo->w_count)++;
           if(memo->w_count == 1) sem_wait(sem.sem_r);
        sem_post(sem.mutex2);


			sem_wait(sem.inc);
				fprintf(out,"%d: writer: %d: new value\n",(memo->index)++,i);
			sem_post(sem.inc);

			// writer simuluje uspanie pre cas [0-time]
		if(time != 0) {
			sem_post(sem.sem_r);
   			usleep((random() % (time + 1))*1000);
			sem_wait(sem.sem_r);
			sem_post(sem.sem_w);
		}

			sem_wait(sem.inc);
				fprintf(out,"%d: writer: %d: ready\n",(memo->index)++,i);
			sem_post(sem.inc);

		sem_wait(sem.sem_w);
			sem_wait(sem.inc);
				fprintf(out,"%d: writer: %d: writes a value\n",(memo->index)++,i);

				// writer zapise svoj index		
				memo->value = i;

				fprintf(out,"%d: writer: %d: written\n",(memo->index)++,i);
			sem_post(sem.inc);
		sem_post(sem.sem_w);

  		sem_wait(sem.mutex2);
     		(memo->w_count)--;
     		if(memo->w_count == 0) sem_post(sem.sem_r);
  		sem_post(sem.mutex2);
	}
    exit(0);
}

/* Funkcia pre uvolnenie zdrojov */
void free_sources(t_shared * memo, t_semf sem,int fd, FILE *out)	{

	if(	sem_destroy(sem.mutex1) == -1	||
    	sem_destroy(sem.mutex2) == -1	||
    	sem_destroy(sem.mutex3) == -1	||
    	sem_destroy(sem.sem_r) == -1	||
    	sem_destroy(sem.sem_w) == -1)
	fprintf(out,"ERROR: sem_destroy: Semaphore destroy failed.\n");

    close(fd);
    if(shm_unlink(memo->shm_name) == -1)
		fprintf(out,"ERROR: shm_unlink: Unlinking memory failed.\n");	

	fclose(out);
}

/***************** Main ******************/
int main(int argc, char *argv[])	{

	t_params myparams = getparams(argc,argv);
	FILE * output;

	if(myparams.error != 0)	exit(2);

	// otvorenie suboru
	if(strcmp(myparams.out,"-") != 0)	{
		output = fopen(myparams.out,"w");

		if(output == NULL)	{
			fprintf(stderr,"ERROR: Failed to open output file!\n");
			exit(1);
		}
	}
	else output = stdout;

	setbuf(output, NULL);

	t_shared * memo;
	t_semf sem;
	
	//---Alokacia pamati--------------------------------------
    int fd = shm_open("/xkubov02-mem", O_RDWR|O_CREAT, 0600);
    if (fd < 0)
        die("ERROR: shm_open: failed to create shared memory obect!\n");

    if (ftruncate(fd, sizeof(t_shared)))
        die("ERROR: ftruncate: failed to truncate a file!\n");

    memo = mmap(NULL, sizeof(t_shared), PROT_READ|PROT_WRITE,
            MAP_SHARED, fd, 0);
    if (memo == MAP_FAILED)
        die("ERROR: mmap: failed to map a memory!\n");
	//-----------------------------------------------------------

	// Namapovanie semaforov do zdielanej pamati
	if(((((((	(sem.inc = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)	||
		(sem.sem_w = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)	||
		(sem.sem_r = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)	||
		(sem.mutex1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)	||
		(sem.mutex2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)	||
		(sem.mutex3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED))	{
	fprintf(output,"ERROR: mmap: Semaphore mapping failed.\n");
	free_sources(memo,sem,fd,output);
	return EXIT_FAILURE;
	}

	// Inicializacia semaforov
	if(	sem_init(sem.mutex1,1,1) == -1	||
		sem_init(sem.mutex2,1,1) == -1	||
		sem_init(sem.mutex3,1,1) == -1	||
		sem_init(sem.sem_w,1,1) == -1	||
		sem_init(sem.sem_r,1,1) == -1	||
		sem_init(sem.inc,1,1) == -1)	{		
	fprintf(output,"ERROR: sem_init: Semaphore inicialization failed.\n");
	free_sources(memo,sem,fd,output);
	return EXIT_FAILURE;
	}

	// Inicializacia citacov a ostatnych zdielanych premennych
	memo->value = -1;
	memo->index = 1;
	memo->w_count = 0;
	memo->r_count = 0;
	memo->shm_name = "xkubov02-mem";

	// zabezpeci nahodne generovanie cisel
	srand(time(0));

	// polia pre ulozenie pid vytvaranych procesov
	pid_t pid_writer[myparams.writers];
	pid_t pid_reader[myparams.readers];

	// Vytvori procesy writerov
	for(unsigned i = 0; i < myparams.writers; i++)	{
	
		pid_writer[i] = fork();
		
		if(pid_writer[i] == 0)
			writer(myparams.cycles,memo,sem,i+1,myparams.sw,output);
		else if(pid_writer[i] < 0)
			fprintf(output,"ERROR: fork failed.\n");
	}

	// Vytvori procesy readerov
	for(unsigned int i = 0; i < myparams.readers; i++)	{
        
		pid_reader[i] = fork();

		if(pid_reader[i]==0)
			reader(memo,sem,i+1,myparams.sr,output);
		else if(pid_reader[i] < 0)
			fprintf(output,"ERROR: fork failed.\n");
	}

    // Caka na ukoncenie writerov
    for(unsigned int i = 0; i  < myparams.writers; i++)
        waitpid(pid_writer[i], NULL, 0);

	// Procesy writerov su ukoncene, hlavny proces vklada do pamati hodnotu 0
    
	sem_wait(sem.inc);
        memo->value = 0;
    sem_post(sem.inc);

	// Caka na ukoncenie readerov
    for(unsigned int i = 0; i < myparams.readers; i++)
        waitpid(pid_reader[i], NULL, 0);

	free_sources(memo,sem,fd,output);

    return 0;
}
