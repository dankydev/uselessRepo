
/* inclusione delle librerie */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

/* zprintf : funzione che simula il comportamento della fprintf attraverso chiamate di sistema. E' sa */
void zprintf(int fd, const char *fmt, ...) 
{
   static char msg[1024];
   int n;
   va_list ap;
   
   va_start(ap, fmt);
   n = vsnprintf(msg, 1024, fmt, ap);
   n = write(fd, msg, n);
   va_end(ap);
}

/* struttura di dati richiesta */
typedef struct mstruct{
   char result[4096];
}mstruct;

/* tipo di dato apposito per semplificare l'utilizzo delle pipe multiple */
typedef int pipe_t[2];

/* main : funzione contenente il codice del processo padre */
int main(int argc, char **argv) 
{
   /* variabili di iterazione dei cicli */
   int i, j;

   int status; /* gestione del valore di uscita dei processi figli*/
   pid_t pid; /* [GENERIC] variabile/array per gestire il/i pid creato/i tramite fork() */
   int child_n; /* variabili atte a contenere il numero di processi figli e nipoti da generare */
   pipe_t *p; /* variabile puntatore alla zona di memoria contenente tutte le pipe in utilizzo */
   pipe_t p2; /* pipe di comunicazione tra nipote e figlio */
   char bufferchild;
   int bufferfather;

   int min_argc = 3; /* valore minimo di argc */
   
   /* controllo che il numero dei parametri passati allo script sia appropriato */
   if (argc < min_argc) 
   {
      /* stampo tutti gli errori sul file avente file descriptor 2, ovvero su stderror */
      zprintf(2, "\t[%d, padre] Utilizzo : %s file_1 file_2 [file_3] ... [file_n]\n\n", getpid(), argv[0]);
      exit(1);
   }
   
   /* stabilisco il numero di processi da creare (tanti quanti i file passati) */
   child_n = argc - 1;

   /* [GENERIC] controllo i parametri passati a linea di comando */
   if (child_n <= 0)
   {
      zprintf(2, "\t[%d, padre] Errore : il numero dei processi figli deve essere positivo\n\n", getpid());
      exit(2);
   }

   /* alloco la zona di memoria dedicata alle pipe tra processo padre e processi figli, con controllo dell'errore */
   p = (pipe_t *) malloc(sizeof(pipe_t) * child_n);
   if (p == NULL) 
   {
      zprintf(2, "\t[%d, padre] Errore : malloc() fallita \n\n", getpid());
      exit(3);
   }
   
   /* inizializzo le pipeline allocate attraverso la primitiva pipe() */
   for (i = 0; i < child_n; i++) 
   { 
      if (pipe(p[i]) < 0) 
      { 
         zprintf(2, "\t[%d, padre] Errore : pipe() fallita \n\n");
         exit(4);
      }
   }

   /* creo i processi figli */
   for (i = 0; i < child_n; i++)
   {
      /* controllo l'esito della fork() */
      switch (fork())
      {
         case 0:
	    /* codice del figlio */
	    
            /* chiudo i lati della pipe non utilizzati dal figlio */
	    for (j = 0; j < child_n; j++)
	    {
	       close(p[j][0]);
	       if (j != i) close(p[j][1]);
	    }

	    /* genero la pipe di comunicazione tra figlio e nipote */
	    if (pipe(p2) < 0)
	    {
	       zprintf(2, "\t[%d, figlio] Errore : pipe() fallita\n\n", getpid());
	       exit(5);
	    }

	    /* creo il nipote, con controllo dell'errore */
	    switch(fork())
	    {
	       case 0:
		  /* chiudo lo standard output del nipote così da ridirigerlo sulla pipe di comunicazione con il figlio che l'ha generato */
		  close(1);
		  dup(p2[1]);
                  close(0);
		  if (open(argv[i + 1], O_RDONLY) < 0)
		  {
		     zprintf(2, "\t[%d, nipote] Errore : open() fallita\n\n", getpid());
		     exit(6);
		  }

                  /* eseguo il comando richiesto */
		  execlp("tail", "tail", "-1", (char *)0);

		  /* la primitiva exec() ritorna solo in caso di errore, quindi esco dal processo figlio di conseguenza */
		  exit(EXIT_FAILURE);
	          break;

	       case -1:
	          zprintf(2, "\t[%d, figlio] Errore : fork() fallita\n\n", getpid());
		  exit(7);
		  break;
	    }
            
            /* azioni del figlio */
	    /* chiudo i lati della pipe inutilizzati */
            close(p2[1]);

	    /* lettura, carattere per carattere, dell'output del comando */
	    j = 0;
            while(read(p2[0], &bufferchild, sizeof(char)) == sizeof(char)) j++;

	    /* escludo il terminatore di riga dal conteggio */
	    j--;

	    if (write(p[i][1], &j, sizeof(int)) != sizeof(int))
	    {
	       zprintf(2, "\t[%d, figlio] Errore : write() fallita\n\n", getpid());
	       exit(9);
	    }

	    /* attendo la terminazione del nipote */
            if (wait(&status) < 0)
	    {
	       zprintf(2, "\t[%d, figlio] Errore : wait() fallita\n\n", getpid());
	       exit(10);
	    }

	    /* controllo che la exec del nipote sia andata a buon fine */
            if (WEXITSTATUS(status) == EXIT_FAILURE)
	    {
	       zprintf(2, "\t[%d, figlio] Errore : exec() fallita\n\n", getpid());
	       exit(11);
	    }

            /* termino */
	    exit(WEXITSTATUS(status));
	    break;

	 /* caso di fork() fallita */
	 case -1:
	    zprintf(2, "\t[%d, padre] Errore : fork() fallita\n\n", getpid());
	    exit(12);
	    break;
      }
   }

   /* codice del padre */
   /* chiudo i lati della pipe inutili per il padre */
   for (i = 0; i < child_n; i++)
   {
      close(p[i][1]);
   }

   /* azioni del padre */
   /* leggo le pipe in ordine inverso, così da leggere dall'ultimo al primo risultato */
   zprintf(1, "\n");
   for (i = child_n - 1; i >= 0; i--)
   {
      if (read(p[i][0], &bufferfather, sizeof(int)) != sizeof(int))
      {
         zprintf(2, "\t[%d, padre] Errore : read() fallita\n\n", getpid());
	 exit(13);
      }
      zprintf(1, "[%d, padre] Il processo di indice %d, inerente il file %s, ha calcolato una lunghezza di linea pari a %d\n", getpid(), i, argv[i + 1], bufferfather);
   }

   zprintf(1, "\n");
   /* attendo la terminazione dei processi figli */
   for (i = 0; i < child_n; i++)
   {
      if ((pid = wait(&status)) < 0)
      {
         zprintf(2, "\t[%d, padre] Errore : wait() fallita\n\n");
	 exit(14);
      }

      zprintf(1, "[%d, padre] Il processo figlio con pid %d è uscito co valore di ritorno %d\n", getpid(), pid, WEXITSTATUS(status));
   }
   exit(EXIT_SUCCESS);
}