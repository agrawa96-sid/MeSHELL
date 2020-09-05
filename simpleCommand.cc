#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <pwd.h>
#include <iostream>
#include <string.h>
#include <regex.h>
#include "simpleCommand.hh"
#include <dirent.h>

SimpleCommand::SimpleCommand() {
		_argumentsArray = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
		// iterate over all the arguments and delete them
		for (auto & arg : _argumentsArray) {
				if (arg)
						delete arg;
		}
}


void SimpleCommand::insertArgument( std::string * argument ) {
		// simply add the argument to the vector
		int size = strlen(argument->c_str()) + 1;
		char arg1 [size];
		char *arg = arg1;
		strcpy(arg, argument->c_str());
		arg[strlen(argument->c_str())] = '\0';
		// Tilde Expansion
		if (arg[0] == '~') {
				if (strlen(argument->c_str()) == 1) {
						char *str = new char[strlen(getenv("HOME"))];
						strcpy(str, getenv("HOME"));
						argument = new std::string(str);
						delete(str);
				}
				else {
						// Expand to Home
						if (arg[1] == '/') {
								char *str = new char[strlen(getenv("HOME"))];
								strcpy(str, getenv("HOME"));
								str = strcat(str, arg + 1);
								argument = new std::string(str);
								delete(str);
						}
						else {
								char *arg_ptr = arg;
								arg_ptr++;
								char *user_name = new char[40];
								int i = 0;
								int len = strlen(arg);
								for (i = 0; i < len; i++) {
										if (*arg_ptr == '/')
												break;
										user_name[i] = *arg_ptr;
										arg_ptr++;
								}
								user_name[i] = '\0';
								if (*arg_ptr) {
										char *final_str = new char[strlen(getpwnam(user_name)->pw_dir) + strlen(arg_ptr) + 1];
										strcpy(final_str, getpwnam(user_name)->pw_dir);
										char *ptr2 = final_str + strlen(getpwnam(user_name)->pw_dir);
										strcpy(ptr2, arg_ptr);
										final_str[strlen(final_str)] = '\0';
										argument = new std::string(final_str);
										delete(final_str);
								}
								else {
										char *final_str = new char[strlen(getpwnam(user_name)->pw_dir)];
										strcpy(final_str, getpwnam(user_name)->pw_dir);
										argument = new std::string(final_str);
										free(final_str);
								}
								delete(user_name);
						}
				}
		}
		_argumentsArray.push_back(argument);
	//	delete(arg1);
}

// Print out the simple command
void SimpleCommand::print() {
		for (auto & arg : _argumentsArray) {
				std::cout << "\"" << *arg << "\" \t";
		}
		// effectively the same as printf("\n\n");
		std::cout << std::endl;
}
