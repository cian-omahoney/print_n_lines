/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	File Name:      print_n_lines.c
	Description: 	The following program reads the first n
			lines from a specified file and prints
			these lines to the standard output.
			This is a similar to the UNIX utility
			head.
			The user can specify a number of options:
				-n  k
					By default, prints the first
					10 lines from the specified file.
					This can be changed with the
					n option. Note that a negative
					k will print all but
					the last k lines of the file.
				-V	
					Prints version information.
				-h	
					Prints help information.
				-e|-o	
					User can choose whether to
					print the odd or even lines
					only. By default all lines
					are printed.
			If no file is specifed, this program reads
			from the standard input instead.
	Completion:	All the features listed above work well.
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	12/2/2021
	Version:     	1.0

======================================================================*/


/*======================================================================
Systems header files
======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>	// for getopt()
#include <errno.h>	// for errno


/*=====================================================================
Constant definitions
=====================================================================*/
#define VERSION_NO 1.0
#define DEBUG 0		// 0: Debug mode off. 1: Debug mode on.
#define READ_ONLY "r"
#define P_FAILURE -1	//Failure to print
#define P_SUCCESS  1	//Successful to print
#define MAX_BUFFER 256	//Maximum allowed buffer size
#define DEFAULT_LINES 10	//Specifies the number of lines to print by default

/*=====================================================================
Function Definitions
=====================================================================*/

/*FUNCTION PROTOTYPES*/
int print_head(FILE*, int, int);
int line_count(FILE*);
int print_stdin(int, int);

/*Define 'Listing' type which holds users choice of even or odd lines*/
typedef enum{NOT_CHOSEN, ODD, EVEN} Listing;

/*FUNCTION: main()*/
int main(int argc, char *argv[])
{
	FILE *input;
	int opt_char;
	int n_lines = DEFAULT_LINES;
	float version = VERSION_NO;
	Listing e_or_o = NOT_CHOSEN;	//Users choice of even or odd lines listing

	while((opt_char = getopt(argc, argv, "n:Vheo")) != -1)
	{
		switch(opt_char)
		{
			/*n:	Option to change number of lines printed.*/
			case 'n':
				if(atoi(optarg) != 0)
				{
					n_lines = atoi(optarg);
					break;
				}
				//If not option argument entered or argument is zero
				else
				{
					fprintf(stdout, "%s: option requires an argument -- 'n'\n", argv[0]);
					exit(EXIT_FAILURE);
				}

			/*V:	Option to print the version information.*/
			case 'V':
				fprintf(stdout,"Program:\t%s\n", argv[0]);
				fprintf(stdout,"Source:\t\tprint_n_lines.c\n");
				fprintf(stdout,"Version:\t%.1f\n", version);
				fprintf(stdout,"Author:\t\tCian O'Mahoney\n\t\t19351611\n");
				fprintf(stdout, "\t\tcian.omahoney@ucdconnect.ie\n");
				exit(EXIT_SUCCESS);

			/*h:	Option to print help information.*/
			case 'h':
				fprintf(stdout,"Usage:\t\t%s [OPTION]... [FILE]\n", argv[0]);
				fprintf(stdout,"Options:\t-n K\tOutput first K lines\n");
				fprintf(stdout,"\t\t-V\tVersion information\n");
				fprintf(stdout,"\t\t-h\tHelp information\n");
				fprintf(stdout,"\t\t-e|o\tPrint only even or odd lines\n");
				exit(EXIT_SUCCESS);

			/*e:	Option to choose even line listing.*/
			case 'e':
				if(e_or_o != ODD)
				{
					e_or_o = EVEN;
					break;
				}
				else
				{
					fprintf(stdout, "You can only choose odd or even line listing, not both.\n");
					exit(EXIT_FAILURE);
				}

			/*o:	Option to choose odd line listing.*/
			case 'o':
				if(e_or_o != EVEN)
				{
					e_or_o = ODD;
					break;
				}
				else
				{
					fprintf(stdout,"You can only choose odd or even line listing, not both.\n");
					exit(EXIT_FAILURE);
				}
			/*?:	Incorrect option or option argument.*/
			default:
				fprintf(stdout, "Usage: %s [OPTION]... [FILE]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}
		
	/*If no file argument is given, print from standard input.*/
	if(argv[optind] == NULL)
	{
		if(DEBUG)
			fprintf(stdout, "Reading from standard input:\n");

		if(print_stdin(n_lines, e_or_o) == P_FAILURE)
		{
			fprintf(stderr, "Error in 'main': Failed to print first %d lines from standard input.\n", n_lines);
			exit(EXIT_FAILURE);
		}
	}

	/*If a file argument is given, print from this file.*/
	else
	{
		//Open user specified file
		if((input = fopen(argv[optind], READ_ONLY)) == NULL)
		{
			fprintf(stderr, "Error in 'main': Unable to open file %s.\n", argv[optind]);
			exit(EXIT_FAILURE);
		}
		else
		{
			if(DEBUG)
				fprintf(stdout, "Reading the first %d lines from file %s:\n\n",n_lines, argv[optind]);

			/*If function print_head() fails to print the requested lines, return an error*/
			if(print_head(input, n_lines, e_or_o) == P_FAILURE)
                        {
                        	fprintf(stderr, "Error in 'main': Failed to print first %d lines of file '%s'.\n", n_lines, argv[optind]);
				fclose(input);
				input = NULL;
				exit(EXIT_FAILURE);
			}

			fclose(input);
			input = NULL;
		}
	}
	return 0;
}//END of 'main()' function.




/*FUNCTION: print_head()
 *	    Function to print the first n lines from
 *	    a specified file. Has optional functionality to
 *	    only print odd or even lines.
 */
int print_head(FILE *input, int n_lines, int line_listing)
{
	char *line = NULL;
	ssize_t characters_read = 0;
	size_t line_len = MAX_BUFFER;
	int n_lines_printed = 0;	//Number of lines read from stream
	int n_lines_read = 0;		//Number of lines printed to standard output

	/* If the user has specified a negative argument k for the 'n' option
	 * calculate the total number of lines in the file, then set the
	 * number of lines to be printed to be this total number less k.
	 * In doing this, all but the last k lines will be printed.
	 */
	if(n_lines < 0)
	{
		int total_file_lines = line_count(input);

		/* If the total number of lines minus k is not positive, print zero lines.*/
		if((total_file_lines + n_lines) <= 0)
		{	
			if(DEBUG)	
				fprintf(stdout, "This file only contains %d lines.", total_file_lines);
			n_lines = -total_file_lines;
		}
		n_lines = total_file_lines + n_lines;
	}

	/* The following loop construct will read lines from a file and print them
	 * to the standard output until either the end of file is reached, an error occurs
	 * or the number of lines requested to be printed is reached.
	 */
	while(((characters_read = getline(&line, &line_len, input)) != -1) && (n_lines > n_lines_printed))
	{
		n_lines_read++;

		/*If neither odd nor even line listing is chosen, print all lines.*/
		if(line_listing == NOT_CHOSEN)
		{
			fprintf(stdout, "%s", line);
			n_lines_printed++;
		}
		/*If either odd or even line listing is chosen, print only alternating lines.*/
		else if(((n_lines_read + line_listing) % 2) == 0)
		{
			fprintf(stdout ,"%s",line);
			n_lines_printed++;
		}
	}

	if(characters_read == -1)
	{
		/* If getline() returns -1 and the end of file is not reached,
		 * an error has occured.*/
		if (!feof(input))
		{	
			fprintf(stderr, "Error in 'print_head': Failed to read file.\n");
			return P_FAILURE;
		}

		/* If the end of file is reached before the requested number of lines
		 * is printed, no problem will arise and the program will continue as normal.*/
		else if (feof(input) && (n_lines > n_lines_printed))
		{
			if(DEBUG)
				fprintf(stderr, "Warning in 'print_head': Only able to read %d lines from file.\n", n_lines_printed);
		}
	}

	if(DEBUG)
		fprintf(stdout, "\nPrinted the first %d lines from file using line listing mode %d.\n", n_lines_printed, line_listing);

	return P_SUCCESS;

}//END of 'print_head()' function




/*FUNCTION: line_count()
 * This function returns the number of lines in a given file.
 */
int line_count(FILE *input)
{
	char c;
	int lcount = 0;

	/*Count the number of new line characters in file*/
	while((c = fgetc(input)) != EOF)
	{
		if(c == '\n')
			lcount++;
	}

	/* Return file position to start of file*/
	rewind(input);	

	/* Return number of lines in file*/
	return lcount;

}//END of 'line_count()' function




/*FUNCTION: print_stdin()
 * The function reads a specified number lines from the standard input and 
 * prints them to the standard output. There is optional functionality to
 * print only the odd or even lines.
 */
int print_stdin(int n_lines, int line_listing)
{
        char *line = NULL;
        size_t line_len = MAX_BUFFER;
        int n_lines_printed = 0;	//Number of lines printed to standard output
        int n_lines_read = 0;		//Number of lines read from standard input
        
	/*If the user requested to print a negative quantity of lines from the standard input,
	 * currently this requst is ignored.*/
        if(n_lines < 0)
		n_lines = abs(n_lines);
        
        while(n_lines > n_lines_printed && (getline(&line, &line_len, stdin) != -1))
	{
		n_lines_read++;

		//If no line listing is specified
		if(line_listing == NOT_CHOSEN)
        	{
        		fprintf(stdout, "%s", line);
        		n_lines_printed++;
        	}
		//If even or odd line listing is specified, only print as required.
        	else if(((n_lines_read + line_listing) % 2) == 0)
		{
			fprintf(stdout, "%s", line);
			n_lines_printed++;
		}
	}

	//If getline() returns no error, return success. If getline() returns any error, return failure.
	if(errno == 0)
	{
		if(DEBUG)
			fprintf(stdout, "\nPrinted the first %d lines from standard input using line listing mode %d.\n", n_lines_printed, line_listing);
		return P_SUCCESS;
	}
	else
	{	
		fprintf(stderr,"Error in 'print_stdin': Unable to read lines.");
		return P_FAILURE;
	}

}//END of 'print_stdin()' function.

