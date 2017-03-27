#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

void ResetCanonicalMode(int fd, struct termios *savedattributes){
    tcsetattr(fd, TCSANOW, savedattributes);
}

void SetNonCanonicalMode(int fd, struct termios *savedattributes){
    struct termios TermAttributes;

    // Make sure stdin is a terminal. 
    if(!isatty(fd)){
        fprintf (stderr, "Not a terminal.\n");
        exit(0);
    }
    
    // Save the terminal attributes so we can restore them later. 
    tcgetattr(fd, savedattributes);
    
    // Set the funny terminal modes. 
    tcgetattr (fd, &TermAttributes);
    TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO. 
    TermAttributes.c_cc[VMIN] = 1;
    TermAttributes.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}

int get_size(char * str){
	int size = 0;
	int i = 0;
	while(str[i] != '\0'){
		size++;
		i++;
	}
	return size;
}

void history()
{
	
}

void cd()
{
	
}

void pwd(){
	char * curr_directory = get_current_dir_name();
	write(STDOUT_FILENO, curr_directory, get_size(curr_directory));
	write(STDOUT_FILENO, "\n", 1);
}

void ls(char *path)
{
	DIR *dir = opendir(path);
	struct stat perms;
	if(dir) {
		struct dirent *dirInfo;
		while((dirInfo = readdir(dir)) != NULL)
		{
			stat(dirInfo->d_name, &perms);
			//check each permission individually
			if(S_ISDIR(perms.st_mode)) 
				write(STDOUT_FILENO, "d", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			//user permissions
			if(S_IRUSR & perms.st_mode)
				write(STDOUT_FILENO, "r", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IWUSR & perms.st_mode)
				write(STDOUT_FILENO, "w", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IXUSR & perms.st_mode)
				write(STDOUT_FILENO, "x", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			//group permissions
			if(S_IRGRP & perms.st_mode)
				write(STDOUT_FILENO, "r", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IWGRP & perms.st_mode)
				write(STDOUT_FILENO, "w", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IXGRP & perms.st_mode)
				write(STDOUT_FILENO, "x", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			//other permissions
			if(S_IROTH & perms.st_mode)
				write(STDOUT_FILENO, "r", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IWOTH & perms.st_mode)
				write(STDOUT_FILENO, "w", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			if(S_IXOTH & perms.st_mode)
				write(STDOUT_FILENO, "x", 1);
			else
				write(STDOUT_FILENO, "-", 1);
			
			write(STDOUT_FILENO, " ", 1);
			write(STDOUT_FILENO, dirInfo->d_name, get_size(dirInfo->d_name));
			write(STDOUT_FILENO, "\n", 1);
		}
	}
	return;
}

void clearString(char string[], int len)
{
	for(int i = 0; i < len; i++)
		string[i] = 0;
}

class histQ {
	public:
		char hist[10][256];
		int numHist;
		
		histQ() {
			numHist = 0;
			for(int i = 0; i < 10; i++)
				clearString(hist[i], 256);
		}
		
		void enqueue(char *cmd) {
			if(numHist == 10) {
				for(int i = 0; i < 9; i++) {
					strcpy(hist[i], hist[i+1]);
				}
				strcpy(hist[9], cmd);
			}
			else {
				strcpy(hist[numHist], cmd);
				numHist++;
			}		
		}
		
		void showAll() {
			char num[1];
			for(int i = 0; i < numHist; i++) {
				num[0] = '0' + i;
				write(STDOUT_FILENO, num, 1);
				write(STDOUT_FILENO, " ", 1);
				write(STDOUT_FILENO, hist[i], get_size(hist[i]));
				write(STDOUT_FILENO, "\n", 1);
			}
		}
		
		char* getHist(int index) {
			return hist[index];
		}
};

void parse(char *data, histQ history)
{
	int argCount = 0;
	char args[16][256];
	char currArg[256];
	int currArgC = 0;
	for(int i = 0; i < 16; i++)
		clearString(args[i], 256);
	for(int i = 0; i < get_size(data)+1; i++) {
		if(data[i] == ' ') {	//space
			strcpy(args[argCount], currArg);
			argCount++;
			clearString(currArg, 256);
			currArgC = 0;
		}
		else if(data[i] == '\000') { //end of data
			strcpy(args[argCount], currArg);
			argCount++;
			clearString(currArg, 256);
			currArgC = 0;
		}
		else if(data[i] == '>') { //input redirect
			strcpy(args[argCount], currArg);
			argCount++;
			strcpy(args[argCount], ">");
			argCount++;
			clearString(currArg, 256);
			currArgC = 0;
			//redirect stuff
		}
		else if(data[i] == '<') { //output redirect
			strcpy(args[argCount], currArg);
			argCount++;
			strcpy(args[argCount], "<");
			argCount++;
			clearString(currArg, 256);
			currArgC = 0;
			//redirect stuff
		}
		else if(data[i] == '|') { //pipe
			strcpy(args[argCount], currArg);
			argCount++;
			strcpy(args[argCount], "|");
			argCount++;
			clearString(currArg, 256);
			currArgC = 0;
			//redirect stuff
		}
		else { //build currArg
			currArg[currArgC] = data[i];
			currArgC++;
		}
	}
	if(strcmp(args[0], "ls") == 0) {
		if(args[1][0] != '\0')
			ls(args[1]);
		else 
			ls(".");
	}
	else if(strcmp(args[0], "pwd") == 0) {
		pwd();
	}
	else if(strcmp(args[0], "exit") == 0) {
		exit(1);
	}
	else if(strcmp(args[0], "history") == 0) {
		history.showAll();
	}
}



int main(int argc, char *argv[])
{
	struct termios SavedTermAttributes;
	SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
	char data[256];
	clearString(data, 256);
	int dataCount = 0;
	char RXchar;
	char cmdChar[5];
	histQ history;
	int histNum = 0;
	int histIndex = 0;
	
	while(true)
	{
		char * curr_directory = get_current_dir_name();
		write(STDOUT_FILENO, curr_directory, get_size(curr_directory));
		write(STDOUT_FILENO, ">", 1);
		histIndex = histNum;
		while(1)
		{
			clearString(cmdChar, 5);
			read(STDIN_FILENO, &RXchar, 1);
			if(RXchar == 0x1B) //up, down, delete
			{
				read(STDIN_FILENO, &cmdChar, 5);
				if(cmdChar[1] == 0x41)	//up
				{
					if(histIndex == 0) {
						write(STDOUT_FILENO, "\a", 1);
					}
					else {
						while(dataCount != 0) {
							write(STDOUT_FILENO, "\b \b", 3);
							dataCount--;
						}
						clearString(data, 256);
						histIndex--;
						strcpy(data, history.getHist(histIndex));
						write(STDOUT_FILENO, data, get_size(data));
						dataCount = get_size(data);
						continue;
					}
				}
				else if(cmdChar[1] == 0x42) //down
				{
					if(histIndex >= 9) {
						write(STDOUT_FILENO, "\a", 1);
					}
					else {
						while(dataCount != 0) {
							write(STDOUT_FILENO, "\b \b", 3);
							dataCount--;
						}
						clearString(data, 256);
						histIndex++;
						strcpy(data, history.getHist(histIndex));
						write(STDOUT_FILENO, data, get_size(data));
						dataCount = get_size(data);
						continue;
					}
				}
				else if(cmdChar[1] == 0x33) //delete
				{
					if(dataCount > 0)
					{
						write(STDOUT_FILENO, "\b \b", 3);
						dataCount--;
					}	
					else
						continue;
				}
				else
					continue;
			}
			else if(RXchar == 0x7F) //backspace
			{
				if(dataCount > 0)
				{
					write(STDOUT_FILENO, "\b \b", 3);
					dataCount--;
				}	
				else
					continue;
			}
			else if(RXchar == 0x0A) //enter 
			{
				write(STDOUT_FILENO, "\n", 1);
				data[dataCount] = '\0';
				history.enqueue(data);
				if(histNum < 10)
					histNum++;
				parse(data, history);
				clearString(data, dataCount+1);
				dataCount = 0;
				break;
			}
			else //alphanumeric
			{
				data[dataCount] = RXchar;
				dataCount++;
				write(STDOUT_FILENO, &RXchar, 1);
			}
		}
		//write(STDOUT_FILENO, "\n", 1);
	}
	ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes);
}