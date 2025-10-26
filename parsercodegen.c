/*
    Assignment:
    HW3 - Parser and Code Generator for PL/0

    Author(s): Collin Van Meter, Jadon Milne
    Language: C (only)
        
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
#include <string.h>

// Constants
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_CODE_LENGTH 1000
#define MAX_TOKENS 1000

// Struct Definitions
typedef struct {
    int kind;        // const = 1, var = 2, proc = 3
    char name[12];   // symbol name
    int val;         // value (for constants)
    int level;       // scope level
    int addr;        // address
    int mark;        // marked for deletion
} symbol;

typedef struct {
    int op;          // operation code
    int l;           // lexicographical level
    int m;           // modifier
} instruction;

typedef struct {
    int type;        // token type
    char name[12];   // identifier name or number string
    int value;       // numeric value
} token;

// Global Variables
symbol symbolTable[MAX_SYMBOL_TABLE_SIZE];
instruction code[MAX_CODE_LENGTH];
token tokenList[MAX_TOKENS];
int tokenIndex;
int codeIndex;
int symTableSize;
FILE *output;
token currentToken;

// Function Prototypes
void loadTokens();
void emit(int op, int l, int m);
void error(const char *message);
void getNextToken();
int findSymbol(char *name);
void addSymbol(int kind, char *name, int value, int addr);

void loadTokens()
{
    FILE *file = fopen("tokens.txt", "r");
    if (file == NULL)
    {
        error("Cannot open tokens.txt");
    }

    int type;
    char name[12];
    int value;
    tokenIndex = 0;

    while (fscanf(file, "%d %s %d", &type, name, &value) == 3)
    {
        tokenList[tokenIndex].type = type;
        strcpy(tokenList[tokenIndex].name, name);
        tokenList[tokenIndex].value = value;
        tokenIndex++;
    }

    fclose(file);
    tokenIndex = 0;
    getNextToken();
}


void program();
void block();
void const_declaration();
int var_declaration();
void statement();
void condition();
void expression();
void term();
void factor();

int main()
{
    loadTokens();
    program();
    
    return 0;
}