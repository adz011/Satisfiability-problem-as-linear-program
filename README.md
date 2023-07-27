# Satisfiability-problem-as-linear-program
Satisfiability problem defined as a linear program using GLPK library and DIMACS CNF file format


# DIMACS CNF file format:

All files have .txt extension. Each one can start with a letter c subsequently having arbitrary amount of characters ending with a next line. These are comments lines and are ignored by the read function.

The following line starts with 'p cnf', indicating the start of useful information about boolean expression. First number means the quantity of variables, and the next indicates the number of clauses.

Following lines are descriptions of a clauses. Each line contains non-zero integers followed by a 0 that express the end of line. Positive integer means that the variable exists in the clause, non-positive indicates 

negated form, whereas if for any variable that doesn't appear in the clause, it's simply ignored.

**Example** of such file: 

c comments can be added here

p cnf 4 5 

1 2 -3 0

1 2 -4 0

-1 3 0

-2 -3 0
