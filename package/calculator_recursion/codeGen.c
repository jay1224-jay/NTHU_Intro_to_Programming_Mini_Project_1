#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

int has_ID;

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
