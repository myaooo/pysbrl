/*
 * Copyright (c) 2016 Hongyu Yang, Cynthia Rudin, Margo Seltzer, and
 * The President and Fellows of Harvard College
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//
// Created by Ming Yao on 7/5/18.
//

#ifndef SBRL_RULE_H
#define SBRL_RULE_H

#include <stdlib.h>
#include <gsl/gsl_matrix.h>
#include "bit_vector.h"

/*
 * This library implements rule set management for Bayesian rule lists.
 */

/*
 * Rulelist is an ordered collection of rules.
 * A Rule is simply and ID combined with a large binary vector of length N
 * where N is the number of samples and a 1 indicates that the rule captures
 * the sample and a 0 indicates that it does not.
 *
 * Definitions:
 * captures(R, S) -- A rule, R, captures a sample, S, if the rule evaluates
 * true for Sample S.
 * captures(N, S, RS) -- In ruleset RS, the Nth rule captures S.
 */

/*
 * Even though every rule in a given experiment will have the same number of
 * samples (n_ys), we include it in the rule definition. Note that the size of
 * the captures array will be n_ys/sizeof(unsigned).
 *
 * Note that a rule outside a rule set stores captures(R, S) while a rule in
 * a rule set stores captures(N, S, RS).
 */

#define RANDOM_RANGE(lo, hi) \
    (unsigned)((lo) + (unsigned)((rand() / (float)RAND_MAX) * ((hi) - (lo) + 1)))

/*
 * We have slightly different structures to represent the original rules 
 * and rulesets. The original structure contains the ascii representation
 * of the rule; the ruleset structure refers to rules by ID and contains
 * captures which is something computed off of the rule's truth table.
 */

typedef struct rule_data {
    int cardinality;
    char *feature_str;
    bit_vector_t * truthtable;
} rule_data_t;


typedef struct rulelist_entry {
	unsigned rule_id;
    bit_vector_t * captures;
} rulelist_entry_t;

typedef struct rulelist {
	int n_rules;			    /* Number of actual rules in the ruleset. */
	int n_alloc;			    /* Spaces allocated for rules. */
	int n_samples;
	rulelist_entry_t* rules;	/* Array of rules. */
} rulelist_t;

typedef struct params {
	double lambda;
	double eta;
	// double threshold;
	int iters;
	int n_chains;
	int n_classes;
	int * alpha;
} params_t;

typedef struct data {
	rule_data_t * rules;		/* rules in BitVector form in the data */
	rule_data_t * labels;	/* labels in BitVector form in the data */
//    int * cardinalities;
	int n_rules;		/* number of rules */
	int n_samples;		/* number of samples in the data. */
    int n_classes;
} data_t;

typedef struct pred_model {
	rulelist_t *rs;		/* best ruleset. */
	gsl_matrix * theta;
} pred_model_t;


/*
 * Functions in the library
 */

//
// ruleset_entry
//

void ruleset_entry_copy(rulelist_entry_t *dest, rulelist_entry_t *src);

//
// Ruleset
//

rulelist_t * ruleset_create(int n_alloc, int n_samples);

rulelist_t * ruleset_init(int, int, const int *, rule_data_t *);
int ruleset_add(rule_data_t *, rulelist_t **, int, int);
int ruleset_backup(rulelist_t *, int **);
int ruleset_copy(rulelist_t **, rulelist_t *);
int ruleset_delete(rule_data_t *, rulelist_t *, int);
void ruleset_swap(rulelist_t *, int, int, rule_data_t *);
void ruleset_swap_any(rulelist_t *, int, int, rule_data_t *);
int pick_random_rule(int, const rulelist_t *);

void ruleset_destroy(rulelist_t *);
void ruleset_print(rulelist_t *, rule_data_t *, int);
void ruleset_entry_print(rulelist_entry_t *, int);
rulelist_t * create_random_ruleset(int, int, int, rule_data_t *);

int load_data(const char *, const char *, data_t *);
int rules_init_from_file(const char *, int *, int *, rule_data_t **,  int);
void data_free(data_t * data);
//int rules_init_from_data(int, int, char **, rule_data_t **, int);
void rules_free(rule_data_t *, int);

void rule_print(rule_data_t *, int, int);
//void rule_print_all(rule_data_t *, int);
void init_gsl_rand_gen(long seed);

/* Functions for the Scalable Baysian Rule Lists */
gsl_matrix *predict(pred_model_t *, rule_data_t *labels, params_t *);
int ruleset_proposal(rulelist_t *, int, int *, int *, char *, double *);
rulelist_t *run_mcmc(data_t *, params_t *, double);
//rulelist_t *run_simulated_annealing(data_t *, params_t *, int);
pred_model_t *train(data_t * train_data, params_t * params, long seed, int verbose);

#endif //SBRL_RULE_H
