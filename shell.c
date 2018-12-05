/******************************************************************************
* 
* Name: 	Zaid Albirawi
* Email: 	zalbiraw@uwo.ca
*
* shell.c processes the users input, passes it to a tokenizer, and then passes
* the tokens obtained to the process_commands function which will process and
* execute the commands.
*
******************************************************************************/

#include "shell.h"

void parse( char *line , char **tokens , char* delim);
bool is_exit(char* input);
void remove_space(char *line);
void check_out_file(char *tokens,int *OUTFILE);
void check_in_file(char *tokens, int *INFILE);
bool two_consecutive_operands(char a, char b);
bool is_valid_command(char *line);

/******************************************************************************
* Processes the input and determine whether it is a user interface operation
* or a set of commands that will need to be executed.
******************************************************************************/
void shell(char* filename)
{

	/**************************************************************************
	* short			special_char 	determines whether the character to be
	*								processed is special or not.
	* int 			len 			keeps track of the current line length
	* char 			line 			holds current line
	**************************************************************************/
	short special_char = FALSE;
	int status, len = 0;
	char ch, *line = (char*)malloc(MAX_LEN);
	FILE *fp = NULL;

	if (filename != NULL)
	{
		fp = fopen(filename, READ);

		if (fp == NULL) printf("Unable to open %s\n", filename);

	}

	/**************************************************************************
	* Loops until the user exits the program.
	**************************************************************************/
	print_user();
	while(TRUE)
	{

		ch = getch(fp);

		if (special_char)
		{
			special_char = FALSE;
			ch = switch_keypad(ch);
		}

		/**********************************************************************
		* switchs arround possible cases depending on the read character
		**********************************************************************/
		switch(ch)
		{
			/******************************************************************
			* handles the ascii translation of arrow characters
			******************************************************************/
			case '\033':
				getch(NULL);
				special_char = TRUE;
				continue;
				break;

				/******************************************************************
                * ignore arrow characters and tab
                ******************************************************************/
			case KEYLEFT:
			case KEYRIGHT:
			case KEYUP:
			case KEYDOWN:
			case '\t':
				break;

				/******************************************************************
                * handles backspacing
                ******************************************************************/
			case DELETE:
			{
				if (len > 0)
					delete(--len, line);
				break;
			}

				/******************************************************************
                * if the maximum line length is not exceeded the program will print
                * the character. if the character is not a new line then continue.
                * Else, terminate line, pass it to the execute_commands methond,
                * allocate a new line, and reset the line length.
                ******************************************************************/
			default:
			{
				if (len < MAX_LEN)
				{

					if (ch != '\n')
					{
						printf("%c", ch);
						line[len++] = ch;
					}

					else if (ch == '\n' && len > 0)
					{
						printf("%c", ch);
						line[len] = '\0';
						status = execute_commands(line);

						free(line);
						if (status == TERMINATE) exit(SUCCESS);

						line = (char*)malloc(MAX_LEN);
						len = 0;

						print_user();
					}
				}
				break;
			}
		}
	}

	fclose(fp);
}

/******************************************************************************
* execute_commands will process and execute the commands in the variable line.
******************************************************************************/
short execute_commands(char* line) {

	short status = SUCCESS;

	int fd[2];

	int IN_FILE = 0;
	int OUT_FILE = 1;

	char *temp[256]={NULL};

	/*****************************************************************************************
	 * checks if the line has any two consecutive operands, and will return FAILURE if it does
	 ****************************************************************************************/

	if (!(is_valid_command(strdup(line)))){
		printf("Not a valid command, try again!\n");
		return FAILURE;
	}

	/*****************************************************************************************
 	* parses the line with delimiter | and saves the tokens in temp
 	****************************************************************************************/
	parse( line , temp , "|" );

	int i = 0;

	while( temp[i] != NULL ) {

		char *tokens[256] = {NULL};

		check_out_file(temp[i],&OUT_FILE);
		check_in_file(temp[i],&IN_FILE);

		parse( temp[i] , tokens , CMD_DEL );

		if(tokens[0]==NULL){ // if the line only includes operands
			printf("error: not a valid command\n");
			return FAILURE;
		}

		else {
			pipe(fd);
			pid_t  pid;
			pid = fork();

			/*****************************************************************
 			* if the fork() call wasn't successful
 			******************************************************************/
			if( pid < 0 ) {
				printf("error: can not fork\n");
				return FAILURE;
			}

			/*****************************************************************
             * CHILD
             ******************************************************************/
			else if(pid  == 0 ) {

				if ( is_exit(tokens[0])) { // if the command is exit (non-sensitive)
					return TERMINATE;
				}

				if(IN_FILE != 0) {
					dup2(IN_FILE,STDIN_FILENO);
					close(IN_FILE);
				}

				if(OUT_FILE != 1) {
					dup2(OUT_FILE,STDOUT_FILENO);
					close(OUT_FILE);
				}

				if(temp[i+1]!=NULL) {
					dup2(fd[1],STDOUT_FILENO);
					close(fd[1]);
				}

				if(execvp(*tokens,tokens)<0) { // executes the command
					printf("error: Not an executable command\n");
					return FAILURE;
				}
			}

			/*****************************************************************
 			* PARENT
 			******************************************************************/
 			else {
 				wait(NULL); // waits for child

 				IN_FILE=fd[0];
 				close(fd[1]);

 				if ( is_exit(tokens[0])){ // checks if commands is exit
 					return TERMINATE;
 				}

 			}
		}

		i++;
	}

	IN_FILE=0;
	OUT_FILE=1;
	return status;
}


/**************************************************************************
* parses a string based on delims and saves it in an array of chars (tokens)
**************************************************************************/
void parse( char *line , char **tokens , char* delim) {
	char *cur_token;
	cur_token = strtok( line , delim );
	int counter = 0;
	while( cur_token != NULL )
	{
		tokens[counter++] = cur_token;
		cur_token = strtok( NULL, delim);
	}
}


/**************************************************************************
* checks if a command is exit (not case-sensitive)
**************************************************************************/
bool is_exit(char* input){

	remove_space(input);

	if (((input[0]=='e') || (input[0]=='E'))
		&& ((input[1]=='x') || (input[1]=='X'))
		&& ((input[2]=='i') || (input[2]=='I'))
		&& ((input[3]=='t') || (input[3]=='T'))
		&& input[4]=='\0'){
		return true;
	}
	return false;
}



void check_in_file(char *tokens, int *INFILE){

	char *temp[256]={NULL} ;
	parse( tokens , temp , "<" );

	if(temp[1]!=NULL){
		char *temp1[256]={NULL};
		parse( temp[1] , temp1 , " " );
		*INFILE = open(temp1[0],O_RDONLY);
	}
}


void check_out_file(char *tokens,int *OUTFILE){
	char *temp[256]={NULL} ;

	parse( tokens , temp , ">" );

	if(temp[1]!=NULL) {
		char *temp1[256]={NULL};
		parse( temp[1] , temp1 , " " );
		*OUTFILE = open(temp1[0],O_TRUNC | O_WRONLY | O_CREAT, S_IRWXU);
	}
	else
		*OUTFILE= 1;
}



/**************************************************************************
* checks if a line of command is valid
**************************************************************************/
bool is_valid_command(char *line){

	// remove the space and blanks
	remove_space(line);

	for (int i = 0; line[i]!='\0'; i++){

		// if two consecutive operands are found, returns false (line isn't a valid command)
		if (two_consecutive_operands(line[i],line[i+1])){
			return false;
		}
	}

	return true;

}

/**************************************************************************
* checks if two given characters are both operands
**************************************************************************/
bool two_consecutive_operands(char a, char b){
	if (a == '<' || a == '>' || a == '|' || a == '&' ){
		if (b == '<' || b == '>' || b == '|' || b == '&'){
			return true;
		}
	}
	return false;
}


/**************************************************************************
* gets a string and removes spaces from it
**************************************************************************/
void remove_space(char *line){

	char blank[1000];
	int a = 0, b = 0;

	while (line[a] != '\0') {

		if (line[a] == ' ') {

			int temp = a + 1;

			if (line[temp] != '\0') {

				while (line[temp] == ' ' && line[temp] != '\0') {
					if (line[temp] == ' ') {
						a++;
					}

					temp++;
				}
			}
		}

		blank[b] = line[a];
		a++;
		b++;
	}
}