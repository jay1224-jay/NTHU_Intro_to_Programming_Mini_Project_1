#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

int has_ID;

#define REG_NUMBER 8

int reg_used[REG_NUMBER]; // 8 register
int mem_idx = 0;
int variable_reg[256]; // address of variable in regiser

int alloc_reg(void) {
    for ( int i = 0 ; i < REG_NUMBER ; ++i ) {
        if ( !reg_used[i] ) {
            reg_used[i] = 1;
            return i;
        }
    }
    puts("\n\n\nRunning out of registers\n\n\n");
    return -1;
}

void free_reg(int r) {
    reg_used[r] = 0;
}

/*

Rule for genCode:

1. r0
r0

2. r1 = r0
MOV r1 r0
return r1;

3. r1 OP r0
OP r1 r0
return r1;

*/

void mov_rm(int r, int m) {
    printf("MOV r%d [%d]\n", r, m);
}

void mov_mr(int m, int r) {
    printf("MOV [%d] r%d\n", m, r);
}

void mov_rc(int r, int c) {
    printf("MOV r%d %d\n", r, c);
}

void mov_rr(int r, int R) {
    printf("MOV r%d r%d\n", r, R);
}

int genCode(BTNode *root) {
    int retaddr = 0, r0 = 0, r1 = 0;
    int l_mem;
    char* op = NULL;
    if (root != NULL) {
        switch (root->data) {
            case ID:
                retaddr = alloc_reg();
                mov_rm(retaddr, getaddress(root->lexeme));
                break;
            case INT:
                retaddr = alloc_reg();
                mov_rc(retaddr, atoi(root->lexeme));
                break;
            case ASSIGN:
                l_mem = getaddress(root->left->lexeme);
                if ( l_mem == -1 ) {
                    // not found
                    l_mem = setaddress(root->left->lexeme);
                }
                r1 = genCode(root->right);
                mov_mr(l_mem, r1);
                free_reg(r1);
                break;
            case ADDSUB_ASSIGN:
                l_mem = getaddress(root->left->lexeme);
                if ( l_mem == -1 ) {
                    // not found
                    l_mem = setaddress(root->left->lexeme);
                }
                r0 = alloc_reg();
                mov_rm(r0, l_mem);
                r1 = genCode(root->right);
                if ( strcmp(root->lexeme, "+=") == 0 ) {
                    printf("ADD r%d r%d\n", r0, r1);
                } else {
                    printf("SUB r%d r%d\n", r0, r1);
                }
                mov_mr(l_mem, r0);
                free_reg(r1);
                free_reg(r0);
                break;
            case INCDEC:
                l_mem = getaddress(root->right->lexeme);
                retaddr = alloc_reg();
                mov_rm(retaddr, l_mem);
                r0 = alloc_reg();
                mov_rc(r0, 1);
                printf("ADD r%d r%d\n", retaddr, r0);
                free_reg(r0);
                mov_mr(l_mem, retaddr);
                break;
            case ADDSUB:
            case MULDIV:

                r0 = genCode(root->left);
                r1 = genCode(root->right);

                if (strcmp(root->lexeme, "+") == 0) {
                    op = "ADD";
                } else if (strcmp(root->lexeme, "-") == 0) {
                    op = "SUB";
                } else if (strcmp(root->lexeme, "*") == 0) {
                    op = "MUL";
                } else if (strcmp(root->lexeme, "/") == 0) {
                    op = "DIV";
                }

                printf("%s r%d r%d\n", op, r0, r1);
                retaddr = r0;
                free_reg(r1);
                break;
            case AND:
            case OR:
            case XOR:
                r0 = genCode(root->left);
                r1 = genCode(root->right);

                if (strcmp(root->lexeme, "&") == 0) {
                    op = "AND";
                } else if (strcmp(root->lexeme, "|") == 0) {
                    op = "OR";
                } else if (strcmp(root->lexeme, "^") == 0) {
                    op = "XOR";
                }

                printf("%s r%d r%d\n", op, r0, r1);
                retaddr = r0;
                free_reg(r1);
                break;
            default:
                retaddr = 0;
        }
    }
    return retaddr;
}










int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                has_ID = 1;
                retval = getval(root->lexeme);
                break;
            case INT:
                retval = atoi(root->lexeme);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                break;
            case ADDSUB_ASSIGN:
                rv = evaluateTree(root->right);
                if ( strcmp(root->lexeme, "+=") == 0 )
                    retval = setval(root->left->lexeme, getval(root->left->lexeme) + rv);
                else
                    retval = setval(root->left->lexeme, getval(root->left->lexeme) - rv);
                break;
            case INCDEC:
                // printf("var = %s, val = %d\n", root->right->lexeme, getval(root->right->lexeme));
                if ( strcmp(root->lexeme, "++") == 0 )
                    retval = setval(root->right->lexeme, getval(root->right->lexeme)+1);
                else
                    retval = setval(root->right->lexeme, getval(root->right->lexeme)-1);
                break;
            case ADDSUB:
            case MULDIV:
                lv = evaluateTree(root->left);
                has_ID = 0;
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    /*
                    
2. At least one variable in the right-hand side:

                x = 0
                y = 5 / x

This is a valid expression.
                    
                    */
                    if (rv == 0) {
                        if ( has_ID ) {
                            retval = -1;
                        } else {
                            error(DIVZERO);
                        }
                    }
                    has_ID = 0;
                    retval = lv / rv;
                }
                break;
            case AND:
            case OR:
            case XOR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                } else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                } else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                }
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}
