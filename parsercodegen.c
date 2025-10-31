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

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_CODE_LENGTH 1000
#define MAX_TOKENS 1000
#define MAX_IDENT_LEN 12
#define MAX_NUMBER_LEN 5
#define TOKEN_FILENAME "tokens.txt"
#define CODE_FILENAME "elf.txt"

// Enum Definitions
enum token_type {
    skipsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, eqlsym, neqsym,
    lessym, leqsym, gtrsym, geqsym, lparentsym,
    rparentsym, commasym, semicolonsym, periodsym, becomessym,
    beginsym, endsym, ifsym, fisym, thensym, whilesym,
    dosym, callsym, constsym, varsym, procsym,
    writesym, readsym, elsesym, evensym
};

enum opcode {
    LIT = 1, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS
};

enum symbol_kind {
    CONSTANT = 1, VARIABLE = 2
};

// Struct Definitions
typedef struct {
    int kind;        // const = 1, var = 2
    char name[MAX_IDENT_LEN];   // symbol name
    int val;         // value for constants
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
    char name[MAX_IDENT_LEN];   // identifier name or number string
    int value;       // numeric value
} token;

// Global Variables
instruction code[MAX_CODE_LENGTH];
symbol sym_table[MAX_SYMBOL_TABLE_SIZE];

int code_index = 0; // Next available code index
int sym_index = 0;  // Next available symbol table index
int token_list[10000]; // Array to hold all tokens
char token_lexeme[10000][MAX_IDENT_LEN]; // Array to hold lexemes/values
int token_count = 0; // Total tokens read
int token_ptr = 0;   // Current token index
int error_flag = 0;  // Flag to indicate an error has occurred
FILE *code_file;     // File pointer for elf.txt

// The current token's ID, lexeme/value, and numeric value (if applicable)
int current_token;
char current_lexeme[MAX_IDENT_LEN];
int current_number_val; // For numbersym

// Function Prototypes
void read_token_list();
void advance_token();
void emit(int op, int l, int m);
void error(int code);
int find_symbol(const char *name, int kind);
int add_symbol(int kind, const char *name, int val, int level, int addr);
void print_assembly_code();
void print_symbol_table();
void mark_all_symbols();
void block(int level, int *data_size);
void const_declaration(int level);
void var_declaration(int level, int *data_size);
void statement(int level);
void condition(int level);
void expression(int level);
void term(int level);
void factor(int level);


// Load tokens from "tokens.txt" into tokenList
void read_token_list()
{
    FILE *fp = fopen(TOKEN_FILENAME, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open input file '%s'. Ensure 'lex.c' was run successfully.\n", TOKEN_FILENAME);
        exit(EXIT_FAILURE);
    }

    token_count = 0;

    // Loop until we can't read another token ID
    while (fscanf(fp, "%d", &token_list[token_count]) == 1) {
        int token_id = token_list[token_count];
        token_lexeme[token_count][0] = '\0'; // initialize

        if (token_id == identsym) {
            if (fscanf(fp, "%s", token_lexeme[token_count]) != 1) {
                fprintf(stderr, "Error: Expected identifier after identsym at token %d\n", token_count);
                break;
            }
        }
        else if (token_id == numbersym) {
            int num_val;
            if (fscanf(fp, "%d", &num_val) != 1) {
                fprintf(stderr, "Error: Expected number after numbersym at token %d\n", token_count);
                break;
            }
            snprintf(token_lexeme[token_count], MAX_IDENT_LEN, "%d", num_val);
        }

        token_count++;
        if (token_count >= MAX_TOKENS) break;
    }

    fclose(fp);
}


// Advance to the next token in the token list
void advance_token() {
    if (error_flag) return;

    if (token_ptr < token_count) {
        current_token = token_list[token_ptr];
        
        if (current_token == skipsym) {
            error_flag = 1;
            error(1);
            return;
        }

        strncpy(current_lexeme, token_lexeme[token_ptr], MAX_IDENT_LEN);
        current_lexeme[MAX_IDENT_LEN-1] = '\0';

        if (current_token == numbersym) {
            current_number_val = atoi(current_lexeme);
        } else {
            current_number_val = 0;
        }

        token_ptr++;
    } else {
        current_token = skipsym;
        strcpy(current_lexeme, "");
        current_number_val = 0;
    }
}
// Error handling function
void error(int code) {
    if (error_flag) return;
    error_flag = 1;
    char *msg;
    switch (code) {
        case 1:  msg = "Error: Scanning error detected by lexer (skipsym present)"; break; // lexer error
        case 2:  msg = "Error: const, var, and read keywords must be followed by identifier"; break; // identifier expected
        case 3:  msg = "Error: symbol name has already been declared"; break; // duplicate symbol
        case 4:  msg = "Error: constants must be assigned with ="; break; // '=' expected
        case 5:  msg = "Error: constants must be assigned an integer value"; break; // number expected
        case 6:  msg = "Error: constant and variable declarations must be followed by a semicolon"; break; // semicolon expected
        case 7:  msg = "Error: undeclared identifier"; break; // undeclared identifier
        case 8:  msg = "Error: only variable values may be altered"; break; // assignment to non-variable
        case 9:  msg = "Error: assignment statements must use :="; break;// ':=' expected
        case 10: msg = "Error: begin must be followed by end"; break;// 'end' expected
        case 11: msg = "Error: if must be followed by then"; break;// 'then' expected
        case 12: msg = "Error: while must be followed by do"; break;// 'do' expected
        case 13: msg = "Error: condition must contain comparison operator"; break;// relational operator expected
        case 14: msg = "Error: right parenthesis must follow left parenthesis"; break;// ')' expected
        case 15: msg = "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols"; break;// factor expected
        case 16: msg = "Error: program must end with period"; break;// '.' expected
        case 32: msg = "Error: if must be followed by fi"; break;// 'fi' expected
        default: msg = "Error: Unknown error occurred"; break;// unknown error
    }

    fprintf(stderr, "%s\n", msg);// Print to stderr
    fprintf(code_file, "%s\n", msg);// Print to elf.txt
    fclose(code_file);// Close output file
    exit(EXIT_SUCCESS);
}

// function to emit instructions
void emit(int op, int l, int m) {
    if (code_index >= MAX_CODE_LENGTH) {
        fprintf(stderr, "Error: Code array overflow.\n");
        exit(EXIT_FAILURE);
    }
    // Add instruction to code array
    code[code_index].op = op;
    code[code_index].l = l;
    code[code_index].m = m;
    code_index++; // increment code index
}


// function to print assembly code
void print_assembly_code() {
    // mnemonic def for opcodes
    char *opname[] = {"", "LIT", "OPR", "LOD", "STO", "CAL", "INC", "JMP", "JPC", "SYS"};
    
    // Print column header
    printf("Line OP L M\n");
    // loop through code array and print instructions
    for (int i = 0; i < code_index; i++) {
        printf("%3d %s %d %d\n", i, opname[code[i].op], code[i].l, code[i].m);
    }
}


// function to print symbol table
void print_symbol_table() {
    // symbol table header
    printf("\nSymbol Table:\n");
    printf("Kind | Name        | Value | Level | Address\n");
    printf("-----|-------------|-------|-------|--------\n");

    //loop through symbol table and print entries
    for (int i = 0; i < sym_index; i++) {
        printf("%4d | %-11s | %5d | %5d | %7d\n", // formatting/alignment
               sym_table[i].kind, sym_table[i].name, sym_table[i].val, 
               sym_table[i].level, sym_table[i].addr);
    }
    printf("\n");
}


// function to mark all symbols as used (set mark to 1)
void mark_all_symbols() {
    for (int i = 0; i < sym_index; i++) {
        sym_table[i].mark = 1;
    }
}


// writes to elf.txt
void write_code_to_file() {
    // loop through code array and write instructions elf.txt
    for (int i = 0; i < code_index; i++) {
        fprintf(code_file, "%d %d %d\n", code[i].op, code[i].l, code[i].m);
    }
}


// function to find symbol in symbol table
int find_symbol(const char *name, int kind) {
    // Note: The level check is simplified since level is always 0 in HW3
    for (int i = sym_index - 1; i >= 0; i--) {
        if (strcmp(sym_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // not found
}


// function to add symbol to symbol table
int add_symbol(int kind, const char *name, int val, int level, int addr) {

    // overflow
    if (sym_index >= MAX_SYMBOL_TABLE_SIZE) {
        fprintf(stderr, "Error: Symbol table overflow.\n");
        return -1;
    }
    // duplicate check
    if (find_symbol(name, level) != -1) {
        error(3);
        return -1;
    }

    // add symbol to table
    sym_table[sym_index].kind = kind;
    strncpy(sym_table[sym_index].name, name, MAX_IDENT_LEN);
    sym_table[sym_index].name[MAX_IDENT_LEN - 1] = '\0';
    sym_table[sym_index].val = val;
    sym_table[sym_index].level = level;
    sym_table[sym_index].addr = addr;

    return sym_index++; // increment symbol index upon return
}


// GRAMMAR DEFINITIONS AND PARSING FUNCTIONS


void program() {
    emit(JMP, 0, 3); // Jump to address 3 per HW3 spec requirement
    
    int data_size; // initialize data size
    block(0, &data_size); // parse main block at level 0

    // ensure program ends with period
    if (current_token != periodsym) {
        error(16);
    }
    
    emit(SYS, 0, 3); // halt instruction
}


void block(int level, int *data_size) {
    *data_size = 3; // reserve space for static link, dynamic link, return address
    const_declaration(level);
    var_declaration(level, data_size);

    emit(INC, 0, *data_size); // allocate space for variables

    statement(level);
}


void const_declaration(int level) {
    if (current_token == constsym) {
        advance_token();
        
        // process constant declarations
        do {
            if (current_token != identsym) {
                error(2);
            }
            char ident_name[MAX_IDENT_LEN];
            strcpy(ident_name, current_lexeme);
            advance_token();

            if (current_token != eqlsym) {
                error(4);
            }
            advance_token();

            if (current_token != numbersym) {
                error(5);
            }
            int val = current_number_val;
            
            add_symbol(CONSTANT, ident_name, val, level, 0);
            
            advance_token();
            
        } while (current_token == commasym && (advance_token(), 1));

        if (current_token != semicolonsym) {
            error(6);
        }
        advance_token();
    }
}

void var_declaration(int level, int *data_size) {
    // Handle variable declarations
    if (current_token == varsym) {
        advance_token();
        
        do {
            if (current_token != identsym) {
                error(2);
            }
            
            add_symbol(VARIABLE, current_lexeme, 0, level, *data_size);
            (*data_size)++;
            
            advance_token();
            
        } while (current_token == commasym && (advance_token(), 1));

        if (current_token != semicolonsym) {
            error(6);
        }
        advance_token();
    }
}


void statement(int level) {
    int sym_idx;
    int cx1, cx2;
    // Handle different statement types
    if (current_token == identsym) {
        char ident_name[MAX_IDENT_LEN];
        strcpy(ident_name, current_lexeme);
        sym_idx = find_symbol(ident_name, level);

        if (sym_idx == -1) {
            error(7);
        }
        if (sym_table[sym_idx].kind != VARIABLE) {
            error(8);
        }

        advance_token();
        
        if (current_token != becomessym) {
            error(9);
        }
        advance_token();

        expression(level);
        
        emit(STO, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
    } else if (current_token == readsym) {// read statement
        advance_token();
        if (current_token != identsym) {
            error(2);
        }
        
        sym_idx = find_symbol(current_lexeme, level);
        if (sym_idx == -1) {
            error(7);
        }
        if (sym_table[sym_idx].kind != VARIABLE) {
            error(8);
        }
        
        emit(SYS, 0, 2);
        emit(STO, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
        advance_token();

    } else if (current_token == writesym) {// write statement
        advance_token();
        expression(level);
        emit(SYS, 0, 1);

    } else if (current_token == beginsym) {// begin...end block
        advance_token();
        statement(level);
        
        while (current_token == semicolonsym) {
            advance_token();
            statement(level);
        }

        if (current_token != endsym) {
            error(10);
        }
        advance_token();

    } else if (current_token == ifsym) {// if...then...fi statement
        advance_token();
        condition(level);

        if (current_token != thensym) {
            error(11);
        }
        advance_token();

        cx1 = code_index;
        emit(JPC, 0, 0); 
        
        statement(level);
        
        code[cx1].m = code_index;

        if (current_token != fisym) {
            error(32);
        }
        advance_token();

    } else if (current_token == whilesym) {// while...do statement
        advance_token();
        
        cx1 = code_index;
        condition(level);
        
        if (current_token != dosym) {
            error(12);
        }
        advance_token();
        
        cx2 = code_index;
        emit(JPC, 0, 0); 
        
        statement(level);
        
        emit(JMP, 0, cx1);
        
        code[cx2].m = code_index;
    }
}


void condition(int level) {
    if (current_token == evensym) {
        advance_token();
        expression(level);
        emit(OPR, 0, 11);  // EVEN per ISA Table 2
    } else {
        expression(level); // left-hand side
        
        int rel_op = current_token;
        if (rel_op < eqlsym || rel_op > geqsym) {
            error(13);
        }
        advance_token(); // consume relational operator

        expression(level); // right-hand side

        // Emit appropriate OPR instruction based on relational operator
        // Per ISA Table 2: EQL=5, NEQ=6, LSS=7, LEQ=8, GTR=9, GEQ=10
        switch (rel_op) {
            case eqlsym:  emit(OPR, 0, 5); break;  // EQL
            case neqsym:  emit(OPR, 0, 6); break;  // NEQ
            case lessym:  emit(OPR, 0, 7); break;  // LSS
            case leqsym:  emit(OPR, 0, 8); break;  // LEQ
            case gtrsym:  emit(OPR, 0, 9); break;  // GTR
            case geqsym:  emit(OPR, 0, 10); break; // GEQ
        }
    }
}


void expression(int level) {
    int op;
    term(level);
    // Handle addition and subtraction
    while (current_token == plussym || current_token == minussym) {
        op = current_token;
        advance_token();
        term(level);
        if (op == plussym) {
            emit(OPR, 0, 1);  // ADD per ISA Table 2
        } else {
            emit(OPR, 0, 2);  // SUB per ISA Table 2
        }
    }
}


void term(int level) {
    int op;
    factor(level);
    // Handle multiplication and division
    while (current_token == multsym || current_token == slashsym) {
        op = current_token;
        advance_token();
        factor(level);
        if (op == multsym) {
            emit(OPR, 0, 3);  // MUL per ISA Table 2
        } else {
            emit(OPR, 0, 4);  // DIV per ISA Table 2
        }
    }
}


void factor(int level) {
    int sym_idx;
    // Handle identifier, number, or parenthesized expression
    if (current_token == identsym) {
        sym_idx = find_symbol(current_lexeme, level);
        if (sym_idx == -1) {
            error(7);
        }
        // Load constant or variable value
        if (sym_table[sym_idx].kind == CONSTANT) {
            emit(LIT, 0, sym_table[sym_idx].val);
        } else if (sym_table[sym_idx].kind == VARIABLE) {
            emit(LOD, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
            // advance_token();
        }

        advance_token();
    } else if (current_token == numbersym) {
        emit(LIT, 0, current_number_val);
        advance_token();
    } else if (current_token == lparentsym) {
        advance_token();
        expression(level);
        if (current_token != rparentsym) {
            error(14);
        }
        advance_token();
    } else {
        error(15);
    }
}


// --- MAIN FUNCTION ---
int main(void) {
    code_file = fopen(CODE_FILENAME, "w"); // Open output file
    if (!code_file) { // Check for file open error
        fprintf(stderr, "Error: Could not open output file '%s'.\n", CODE_FILENAME);
        return EXIT_FAILURE;
    }

    read_token_list(); // Load tokens from file
    // printf("Tokens loaded:\n");
    // // Print loaded tokens for debugging
    // for (int i = 0; i < token_count; i++) {
    //     printf("%2d: token=%2d lexeme='%s'\n", i, token_list[i], token_lexeme[i]);
    // }
    // Check if any tokens were read
    if (token_count == 0) {
        fprintf(stderr, "Error: Token input file '%s' is empty or invalid.\n", TOKEN_FILENAME);
        fprintf(code_file, "Error: Token input file '%s' is empty or invalid.\n", TOKEN_FILENAME);
        fclose(code_file);
        return EXIT_SUCCESS;
    }

    advance_token(); // Initialize first token
    
    if (current_token == skipsym) {
        error(1); 
    }

    program(); // Start parsing

    if (!error_flag) {
        mark_all_symbols(); // Mark all symbols as used before exit
        print_symbol_table();
        print_assembly_code();
        write_code_to_file();
        printf("Parsing and code generation successful. Output written to %s.\n", CODE_FILENAME);
    }

    fclose(code_file); //Finished wooooo
    return EXIT_SUCCESS;
}