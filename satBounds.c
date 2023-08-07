//
// Created by adz011 on 05.08.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <glpk.h> // the linear programming toolkit
#include <assert.h>
#include <sys/timeb.h>


int numVars; //number of variables in the SAT instance
int numClauses; // number of clauses in the SAT instance
int *instance; // array defining the SAT instance
int *bestAssignment; // array containing the best solution you've found
int debug; // flag for debug mode; 1 means debug mode, 0 means debug off
int rowBounds;
glp_prob *lp;

/* reads SAT instance from file, creates and fills the global variables as needed,
   it returns 0 if all is okay and 1 otherwise */
int readInput(char *filename);

/* computes lower and upper bounds, stores them accordingly,
   and stores the best found assignment of variables in bestAssignment */
void bounds(int *lowerBound, int *upperBound);

/* initialises a pseudo-random number generator */
void randInit(void);

/* returns 1 with probability p and 0 otherwise */
int flipCoin(double p);


void bounds(int *lowerBound, int *upperBound) {
    int i, j;
    //
    int lowBoundInt, upperBoundInt = 0;

    int rowIndices[numVars + 2];  // array to store indices of variables of currently processed row

    double rowCoefficients[numVars + 2];  // array to store coefficients of variables of currently processed row

    int negatedVariablesInClause = 0; // variable to store quantity of negative coefficients of variables in one constraint/clause

    int currentVariableIndex = 1;  // starting index is set to 1, since GLPK library is one based indexed



    lp = glp_create_prob();

    glp_add_cols(lp, numVars + numClauses); // set number of variables to sum of number of clauses and variables

    for (i = 1; i <= numVars; i++) {
        glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0); // set lower and upper bounds to 0 and 1 respectively for variables inside clauses

    }
    for (i = numVars + 1; i <= numClauses + numVars; i++) {
        glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0); // set lower and upper bounds to 0 and 1 respectively for variables of clauses

        glp_set_obj_coef(lp, i, -1.0); // set objective coefficients of clauses' variables to -1.0

    }

    glp_set_obj_dir(lp, GLP_MIN); // set objective as minimisation of ys

    glp_add_rows(lp, numClauses); // set number of constraints to number of clauses

    for (i = 0; i < numClauses; i++) { // iterate through clauses
        currentVariableIndex = 1;
        negatedVariablesInClause = 0;
        for (j = 0; j < numVars; j++) {
            if (instance[(numVars * i) + j] == -1) { // check if variable is negated inside a clause
                rowIndices[currentVariableIndex] = j + 1;
                rowCoefficients[currentVariableIndex] = 1.0; // set coefficient to 1.0
                negatedVariablesInClause++;
                currentVariableIndex++;

            } else if (instance[(numVars * i) + j] == 1) { // check if variable is positive in a clause
                rowIndices[currentVariableIndex] = j + 1;
                rowCoefficients[currentVariableIndex] = -1.0; // set coefficient to -1.0
                currentVariableIndex++;

            } else {

            }

        }
        rowIndices[currentVariableIndex] = numVars + i + 1; // clause variable
        rowCoefficients[currentVariableIndex] = 1.0;


        double lowBound = negatedVariablesInClause; // constraint of the clause equal to quantity of negated variables in a clause

        glp_set_row_bnds(lp, i + 1, GLP_UP, lowBound, lowBound); // set lower bound of a constraint
        glp_set_mat_row(lp, i + 1, currentVariableIndex, rowIndices, rowCoefficients); // set coefficients of a row
    }


    glp_simplex(lp, NULL); // solve a problem using simplex algorithm
    *upperBound = glp_get_obj_val(lp); // upper bound equal to found optimal value

    for (i = 1; i <= numVars; i++) {

        bestAssignment[i] = glp_get_col_prim(lp, i);
    }

    glp_delete_prob(lp); //release memory for the next lp
}


void printResults(int lowerBound, int upperBound) {
    int i; /* loop variable to go over assignment */

    fprintf(stdout, "Between %d and %d clauses can be satisfied simultaneously of %d.\n"
                    "Best found assignment: ", lowerBound, upperBound, numClauses);
    for (i = 1; i <= numVars; i++) {
        printf("Index %d, value: %d \n", i, bestAssignment[i]);
    }
    fprintf(stdout, "\n");

}

void printInstance(void) {
    int i, j; /* loop variables to go over clauses and variables */
    int prev; /* flags if there was a previous variable */

    for (i = 0; i < numClauses; i++) {
        fprintf(stdout, "Clause %d: ", i + 1);
        prev = 0; /* no variable printed yet */
        for (j = 0; j < numVars; j++) {
            if (instance[numVars * i + j] != 0) {
                if (prev) { /* need OR after previous variable */
                    fprintf(stdout, " OR ");
                } else { /* after printing first we have a previous variable */
                    prev = 1;
                }
                if (instance[numVars * i + j] < 0) {
                    fprintf(stdout, "NOT(x%d)", j + 1);
                } else {
                    fprintf(stdout, "x%d", j + 1);
                }
            }
        }
        fputc('\n', stdout); /* new line for next clause */
    }
}

int main(int argc, char **argv) {
    int i; /* used to run over the command line parameters */
    int lowerBound, upperBound; /* variables to hold resulting bounds */

    if (argc < 2) { /* no command line parameter given */
        fprintf(stderr, "Usage: %s [file1] [file2] [file3] [...]\n"
                        "Where each [file] is the name of a file with a SAT instance.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argv[1][0] == '-' && argv[1][1] == 'd' && argv[1][2] == 0) {
        /* If the first parameter is -d we activate debug mode. */
        debug = 1; /* switch debug mode on */
        fprintf(stdout, "DEBUG: Debug mode activated\n"); /* be explicit about it */
    } else {
        debug = 0; /* switch debug mode off */
    }

    randInit(); /* initialise pseudo-random number generator */
    for (i = 1 + debug; i < argc; i++) { /* go over remaining command line parameters */
        if (readInput(argv[i])) { /* try to read file */
            /* returned with error message */
            fprintf(stderr, "%s: Cannot read instance with filename %s. Skipping it.\n",
                    argv[0], argv[i]);
        } else { /* input read successfully */
            fprintf(stdout, "Looking at instance from %s\n", argv[i]);
            if (debug) {
                printInstance();
            }
            bounds(&lowerBound, &upperBound); /* compute bounds and best instance */
            printResults(lowerBound, upperBound); /* print assignment and bounds */
            /* free memory for next input; avoid a memory leak */
            free(instance);
            free(bestAssignment);
        }
    }
    return EXIT_SUCCESS;
}

/* processClause(char *line, int clause) translates next clause (given as */
/* string) and stores it in instance * at the position defined by clause */
/* return value 1 indicates an error; otherwise 0 is returned */
int processClause(char *line, int clause) {
    long var; /* current variable */
    int pos; /* position of the variable */

    do {
        var = strtol(line, &line, 10);
        if (var >= -numVars && var <= numVars) {
            if (var != 0) { /* actual variable found */
                if (debug) {
                    if (var < 0) {
                        fprintf(stdout, "Have variable x%ld negated in clause %d.\n",
                                -var, clause + 1);
                    } else {
                        fprintf(stdout, "Have variable x%ld in clause %d.\n", var, clause + 1);
                    }
                }
                /* check that the number of the current clause is legal */
                if (clause >= numClauses) {
                    if (debug) {
                        fprintf(stdout, "More than %d clauses in file. Abort.\n", numClauses);
                    }
                    return 1; /* and we flag that we could not read the input */
                }
                if (var < 0) {
                    pos = (-(int) var) - 1;
                } else {
                    pos = ((int) var) - 1;
                }
                if (instance[numVars * clause + pos] != 0) {
                    if (debug) {
                        fprintf(stdout, "Encounter variable %ld in clause %d again. Abort.\n",
                                var, clause + 1);
                    }
                    return 1; /* abandon this clause */
                }
                if (var < 0) {
                    instance[numVars * clause + pos] = -1; /* mark variable negated in clause */
                } else {
                    instance[numVars * clause + pos] = 1; /* mark variable positive in clause */
                }
            }
        } else {
            if (debug) {
                fprintf(stdout, "Illegal variable found: %ld. Abort file.\n", var);
            }
            return 1; /* abondon this clause */
        }
    } while (var != 0); /* var==0 signals end of clause */
    return 0; /* signal that clause was successfully stored */
}

/* readInput(*char filename) reads the input and stores it */
/* return value 1 indicates an error; otherwise 0 is returned */
int readInput(char *filename) {
    FILE *fh;
    int clause; /* counts the current clause */
    int i; /* general loop variable */
    int seenInfo; /* flags if we have seen number of clauses/variables yet */
    char c; /* holds the first character */
    char line[4096]; /* holds one line of input */

    /* open file for reading */
    if ((fh = fopen(filename, "rt")) == NULL) {
        if (debug) {
            fprintf(stdout, "Unable to open file %s.\n", filename);
        }
        return 1;
    }

    seenInfo = 0; /* we've seen nothing yet */
    /* keep reading lines until we reach the end */
    while (!feof(fh)) {
        c = fgetc(fh); /* read first character to get line type */
        switch (c) {
            case -1: /* end of file */
                break;
            case 'c': /* comment line */
                if (debug) {
                    fprintf(stdout, "Comment line encountered. Will be ignored.\nc");
                }
                /* we have to ignore the rest of the line */
                while (c != EOF && c != '\n') {
                    c = fgetc(fh);
                    if (debug) {
                        fputc(c, stdout);
                    }
                }
                if (debug) {
                    fputc('\n', stdout);
                }
                break; /* end of 'c' case */
            case 'p': /* line defining the instance */
                if (seenInfo) { /* already had a p line */
                    if (debug) {
                        fprintf(stdout, "Online one line starting with p allowed. Abort\n");
                    }
                    fclose(fh); /* we close the file that we cannot process */
                    return 1; /* and we flag that we could not read the input */
                } else {
                    seenInfo = 1; /* remember that we now see the info, hopefully */
                    /* we expect 'p cnf ' at the beginning */
                    for (i = 0; i < 6; i++) {
                        if (c != "p cnf "[i]) { /* this should not happen */
                            if (debug) {
                                fprintf(stdout, "Char %c in p cnf line expected. Found %c. "
                                                "Abort file. (%d)\n", "p cnf "[i], c, i);
                            }
                            fclose(fh); /* we close the file that we cannot process */
                            return 1; /* and we flag that we could not read the input */
                        } /* end of checking current character */
                        c = fgetc(fh); /* get next character */
                    } /* end of for-loop confirming correct start of line */
                    ungetc(c, fh); /* put character back in input stream */
                    /* safely read line */
                    if (fgets(line, 40, fh) != NULL) { /* line successfully read */
                        /* read number of variables and clauses */
                        if (sscanf(line, "%d %d", &numVars, &numClauses) != 2) {
                            /* failed to read number of variables or clauses */
                            if (debug) {
                                fprintf(stdout, "Failed to read instance size. Abort file.\n"
                                                "p cnf %s\n", line);
                            }
                            fclose(fh); /* we close the file that we cannot process */
                            return 1; /* and we flag that we could not read the input */
                        } else { /* size of instance read successfully */
                            if (debug) {
                                fprintf(stdout, "Reading instace with %d variables and %d clauses\n",
                                        numVars, numClauses);
                            }
                            if (numVars < 1 || numClauses < 1) {
                                if (debug) {
                                    fprintf(stdout, "Number of clauses and variables need to be positive.\n");
                                }
                                fclose(fh); /* we close the file that we cannot process */
                                return 1; /* and we flag that we could not read the input */
                            }
                            /* create memory for instance and best assignment */
                            instance = (int *) malloc(sizeof(int) * numVars * numClauses);
                            bestAssignment = (int *) malloc(sizeof(int) * numVars * numClauses);
                            assert(instance != NULL && bestAssignment != NULL); /* check memory */
                            clause = 0; /* no clause seen yet */
                            /* initialise all clauses empty and best assignment all 0s */
                            memset(instance, 0, sizeof(int) * numVars * numClauses);
                            memset(bestAssignment, 0, sizeof(int) * numVars * numClauses);
                        }
                    }
                } /* end of reading p line */
                break; /* end of p case */
            default: /* must be a clause */
                ungetc(c, fh); /* put character back in input stream */
                if (fgets(line, 4096, fh) != NULL) { /* line reading successful */
                    if (processClause(line, clause)) {
                        if (debug) {
                            fprintf(stdout, "Could not decode clause %s--about file.\n", line);
                        }
                        fclose(fh); /* we close the file that we cannot process */
                        return 1; /* and we flag that we could not read the input */
                    }
                    clause++; /* count the number of clauses we've seen */
                } /* end of processing clause line */
        } /* end of switch over first character */
    } /* end of while-loop reading intput */
    if (clause < numClauses) {
        if (debug) {
            fprintf(stdout, "Only %d clauses found. Expected %d. Abort file\n",
                    clause, numClauses);
        }
        fclose(fh); /* we close the file that we cannot process */
        return 1; /* and we flag that we could not read the input */
    }
    fclose(fh); /* close file after reading the input */
    return 0; /* signal all went well */
}

/* We use the built-in pseudo-random number generator. */
/* randInit() initialises it, using the system time as seed */
/* randCoin() simulates a random coin flip that yields 1 with probability p */
/* randInt(n) provides a random integer between 0 and less than n */
void randInit(void) {
    struct timeb t; /* to get current system time */

    ftime(&t);
    srand48(t.millitm * t.millitm); /* use ms to initialise PRNG */
}

/* flipCoin(p) returns 1 with probability p and 0 otherwise */
int flipCoin(double p) {
    if (drand48() < p) {
        return 1;
    } else {
        return 0;
    }
}
