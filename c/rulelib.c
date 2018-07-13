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
//#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include "rule.h"
#include "bit_vector.h"
#include "utils.h"


#ifdef DEBUG
#define DEBUG_PRINT(msg,...) printf("[%s:%i] "msg, __FILE__, __LINE__, ##__VA_ARGS__);
#define DEBUG_RUN(code) (code)
#else
#define DEBUG_PRINT(msg,...)
#define DEBUG_RUN(code)
#endif


void
rules_free(rule_data_t *rules, const int n_rules) {
    if (rules == NULL)
        return;
    rule_data_t * cur_rule;
    for (int i = 0; i < n_rules; i++) {
        cur_rule = rules + i;
//        bit_vector_dealloc(&(rules[i].truthtable));
        if (cur_rule->truthtable != NULL)
            bit_vector_free(cur_rule->truthtable);
        if (cur_rule->feature_str != NULL)
            free(cur_rule->feature_str);
    }
    free(rules);
}

void data_free(data_t * data) {
    rules_free(data->rules, data->n_rules);
    rules_free(data->labels, data->n_classes);
}

rulelist_t * ruleset_create(int n_alloc, int n_samples) {
    rulelist_t *rs = malloc(sizeof(rulelist_t));
    rs->n_alloc = n_alloc;
    rs->n_rules = 0;
    rs->n_samples = n_samples;
    rs->rules = malloc(n_alloc * sizeof(rulelist_entry_t));
    return rs;
}

/* Create a ruleset. */
rulelist_t *
ruleset_init(int nrs_rules,
             int n_samples, const int *idarray, rule_data_t *rules) {
    int i;
    rule_data_t *cur_rule;
    rulelist_entry_t *cur_re;
    bit_vector_t *not_captured = bit_vector_init((bit_size_t) n_samples);
    if (not_captured == NULL)
        return NULL;
    bit_vector_set_all(not_captured);
    /*
     * Allocate space for the ruleset structure and the ruleset entries.
     */
    rulelist_t *rs = ruleset_create(nrs_rules, n_samples);

    /*
     * Allocate the ruleset at the front of the structure and then
     * the rulelist_entry_t array at the end.
     */
    DEBUG_PRINT("START ITER");
    for (i = 0; i < nrs_rules; i++) {
        cur_rule = rules + idarray[i];
        cur_re = rs->rules + i;
        cur_re->rule_id = (unsigned) (idarray[i]);

        if ((cur_re->captures = bit_vector_init((bit_size_t) n_samples)) == NULL) {
            fprintf(stderr, "Error initiating bit_vector\n");
            goto err1;
        }
        rs->n_rules++;
        DEBUG_PRINT("%d: \n", i);
        // i.captures = not_captured & truthtable
        bit_vector_and(cur_re->captures, not_captured, cur_rule->truthtable);
        DEBUG_PRINT("AND ");
        // not_captured = not_captured & (~i.captures)
        bit_vector_and_eq_not(not_captured, cur_re->captures);
        DEBUG_PRINT("AND_EQ_NOT .");

    }
    DEBUG_PRINT("| End iter\n");
    assert(bit_vector_n_ones(not_captured) == 0);

    bit_vector_free(not_captured);
    return rs;

err1:
    bit_vector_free(not_captured);
    ruleset_destroy(rs);
    errno = ENOMEM;
    return NULL;
}

/*
 * Save the idarray for this ruleset incase we need to restore it.
 * We don't know how long the idarray currently is so always call
 * realloc, which will do the right thing.
 */
int
ruleset_backup(rulelist_t *rs, int **rs_idarray) {
    int i;
    int *ids = *rs_idarray;

    if ((ids = realloc(ids, (rs->n_rules * sizeof(int)))) == NULL)
        return (errno);

    for (i = 0; i < rs->n_rules; i++)
        ids[i] = rs->rules[i].rule_id;

    *rs_idarray = ids;

    return (0);
}

/*
 * When we copy rulesets, we always allocate new structures; this isn't
 * terribly efficient, but it's simpler. If the allocation and frees become
 * too expensive, we can make this smarter.
 */
int
ruleset_copy(rulelist_t **ret_dest, rulelist_t *src) {
    int i;
    rulelist_t *dest = ruleset_create(src->n_rules, src->n_samples);
    dest->n_rules = src->n_rules;

    for (i = 0; i < src->n_rules; i++) {
        dest->rules[i].rule_id = src->rules[i].rule_id;
        dest->rules[i].captures = bit_vector_clone(src->rules[i].captures);
    }
    *ret_dest = dest;

    return (0);
}

/* Reclaim resources associated with a ruleset. */
void
ruleset_destroy(rulelist_t *rs) {
    for (int j = 0; j < rs->n_rules; j++) {
        bit_vector_free(rs->rules[j].captures);
    }
    free(rs->rules);
    free(rs);
}

#define EXPAND_INC 1

/*
 * Add the specified rule to the ruleset at position ndx (shifting
 * all rules after ndx down by one).
 */
int
ruleset_add(rule_data_t *rules, rulelist_t **rsp, int newrule, int ndx) {
    int i;
    rulelist_t *rs;
    rulelist_entry_t *cur_re;
    bit_vector_t * not_caught;

    rs = *rsp;

    /* Check for space. */
    if (rs->n_alloc < rs->n_rules + 1) {
        rs->rules = realloc(rs->rules,
                (rs->n_alloc + EXPAND_INC) * sizeof(rulelist_entry_t));
        assert(rs->rules != NULL);
//        expand = realloc(rs, sizeof(rulelist_t) +
//                             (rs->n_rules + 1) * sizeof(rulelist_entry_t));
//        if (expand == NULL)
//            return (errno);
        rs->n_alloc = rs->n_alloc + EXPAND_INC;
        *rsp = rs;
    }

    /*
     * Compute all the samples that are caught by rules AFTER the
     * rule we are inserting. Then we'll recompute all the captures
     * from the new rule to the end.
     */
    not_caught = bit_vector_init((bit_size_t) rs->n_samples);
    for (i = ndx; i < rs->n_rules; i++) {
        // not_caught = not_caught | rs->rules[i].captures
        bit_vector_or_eq(not_caught, rs->rules[i].captures);

    }


    /*
     * Shift later rules down by 1 if necessary.
     */
    if (ndx != rs->n_rules) {
        memmove(rs->rules + (ndx + 1), rs->rules + ndx,
                (rs->n_rules - ndx) * sizeof(rulelist_entry_t));
    }

    /* Insert and initialize the new rule. */
    rs->n_rules++;
    rs->rules[ndx].rule_id = (unsigned) newrule;
    rs->rules[ndx].captures = bit_vector_init((bit_size_t) rs->n_samples);

    /*
     * Now, recompute all the captures entries for the new rule and
     * all rules following it.
     */

    for (i = ndx; i < rs->n_rules; i++) {
        cur_re = rs->rules + i;
        /*
         * Captures for this rule gets anything in not_caught
         * that is also in the rule's truthtable.
         */
        bit_vector_and(cur_re->captures, not_caught, rules[cur_re->rule_id].truthtable);
        // not_caught = not_caught & (~cur_re->captures)
        bit_vector_and_eq_not(not_caught, cur_re->captures);

    }
    assert(bit_vector_n_ones(not_caught) == 0);
    bit_vector_free(not_caught);

    return (0);
}

/*
 * Delete the rule in the ndx-th position in the given ruleset.
 */
int
ruleset_delete(rule_data_t *rules, rulelist_t *rs, int ndx) {
    int i;
    rulelist_entry_t *old_re, *cur_re;

    /* Compute new captures for all rules following the one at ndx.  */
    old_re = rs->rules + ndx;

    for (i = ndx + 1; i < rs->n_rules; i++) {
        /*
         * My new captures is my old captures or'd with anything that
         * was captured by ndx and is captured by my rule.
         */
        cur_re = rs->rules + i;
        // captures = captures | (truthtable & old_captures)
//        bit_vector_and(tmp_vec, rules[cur_re->rule_id].truthtable, old_re->captures);
//        bit_vector_or_eq(cur_re->captures, tmp_vec);
        bit_vector_or_eq_and(cur_re->captures, rules[cur_re->rule_id].truthtable, old_re->captures);

        /*
         * Now remove the ones from old_re->captures that just got set
         * for rule i because they should not be captured later.
         */
        // old->captures = old->captures & ~(cur->captures)
        bit_vector_and_eq_not(old_re->captures, cur_re->captures);
    }

    /* Now remove alloc'd data for rule at ndx and for tmp_vec. */
    bit_vector_free(rs->rules[ndx].captures);

    /* Shift up cells if necessary. */
    if (ndx != rs->n_rules - 1) {
        memmove(rs->rules + ndx, rs->rules + ndx + 1,
                (rs->n_rules - 1 - ndx) * sizeof(rulelist_entry_t));
    }

    rs->n_rules--;
    return 0;
}

/*
 * We create random rulesets for testing and for creating initial proposals
 * in MCMC
 */
rulelist_t *
create_random_ruleset(int size,
                      int n_samples, int n_rules, rule_data_t *rules) {
    int i, j, next;
    rulelist_t * ret;
    int *ids = calloc((unsigned) size, sizeof(int));
    for (i = 0; i < (size - 1); i++) {
        try_again:
        next = RANDOM_RANGE(1, (n_rules - 1));
        /* Check for duplicates. */
        for (j = 0; j < i; j++)
            if (ids[j] == next)
                goto try_again;
        ids[i] = next;
    }

    /* Always put rule 0 (the default) as the last rule. */
    ids[i] = 0;

    ret = ruleset_init(size, n_samples, ids, rules);
    free(ids);
    return (ret);
}

/*
 * Swap rules i and j, where i + 1 = j.
 *	j.captures = j.captures | (i.captures & j.tt)
 *	i.captures = i.captures & ~j.captures
 * 	then swap positions i and j
 */
void
ruleset_swap(rulelist_t *rs, int i, int j, rule_data_t *rules) {
//    bit_vector_t *tmp_vec;
    rulelist_entry_t re;

    assert(i < (rs->n_rules - 1));
    assert(j < (rs->n_rules - 1));
    assert(i + 1 == j);

//    tmp_vec = bit_vector_init((bit_size_t) rs->n_samples);

    /* j.captures = j.captures | (i.captures & j.tt) */
    /* tmp_vec =  i.captures & j.tt */
//    bit_vector_and_safe(tmp_vec, rs->rules[i].captures, rules[rs->rules[j].rule_id].truthtable);
    /* j.captures = j.captures | tmp_vec */
//    bit_vector_or_safe(rs->rules[j].captures, rs->rules[j].captures, tmp_vec);
    bit_vector_or_eq_and(rs->rules[j].captures,
                         rs->rules[i].captures, rules[rs->rules[j].rule_id].truthtable);

    /* i.captures = i.captures & ~j.captures */
    bit_vector_and_eq_not(rs->rules[i].captures, rs->rules[j].captures);

    /* Now swap the two entries */
    re = rs->rules[i];
    rs->rules[i] = rs->rules[j];
    rs->rules[j] = re;

//    bit_vector_free(tmp_vec);
}

void
ruleset_swap_any(rulelist_t *rs, int i, int j, rule_data_t *rules) {
    int cnt, cnt_check, k, temp;
    bit_vector_t * caught;

    if (i == j)
        return;

    assert(i < rs->n_rules);
    assert(j < rs->n_rules);

    /* Ensure that i < j. */
    if (i > j) {
        temp = i;
        i = j;
        j = temp;
    }

    /*
     * The captured arrays before i and after j need not change.
     * We first compute everything caught between rules i and j
     * (inclusive) and then compute the captures array from scratch
     * rules between rule i and rule j, both * inclusive.
     */
    caught = bit_vector_init((bit_size_t) rs->n_samples);

    for (k = i; k <= j; k++)
        bit_vector_or_eq(caught, rs->rules[k].captures);
    cnt = bit_vector_n_ones(caught);
    /* Now swap the rules in the ruleset. */
    temp = rs->rules[i].rule_id;
    rs->rules[i].rule_id = rs->rules[j].rule_id;
    rs->rules[j].rule_id = (unsigned) temp;

    cnt_check = 0;
    for (k = i; k <= j; k++) {
        /*
         * Compute the items captured by rule k by anding the caught
         * vector with the truthtable of the kth rule.
         */
        // k.captures = caught & k.tt
        bit_vector_and(rs->rules[k].captures, caught, rules[rs->rules[k].rule_id].truthtable);
        cnt_check += bit_vector_n_ones(rs->rules[k].captures);

        /* Now remove the caught items from the caught vector. */
        // caught = caught & (~captures) (1, 1) => 0; (1, 0) => 1; (0, 0) => 0; (0, 1) => 0 (X)
        // since for all captures = 1, caught must be 1, we can use xor to speed up the calculation
        // That is: caught ^= k.captures
//        bit_vector_and_not_safe(caught, caught, rs->rules[k].captures);
        bit_vector_xor_eq(caught, rs->rules[k].captures);

    }
    assert(bit_vector_n_ones(caught) == 0);
    if (cnt != cnt_check)
        assert(cnt == cnt_check);

    bit_vector_free(caught);
}

void
ruleset_print(rulelist_t *rs, rule_data_t *rules, int detail) {
    int i, total_support;

    printf("\n%d rules %d samples\n", rs->n_rules, rs->n_samples);

    total_support = 0;
    for (i = 0; i < rs->n_rules; i++) {
        rule_print(rules, rs->rules[i].rule_id, detail);
//        ruleset_entry_print(rs->rules + i, detail);
        total_support += bit_vector_n_ones(rs->rules[i].captures);
    }
    printf("Total Captured: %d\n", total_support);
}

void
ruleset_entry_print(rulelist_entry_t *re, int detail) {
    printf("%d captured; \n", bit_vector_n_ones(re->captures));
    if (detail)
        bit_vector_print(re->captures);
}

void
rule_print(rule_data_t *rules, int ndx, int detail) {
    rule_data_t *r;

    r = rules + ndx;
    printf("RULE %d (%s), support=%d\n",
           ndx, r->feature_str, bit_vector_n_ones(r->truthtable));
    if (detail)
        bit_vector_print(r->truthtable);
}


//void
//rule_print_all(rule_data_t *rules, int n_rules) {
//    for (int i = 0; i < n_rules; i++)
//        rule_print(rules, i, 1);
//}
