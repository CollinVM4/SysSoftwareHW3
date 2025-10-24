/*
Assignment:
HW3 - Parser and Code Generator for PL/0

Author(s): Collin Van Meter, Jadon Milne
    Language: C (only)]
    
    To Compile:
        Scanner:
            gcc -O2 -std=c11 -o lex lex.c
    Parser/Code Generator:
        gcc -O2 -std=c11 -o parsercodegen parsercodegen.c
    To Execute (on Eustis):
        ./lex <input_file.txt>
        ./parsercodegen

    where:
        <input_file.txt> is the path to the PL/0 source program
    Notes:
        - lex.c accepts ONE command-line argument (input PL/0 source file)
        - parsercodegen.c accepts NO command-line arguments
        - Input filename is hard-coded in parsercodegen.c
        - Implements recursive-descent parser for PL/0 grammar
        - Generates PM/0 assembly code (see Appendix A for ISA)
        - All development and testing performed on Eustis

    Class: COP3402 - System Software - Fall 2025

    Instructor: Dr. Jie Lin

    Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/

#include <stdio.h>
#include <stdlib.h>


int main()
{
    FILE *file = fopen("tokens.txt", "r"); // hardcoded filename, readmode

    if (file == NULL) 
    {
        printf("Error: cannot open tokens.txt\n");
        return 1;
    }

    char tokens[256]; // tokens array to store each line or token
    while (fgets(tokens, sizeof(tokens), file) != NULL) 
    {
        printf("%s", tokens); // Print each line read from the file
    }





    fclose(file); // Close the file
    return 0;
}