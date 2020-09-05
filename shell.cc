#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include "shell.hh"
#include <string.h>
#include <sys/wait.h>

int yyparse(void);

void Shell::prompt() {
		if (isatty(0)) {
				// Check if Env Var Exsits. If yes, change the promt
				if (getenv("PROMPT") && strlen(getenv("PROMPT")) > 0)
						printf("%s",getenv("PROMPT"));
				else
						printf("MYSHELL> ");
				fflush(stdout);
		}
}


extern "C" void disp ( int sig ) {
		sig = sig;
		fprintf( stderr, "Ctrl-C Hit. Aborting...\n");
}

extern "C" void zomb ( int sig ) {
		while (waitpid(-1, NULL, WNOHANG) > 0) {
				sig = sig;
		}
}

void yyrestart(FILE * file);

void ctrlchandler() {
		// Ctrl C Handler
		struct sigaction ctrlc;
		ctrlc.sa_handler = disp;
		sigemptyset(&ctrlc.sa_mask);
		ctrlc.sa_flags = SA_RESTART;
		if (sigaction(SIGINT, &ctrlc, NULL)){
				perror("sigaction");
				exit(2);
		}
}

void zombhandler() {
		// Handle Zombie Processes
		struct sigaction zombie;
		zombie.sa_handler = zomb;
		sigemptyset(&zombie.sa_mask);
		zombie.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &zombie, NULL)){
				perror("sigaction");
				exit(2);
		}
}



int main() {
		// Check if .shellrc file
		ctrlchandler();
		zombhandler();
		FILE *file = fopen(".shellrc", "r");
		if (file) {
				yyrestart(file);
				yyparse();
                yyrestart(stdin);
				fclose(file);
		}
		else {
				Shell::prompt();
        }
		yyparse();

}

Command Shell::_currentCommand;
