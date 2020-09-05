
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 * NOTICE: This lab is property of Purdue University. You should not for any reason make this code public.
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
		char        *string_val;
		// Example of using a c++ type in yacc
		std::string *cpp_string;
}

%token <cpp_string> WORD
%token GREATGREATAMPERSAND SEMICOLON NOTOKEN NEWLINE GREAT LESS LESSLESS GREATGREAT PIPE AMPERSAND AMPERSANDAMPERSAND HASH AMPERSANDGREAT TWOGREAT GREATAMPERSAND

%{

#define MAXFILENAME 1024
#include <cstdio>
#include <assert.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <dirent.h>
#include "shell.hh"

		void yyerror(const char * s);
		int yylex();
		int maxEntries = 20;
		int nEntries = 0;
		char ** argList;
		bool wildCardPresent = false;
		void expandWC (char *prefix, char *suffix, bool r);
		bool r = false;

		int compare (const void * a, const void * b) {
				return strcmp( *(const char**)a, *(const char**)b);
		}

		void mySort() {
				qsort(argList, nEntries, sizeof(const char*), compare);
		}

		void ArgAdder (char *str) {
				// If root, set r true		
				if (str[0] == '/')
						r = true;
				expandWC(NULL, str, r);
                free(str);
				return;
		}

		void expandWC (char *prefix, char *suffix, bool r) {
				// Initialize the list				
				if (!argList) 
						argList = (char **) malloc (maxEntries * sizeof (char*));
				// If suffix length 0, add the path to list
				if (suffix[0] == 0) {
						if (nEntries == maxEntries) {
								maxEntries *= 2;
								argList = (char**)realloc(argList, maxEntries * sizeof(char*));
								assert(argList);
						}
						if (prefix && strlen(prefix) > 0) {
								argList[nEntries++] = strdup(prefix);
//                                free(prefix);
						}

						return;
				}
				// Check if suffix has path to expand to
				char * s = strchr(suffix, '/');
				char curr[MAXFILENAME];
				// If path
				if (s != NULL) {
						strncpy(curr,suffix, s-suffix);
						curr[strlen(suffix) - strlen(s)] = '\0';
						suffix = s + 1;
				}
				else {
						strcpy(curr, suffix);
						suffix += strlen(suffix);
				}
				// Declare a concatenating string
				char newPF[MAXFILENAME];
				// Checking for Wildcards
				if (strchr(curr, '*') == NULL && strchr(curr, '?') == NULL) {
						if (prefix != NULL) 
								sprintf(newPF,"%s/%s", prefix, curr);
						else 
								sprintf(newPF,"%s", curr);
						expandWC(newPF, suffix, false);
						return;
				}
				wildCardPresent = true;
				char * reg = (char*) malloc (2 * strlen(curr) + 10);
				char * arg_ptr = curr;
				char * reg_ptr = reg;
				*reg_ptr = '^';
				reg_ptr++;
				while (*arg_ptr) {
						if (*arg_ptr == '*') { 
								*reg_ptr = '.'; 
								reg_ptr++; 
								*reg_ptr = '*'; 
								reg_ptr++; 
						}
						else if (*arg_ptr == '?') { 
								*reg_ptr = '.'; 
								reg_ptr++;
						}
						else if (*arg_ptr == '.') { 
								*reg_ptr = '\\'; 
								reg_ptr++;
								*reg_ptr = '.'; 
								reg_ptr++;
						}
						else { 
								*reg_ptr = *arg_ptr; 
								reg_ptr++;
						}
						arg_ptr++;
				}
				*reg_ptr = '$';
				reg_ptr++; 
				*reg_ptr = 0;
				regex_t re;	
				// Compiling Regex
				int result = regcomp( &re, reg,  REG_EXTENDED|REG_NOSUB);
				if ( result != 0 ) {
						fprintf( stderr, "Bad regular expresion \"%s\"\n", reg );
						exit( -1 );
				}
                //int status = regexec(&re, reg, (size_t)0, NULL, 0);
                //regfree(&re);
				DIR * dir;
				if (!prefix) 
						dir = opendir(".");
				else if (strcmp(prefix, "") == 0) {
						// If root
						dir = opendir("/");
						sprintf(newPF, "%s", "/");
				}
				else
						dir = opendir(prefix);
				// If root, add /
				if (r) {
						char str[strlen(prefix) + 1];
						str[0] = '/';
						strcpy(str+1,prefix);
						dir = opendir(str);
				}
				if (dir == NULL) {
						return;
				}
				free(reg);
				struct dirent * ent;
				// Recursively check the different paths
				while (ent = readdir(dir)) {
						if (! regexec(&re, ent->d_name, 0, NULL, 0 )) {
								if (prefix == NULL || prefix[0] == 0) {
										if (newPF[0] == '/')
												sprintf(newPF, "/%s", ent->d_name);
										else
												sprintf(newPF, "%s", ent->d_name);
								}
								else {
										sprintf(newPF, "%s/%s", prefix, ent->d_name);
								}
								if (ent->d_name[0] == '.') {
										if (curr[0] == '.') {
												expandWC(newPF, suffix, false);
										}
								}
								else {
										expandWC(newPF, suffix, false);
								}
						}
				}
				closedir(dir);
                regfree(&re);
				return;
		}


		%}

		%%

		goal:
		commands
		;

commands:
command
| commands command
;

command: simple_command
;

simple_command:	
pipe_list iomodifier_list background_optional NEWLINE {
		Shell::_currentCommand.execute();
}
| NEWLINE {
		Shell::prompt();
}
| error NEWLINE { yyerrok; }
;

command_and_args:
command_word argument_list {
		Shell::_currentCommand.
				insertSimpleCommand( Command::_currSimpleCommand );
}
;

argument_list:
argument_list argument
| /* can be empty */
;

argument:
WORD {
        char *cmd = strdup($1->c_str());
		ArgAdder(strdup(cmd));
		if (argList[0]) {
				mySort();
				for (int i = 0; i < nEntries; i++) {
						Command::_currSimpleCommand->insertArgument(new std::string(argList[i]));
						fflush(0);
				}
		}
		else {
				Command::_currSimpleCommand->insertArgument($1);
				fflush(0);

		}
		wildCardPresent = false;
		for (int i = 0; i < nEntries; i++) { 
				free(argList[i]);
		}
		nEntries = 0;
        free(cmd);
		free(argList);
		argList = NULL;
		r = false;
}
;

command_word:
WORD {
		Command::_currSimpleCommand = new SimpleCommand();
		Command::_currSimpleCommand->insertArgument( $1 );
}
;

pipe_list: 
pipe_list PIPE command_and_args 
| command_and_args 
; 

iomodifier_list: 
iomodifier_list iomodifier_opt
| /*empty*/
;

iomodifier_opt:
GREATGREAT WORD {
		Shell::_currentCommand._outFileName = $2;
		Shell::_currentCommand._app = true;
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
}
| GREAT WORD {
		Shell::_currentCommand._outFileName = $2;
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
}
| LESS WORD {
		Shell::_currentCommand._inFileName = $2;
		if ( Shell::_currentCommand._imod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._imod = true;
}
| LESSLESS WORD {
		Shell::_currentCommand._inFileName = $2;
		if ( Shell::_currentCommand._imod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._imod = true;
}
| GREATAMPERSAND WORD {
		Shell::_currentCommand._outFileName = $2;
		std::string *str = new std::string($2->c_str());
		Shell::_currentCommand._errFileName = str;
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
}
| AMPERSANDGREAT WORD {
		Shell::_currentCommand._outFileName = $2;
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
		std::string *str2 = new std::string($2->c_str());
		Shell::_currentCommand._errFileName = str2;
}  
| GREATGREATAMPERSAND WORD {
		Shell::_currentCommand._outFileName = $2;
		std::string *str1 = new std::string($2->c_str());
		Shell::_currentCommand._errFileName = str1;
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
		Shell::_currentCommand._app = true;
}
| TWOGREAT WORD {
		if ( Shell::_currentCommand._omod ) {
				Shell::_currentCommand._ambiguous = true;
		}
		Shell::_currentCommand._omod = true;
		Shell::_currentCommand._errFileName = $2;
}
;

background_optional:
AMPERSAND {
		Shell::_currentCommand._backgnd = true;
}
| 
;



%%


		void
yyerror(const char * s)
{
		fprintf(stderr,"%s", s);
}

#if 0
main()
{
		yyparse();
}
#endif
