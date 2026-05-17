#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

int has_ID;

#define REG_NUMBER

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



void generateCode(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                break;
            case INT:
                break;
            case ASSIGN:
                break;
            case ADDSUB_ASSIGN:
                break;
            case INCDEC:
                break;
            case ADDSUB:
            case MULDIV:
                break;
            case AND:
            case OR:
            case XOR:
                break;
            default:
                retval = 0;
        }
    }
    return retval;
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
