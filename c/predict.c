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
/* Use scalable bayesian rule list to make predictions. */

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
// #include <unistd.h>
#include <math.h>
#include <time.h>
#include "rule.h"
#include "bit_vector.h"
#include "utils.h"

extern int debug;

double
get_accuracy(rulelist_t *rs,
             gsl_matrix *theta, rule_data_t *test_labels, params_t *params) {
    int j, nwrong = 0;
    int *corrects = (int *) malloc(sizeof(int) * params->n_classes);
    bit_vector_t *v0 = bit_vector_init((bit_size_t) rs->n_samples);

    for (j = 0; j < rs->n_rules; j++) {
        // int n1_correct = 0, n0_correct = 0;
        gsl_vector_view theta_j = gsl_matrix_row(theta, j);
        size_t max_idx = gsl_vector_max_index(&(theta_j.vector));
        bit_vector_and(v0, rs->rules[j].captures, test_labels[max_idx].truthtable);
        corrects[max_idx] = bit_vector_n_ones(v0);
        nwrong += bit_vector_n_ones(rs->rules[j].captures) - corrects[max_idx];

        if (debug) {
            printf("rules[%d] captures %dsamples;\n", j, bit_vector_n_ones(rs->rules[j].captures));
            printf("correctness:");
            for (int i = 0; i < params->n_classes; i++) {
                printf("n%d=%d, ", i, corrects[i]);
            }
            printf(";\nTest prob= %.6f\n", arr_sum(params->n_classes, corrects) * 1.0
                                           / bit_vector_n_ones(rs->rules[j].captures));
        }
    }
    if (debug) {
        printf("ntotal = %d,  n0 = %d,  n1 = %d\n",
               rs->n_samples, bit_vector_n_ones(test_labels[0].truthtable),
               bit_vector_n_ones(test_labels[1].truthtable));
        printf("#wrong predictions = %d,  #total predictions = %d\n",
               nwrong, rs->n_samples);
    }

    free(corrects);
    return 1.0 - ((float) nwrong / rs->n_samples);
}

gsl_matrix *
predict(pred_model_t *pred_model, rule_data_t *labels, params_t *params) {
    // double *prob;
    gsl_matrix *prob;
    int i, j, rule_id, sample, n_captures;
    rulelist_t *rs;

    // prob = calloc(pred_model->rs->n_samples, sizeof(double));
    prob = gsl_matrix_calloc(pred_model->rs->n_samples, params->n_classes);
    if (prob == NULL)
        return NULL;

    // for (i = 0; i < pred_model->rs->n_samples; i++)
    // 	prob[i] = 0.0;

    rs = pred_model->rs;
    for (j = 0; j < rs->n_rules; j++) {
        sample = 0;
        rule_id = rs->rules[j].rule_id;
        n_captures = bit_vector_n_ones(rs->rules[j].captures);
        printf("Rule %d captures %d of %d samples\n",
               rule_id, n_captures, pred_model->rs->n_samples);
        /*
         * Iterate over the captured vector finding each sample
         * captured by this rule; assign it the probability associated
         * with the rule.
         */
        for (i = 0; i < n_captures; i++) {
            sample = bit_vector_first_set(rs->rules[j].captures, sample);
            gsl_vector_view prob_sample = gsl_matrix_row(prob, sample);
            gsl_vector_view theta_j = gsl_matrix_row(pred_model->theta, j);
            gsl_vector_memcpy(&(prob_sample.vector), &(theta_j.vector));
            // prob[sample] = pred_model->theta[j];
            sample++;
        }

    }

    if (debug)
        printf("test accuracy = %.6f \n",
               get_accuracy(pred_model->rs, pred_model->theta,
                            labels, params));

    return prob;
}
