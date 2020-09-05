/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include "command.hh"
#include "shell.hh"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "string.h"

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommand = std::vector<SimpleCommand *>();

    _outFileName = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    append = 0;
    num_redirects = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommand.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
  for (auto simpleCommand : _simpleCommand) {
    delete simpleCommand;
  }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommand.clear();

    if ( _outFileName ) {
        delete _outFileName;
    }
    _outFileName = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;

    append = 0;

    num_redirects = 0;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommand ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFileName?_outFileName->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}


/*int Command::clear_if_necessary(int err, char *message) {
  if (err == 1) {
    perror(message);
  }
  clear();
  prompt();
  return 1;
} */

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommand.size() == 0 ) {
        Shell::prompt();
        return;
    }

    //char *exit = "exit";
    //int exitStatus = strcmp(_simpleCommand[0]->_arguments[0], "exit");
    //printf("Exit Status = %d\n", exitStatus);
    const char *cmnd = _simpleCommand[0]->_arguments[0]->c_str();
    int exitStatus = strcmp(cmnd, "exit");

    if (exitStatus == 0) {
      //printf("Goodbye Message\n");
      exit(1);
    }
  
  
  /* Why isn't this working??
   */ 
  
    if (num_redirects > 1) {
      printf("Ambiguous output redirect.\n");
      clear();
      Shell::prompt();
      return;
    }


    
    // Print contents of Command data structure
    //print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    //save in/out
    int tempin = dup(0);
    int tempout = dup(1);
    int temperror = dup(2);

    //set the initial input
    int fdin;


    if (_errFile) {

      const char *errfile = _errFile->c_str();

      if (append == 1) {

        // append if specified
        fderr = open(errfile, O_RDWR | O_APPEND | O_TRUNC, 777);
      }

      if (append == 0) {

        // overwrite if specified
        fderr = open(errfile, O_RDWR | O_CREAT | O_TRUNC, 777);
      }


    }

    if (_inFile != NULL) {
      const char *infile = _inFile->c_str();
      //char *infile = new char[_inFile.length() + 1];
      //strcpy(infile, _inFile.c_str());
      fdin = open(infile, O_RDWR, 777);
    }
    else {
      //use defualt input
      fdin = dup(tempin);
    }

    int fderr;

    // if there is an error file redirect
    

    // if there is no error file redirect
    else {
      fderr = dup(temperror);
    }

    // do the standard error redirection

    dup2(fderr, 2);
    close(fderr);

    int ret;
    int fdout;
    for (int i = 0; i < _simpleCommand.size(); i++) {
      
      const char *first_cmd = _simpleCommand[i]->_arguments[0]->c_str();

      // setenv

      if (strcmp(first_cmd, "setenv") == 0) {
        int sev = setenv(_simpleCommand[i]->_arguments[1]->c_str(), _simpleCommand[i]->_arguments[2]->c_str(), 1);
        if (sev == 1) {
          perror("Error setting env");

        }
        clear();
        Shell::prompt();
        return;
        
      }

      if (strcmp(first_cmd, "unsetenv") == 0) {
        int unsev = unsetenv(_simpleCommand[i]->_arguments[1]->c_str());
        if (unsev == 1) {
          perror("Error unsetting env");
        }

        clear();
        Shell::prompt();
        return;
      }

      if (strcmp(first_cmd, "cd") == 0) {
        int numargs = _simpleCommand[i]->_arguments.size();
        int changedir;
        if (numargs == 1) {
          changedir = chdir(getenv("HOME"));
        }
        if (numargs > 1) {
          changedir = chdir(_simpleCommand[i]->_arguments[1]->c_str());
        }

        if (changedir < 0) {
          std::string s = "cd: can't cd to: " + *_simpleCommand[i]->_arguments[1];
          //;tring output = "cd: can't cd to: " + _simpleCommand[i]->_arguments[1];
          const char *output = s.c_str();

          perror(output);
          //perror(_simpleCommand[i]->_arguments[1]->c_str());
        }

        clear();
        Shell::prompt();
        return;
      }


      dup2(fdin, 0);
      close(fdin);

      // check if it is the last command

      if (i == _simpleCommand.size()-1) {

        // if there is output redirection
        if (_outFileName) {
           printf("line 269\n");
           const char *outfile = _outFileName->c_str();
           // append to the specified file

           if (append == 1) {
             printf("Line 274\n");
             fdout = open(outfile, O_APPEND | O_CREAT | O_WRONLY, 777);
           }

           // overwrite the specified file

           if (append == 0) {
             fdout = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, 777);
           }
        }

        // write to standard out
        else {

          fdout = dup(tempout);
        }
      }

      // not the last simple command

      else {
        int fdpipe[2];
        pipe(fdpipe);
        fdout = fdpipe[1];
        fdin = fdpipe[0];
      }

      // do the output redirection

      dup2(fdout, 1);
      close(fdout);

      // fork for the next process of the next simple command
      ret = fork();

      // check to see if this is the child process
      if (ret == 0) {
      
        // print env if asked for

        if (strcmp(first_cmd, "printenv") == 0) {
          char **p = environ;
          while (*p != NULL) {
            printf("%s\n", *p);
            p++;
          }
          fflush(stdout);
          _exit(1);
        } 



      // create an argumnets array to fit the execvp arguments

        const char *char_args[_simpleCommand[i]->_arguments.size() + 1];
        for (int j = 0; j <= _simpleCommand[i]->_arguments.size(); j++) {
          if (j == _simpleCommand[i]->_arguments.size()) {

            char_args[j] = 0;

          }

          else {
            const char *temp = _simpleCommand[i]->_arguments[j]->c_str();
            char_args[j] = temp;
          }
        }

        // execute the specified command

        execvp(char_args[0], (char * const *)char_args);

        // if it does not execute, there is an error and it will return here
        perror("execvp");
        exit(1);
    }
  }

  //restore in/out defaults
  dup2(tempin,0);
  dup2(tempout,1);
  dup2(temperror, 2);
  close(tempin);
  close(tempout);
  close(temperror);

  // check to see if background option is on

  if (!_background) {

    // if not on, then wait for next command
    waitpid(ret, NULL, 0);
  }

  // reset and prompt again
  clear();

  if ( isatty(0) ) {
    Shell::prompt();
  }
}

SimpleCommand * Command::_currentSimpleCommand;
