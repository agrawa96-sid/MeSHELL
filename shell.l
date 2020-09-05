/*
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 * NOTICE: This lab is property of Purdue University. You should not for any reason make this code public.
 */

%{

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string.h>
#include "y.tab.hh"
#include <sys/wait.h>
#include <limits.h>
#include "tty-raw-mode.c"
#include "shell.hh"
#undef getc
#define getc(f) mygetc(f)


extern "C" char * read_line();


int mygetc(FILE * f) {
 static char *p;
 char ch;

 if (!isatty(0)) {
 // stdin is not a tty. Call real getc
   return getc(f);
 }

 // stdin is a tty. Call our read_line.
 if (p==NULL || *p == 0) {
   char * s = read_line();
   p = s;
 }

 ch = *p;
 p++;

 return ch;
}

#undef getc
#define getc(f) mygetc(f)

using namespace std;
static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" { return GREATGREAT; }

"<" { return LESS; }

"<<" { return LESSLESS; }

"#" {return HASH; }

"|" { return PIPE;}

"&>" { return AMPERSANDGREAT;}

">&" { return GREATAMPERSAND;}

"2>" { return TWOGREAT;}

">>&" { return GREATGREATAMPERSAND;}

";" { return SEMICOLON;}

"&" { return AMPERSAND;}

"&&" { return AMPERSANDAMPERSAND;}

\$\{\$\} {
  // Expand ${$}
  int pid = getpid();
  yylval.cpp_string = new std::string(std::to_string(pid));
  return WORD;
}

\$\{\?\} {
  // Expand ${?}
  yylval.cpp_string = new std::string(getenv("?"));
  return WORD;
}

\$\{\_\} {
  // Expand ${_}
  yylval.cpp_string = new std::string(getenv("_"));
  return WORD;
}


\$\{\!\} {
  // Expand ${!}
  yylval.cpp_string = new std::string(getenv("!"));
  return WORD;
}

\$\{\S\H\E\L\L\} {
  // Expand ${SHELL}
  char *buf = new char[PATH_MAX];
  char *res = realpath("../shell", buf);
  yylval.cpp_string = new std::string(buf);
  delete(buf);
  return WORD;
}


\$\{[^\$\n\t\{\}]*\}.* {
  // Multiple Env Var Exp 
  char *str = new char[5000];
  char *var;
  char *exp;
  int ctr = 0;
  int pos = 0;
  for (int i = 0; i < strlen(yytext); i++) {
    if (yytext[i] == '$') {
      if (yytext[i + 1] && yytext[i+1] == '{') {
        int len = 0;
        char *ptr = &(yytext[i + 2]);
        char *ptr1 = ptr;
        while (*ptr) {
          if (*ptr == '}') {
            break;
          }
          len++;
          ptr++;  
        }
        var = new char[len + 1];
        strncpy(var, ptr1, len);
        var[len] = '\0';
        int varlen = 0;
        if (getenv(var)) {
          varlen = strlen(getenv(var));
        }
        if (varlen > 0) {
          exp = new char[varlen + 1];
          strcpy(exp,getenv(var));
          exp[varlen] = '\0';
          i += len;
          i += 2;
          for (int l = 0; l < varlen; l++) {
            str[pos++] = exp[l];
          }
          str[pos] = '\0';
        }
      }
    }
    else {
      str[pos++] = yytext[i];
    }
  }
  yylval.cpp_string = new std::string(str);
  delete(str);
  delete(var);
  delete(exp);
  return WORD;
}

\$\{[^\n\t\{\}]*\} {
  // Single Env Var Exp
  int i = 0;
  char *cmd_line = new char[strlen(yytext)];
  strcpy(cmd_line, yytext);
  char *cmd = &(cmd_line[2]);
  char *var = new char[strlen(yytext)];
  char *exp;
  strncpy(var, cmd, strlen(cmd));
  var[strlen(cmd) - 1] = '\0';
  int varlen = 0;
  if(getenv(var)) {
    varlen = strlen(getenv(var));
  }
  if (varlen > 0) {
    exp = new char[varlen];
    strcpy(exp,getenv(var));
    yylval.cpp_string = new std::string(exp);
    delete(cmd_line);
    delete(var);
    delete(exp);
    return WORD;
  }
  else {
    printf("\nVar = %s\n", var);
    yylval.cpp_string = new std::string(yytext);
    delete(cmd_line);
    delete(var);
    return WORD;
  } 
}


\$\([^\n\t\$]*\) {
  // Subshell
  char *action = new char[strlen(yytext) - 2];
  char *ptr = yytext;
  ptr += 2;
  strncpy(action, ptr, strlen(yytext) - 3);
  action[strlen(yytext) - 3] = '\n';
  char *logout_str = new char[7];
  strcpy(logout_str, "logout\n");
  int dfltin = dup(0);
  int dfltout = dup(1);
  int to_cld[2];
  int to_par[2];
  pipe(to_cld);
  pipe(to_par);
  write(to_cld[1], action, strlen(yytext) - 2);
  write(to_cld[1], logout_str, 7);
  close(to_cld[1]);
  dup2(to_cld[0], 0);
  close(to_cld[0]);
  dup2(to_par[1], 1);
  close(to_par[1]);
  char ** args = new char*[2];
  args[0] = strdup("/proc/self/exe");
  args[1] = NULL;
  const char* first_arg =args[0];
  int status = fork();
  if (status == 0) {
    // Child
    execvp(first_arg, args);
    _exit(1);
  }
  else if (status < 0) {
    perror("Error forking");
    exit(1);    
  }
  dup2(dfltin, 0);
  close(dfltin);
  dup2(dfltout, 1);
  close(dfltout);
  waitpid(status, NULL, 0);
  char *op = new char[5000];
  char c;
  int length = 0;
  int check = 0;
  while (read(to_par[0], &c, 1)) {
    if (c == '\n' | c == '\t') 
      op[length] = ' ';
    else 
      op[length] = c;
    length++;
  }
  op[length] = '\0';
  close(to_par[0]);
  for (int i = strlen(op) - 2; i >= 0; i--) {
    if (op[i] == '\\') 
      unput(op[i]);
    unput(op[i]);
  }
  delete(op);
  delete(action);
  delete(logout_str);
  delete(args);
}

"source "[^ \n\t]* {
  char *str = new char(strlen(yytext) - 6);
  strcpy(str, yytext + 7);
  str[strlen(yytext) - 7] = '\0';
  if ( include_stack_ptr >= MAX_INCLUDE_DEPTH ) {
    fprintf( stderr, "Includes nested too deeply" );
    exit( 1 );
  }
  include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
  yyin = fopen(str, "r");
  delete(str);
  if ( ! yyin )
    perror("Error" );
    yy_switch_to_buffer(
    yy_create_buffer( yyin, YY_BUF_SIZE ) );
    BEGIN(INITIAL);
}

<<EOF>> {
  if ( --include_stack_ptr < 0 ) {
    yyterminate();
  }
  else {
    yy_delete_buffer( YY_CURRENT_BUFFER );
    yy_switch_to_buffer(
    include_stack[include_stack_ptr] );
  }        
}


[^ \n\t]*\\[^ \n\t]* {
  int i = 0;
  char *final_str = new char[strlen(yytext)];
  char *ptr = yytext;
  for (i = 0; i < strlen(yytext); i++) {
    if (*ptr == '\\') {
      final_str[i] = *(ptr + 1);
      ptr++;
    }
    else {
      final_str[i] = *ptr;
    }
    ptr++;
  }
  yylval.cpp_string = new std::string(final_str);
  delete(final_str);
  return WORD;
}

\"[^"\n]*\" {
  int len = strlen(yytext) - 1;
  char *str = new char[len];
  char *ptr = yytext;
  ptr++;
  strcpy(str, ptr);
  str[len - 1] = '\0';
  yylval.cpp_string = new std::string(str);
  delete(str);
  return WORD;
}



[^ \t\n\>\<\\\&\|][^ \t\n\>\<\\\&\|]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

%%


