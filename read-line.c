/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <assert.h>
#include <regex.h>
#include <dirent.h>
#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
//extern void expandWC (char *prefix, char *suffix, bool r);
// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];


// Simple history array
// This history does not change. 
// Yours have to be updated.
int pos = 0;
int latest = 0;
int history_index = 0;
char history[30][MAX_BUFFER_LINE];
int history_length = 0;
void read_line_print_usage()
{
		char * usage = "\n"
				" ctrl-?       Print usage\n"
				" Backspace    Deletes last character\n"
				" up arrow     See last command in the history\n";

		write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

		// Set terminal in raw mode
		tty_raw_mode();
		line_length = 0;
		// Read one line until enter is typed
		while (1) {

				// Read one character in raw mode.
				char ch;
				read(0, &ch, 1);

				if (ch>=32 && ch != 127) {
						// Do echo
						write(1,&ch,1);
						// If max number of character reached return.
						if (line_length==MAX_BUFFER_LINE-2) break; 

						for (int i = line_length; i >= pos; i--) 
								line_buffer[i + 1] = line_buffer[i];
						// add char to buffer.
						line_buffer[pos]=ch;
						// It is a printable character.
						//						printf("\nbuff = %s  pos = %d len = %d\n", line_buffer, pos, line_length);
						pos++;
						line_length++;
						int i = 0;
						if (pos != line_length) {
								for (i =0; i < pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								// Print spaces on top
								for (i =0; i < pos; i++) {
										ch = ' ';
										write(1,&ch,1);
								}
								// Print backspaces
								for (i =0; i < pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								write(1, line_buffer, line_length);
								// Move Cursor								
								for (i = 0; i < line_length - pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
						}
				}
				else if (ch == 4) {
						// Delete
						if ( pos != line_length ) {
								// Update Buffer
								for (int i = pos; i < line_length; i++) {
										line_buffer[i] = line_buffer[i+1];
								}
								line_length--;
								int i;
								// Move Cursor to Beg
								for (i = 0; i < pos ; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								// Print spaces on top
								for (i = 0; i <= line_length; i++) {
										ch = ' ';
										write(1,&ch,1);
								}
								// Print backspaces
								for (i = 0; i <= line_length; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								write(1, line_buffer, line_length);
								for (i = 0; i < line_length - pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
						}
				}
				else if (ch==10) {
						// <Enter> was typed. Return line
						if (strlen(line_buffer) > 0) {
								if (history_index >= 29) {
										history_index = 0;
										history_length--;
								}
								latest = history_index;
								line_buffer[strlen(line_buffer)] = '\0';
								strncpy(history[history_index], line_buffer, line_length);
								history_length++;
								history_index++;
						}
						// Print newline
						write(1,&ch,1);
						pos = 0;
						break;
				}
				else if (ch == 31) {
						// ctrl-?
						read_line_print_usage();
						line_buffer[0]=0;
						break;
				}
				else if (ch==27) {
						// Escape sequence. Read two chars more
						//
						// HINT: Use the program "keyboard-example" to
						// see the ascii code for the different chars typed.
						//
						char ch1; 
						char ch2;
						read(0, &ch1, 1);
						read(0, &ch2, 1);
						if (ch1==91 && ch2==65) {

								// Up arrow. Print next line in history.

								// Erase old line
								// Print backspaces
								int i = 0;
								for (i =0; i < pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								// Print spaces on top
								for (i =0; i < line_length; i++) {
										ch = ' ';
										write(1,&ch,1);
								}
								// Print backspaces
								for (i =0; i < line_length; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								if (history_index - 1 >= 0) 
										history_index = history_index - 1;
								// Copy line from history
								strcpy(line_buffer, history[history_index]);
								line_length = strlen(history[history_index]);
								// echo line
								write(1, line_buffer, line_length);
								pos = line_length;
						}
						else if (ch1==91 && ch2==66) {
								// Down arrow. Print next line in history.
								// Erase old line
								// Print backspaces
								int i = 0;
								for (i =0; i < pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								// Print spaces on top
								for (i =0; i < line_length; i++) {
										ch = ' ';
										write(1,&ch,1);
								}
								// Print backspaces
								for (i =0; i < line_length; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								if (history_index < history_length) {
										history_index ++;
										strcpy(line_buffer, history[history_index]);
										line_length = strlen(line_buffer);
								}
								else {
										strcpy(line_buffer, "");
										line_length = strlen(line_buffer);
								}
								write(1, line_buffer, line_length);
								pos = line_length;
						}

						else if (ch1 == 91 && ch2 == 68) {
								// Left Arrow
								if (pos > 0) {
										pos--;
										char c = 8;
										write(1,&c,1);
								}
						}
						else if (ch1 == 91 && ch2 == 67) {
								// Right Arrow
								if (pos < line_length) {
										char c = line_buffer[pos];
										write(1,&c,1);
										pos++;
								}         
						}
						else if ( ch1 == 79 && ch2 == 72) {
								if (line_length > 0 && pos != 0) {
										for (int i = 0; i < pos; i++) {
												char c = 8;
												write(1, &c, 1);
										}
										pos = 0;
								}
						}
				} 
				else if (ch == 127 || ch == 8) {
						// <backspace> was typed. Remove previous character read.
						if (line_length > 0 && pos > 0) {
								for (int i = pos - 1; i < line_length; i++) {
										line_buffer[i] = line_buffer[i + 1];
								}
								line_length--;

								int i;
								for (i = 0; i < pos ; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								pos--;		
								// Print spaces on top
								for (i = 0; i <= line_length; i++) {
										ch = ' ';
										write(1,&ch,1);
								}
								// Print backspaces
								for (i = 0; i <= line_length; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								write(1, line_buffer, line_length);
								// Move Cursor
								for (i = 0; i < line_length - pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
						}
				}
				else if (ch == 1) {
						// Home
						if (pos != 0) {
								// Move Cursor
								for (int i = 0; i <  pos; i++) {
										ch = 8;
										write(1,&ch,1);
								}
								pos = 0;
						}
				}
				else if (ch == 5) {
						// End
						if (pos != line_length) {
								int i;
								// Move Cursor
								for (i = 0; i < pos ; i++) {
										ch = 8;
										write(1,&ch,1);
								}		
								write(1, line_buffer, line_length);
								pos = line_length;
						}

				}
		}

		// Add eol and null char at the end of string
		line_buffer[line_length]=10;
		line_length++;
		line_buffer[line_length]=0;

		return line_buffer;
}

