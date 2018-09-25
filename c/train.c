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
/*
 * Scalable Bayesian Rulelist training
 */

//#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
// #include "mytime.h"
#include "rule.h"
#include "utils.h"

#ifdef DEBUG
#define DEBUG_PRINT(msg,...) printf("[%s:%i] "msg, __FILE__, __LINE__, ##__VA_ARGS__);
#define DEBUG_RUN(code) (code)
#else
#define DEBUG_PRINT(msg,...)
#define DEBUG_RUN(code)

#endif

//#define EPSILON 1e-9
#define MAX_RULE_CARDINALITY 10

static gsl_rng *RAND_GSL;

/*
 * File global variables.
 * These make the library not thread safe. If we want to be thread safe during
 * training, then we should reference count these global tables.
 */
static double *g_log_lambda_pmf, *g_log_eta_pmf;
static double *g_log_gammas;
static double log_gamma_sum;
static double eta_norm;
static int n_add, n_delete, n_swap;
static int card_count[1 + MAX_RULE_CARDINALITY];
// idx: the length of a rule, val: the count of rules having such length

/* These hold the alpha parameter values to speed up log_gamma lookup. */
static int alpha_sum;
// static int a0, a1, a01, alpha_sum;

typedef struct _permute {
    int val;
    int ndx;
} permute_t;
static permute_t *rule_permutation;
static int permute_ndx;

double compute_log_posterior(const rulelist_t *, data_t *, params_t *, int, double *);
//int gen_poission(double);
gsl_matrix *get_theta(rulelist_t *, rule_data_t *, params_t *);
//void gsl_ran_poisson_test();
int clean_static_resources(void);

#define MAX(x, y) ((x) < (y) ? (y) : (x))

/****** These are the heart of both MCMC and SA ******/
/*
 * Once we encapsulate the acceptance critera, we can use the same routine,
 * propose, to make proposals and determine acceptance. This leaves a lot
 * of the memory management nicely constrained in this routine.
 */

int
mcmc_accepts(double new_log_post, double old_log_post,
    double prefix_bound, double max_log_post, const double *extra)
{
    /* Extra = jump_prob */
    return (prefix_bound > max_log_post &&
        log(gsl_rng_uniform(RAND_GSL)) <
        (new_log_post - old_log_post + log(*extra)));
}

//int
//sa_accepts(double new_log_post, double old_log_post,
//    double prefix_bound, double max_log_post, const double *extra)
//{
//    /* Extra = tk */
//    return (prefix_bound > max_log_post &&
//        (new_log_post > old_log_post ||
//         (log(gsl_rng_uniform(RAND_GSL)) <
//         (new_log_post - old_log_post) / *extra)));
//}


/*
 * Create a proposal; used both by simulated annealing and MCMC.
 * 1. Compute proposal parameters
 * 2. Create the new proposal ruleset
 * 3. Compute the log_posterior
 * 4. Call the appropriate function to determine acceptance criteria
 */
rulelist_t *
propose(rulelist_t *rs, data_t* train_data,
    double *jump_prob, double *ret_log_post, double max_log_post,
    int *cnt, const double *extra, params_t *params,
    int (*accept_func)(double, double, double, double, const double *))
{
    char stepchar;
    double new_log_post, prefix_bound;
    int change_ndx, ndx1, ndx2;
    rulelist_t *rs_new, *rs_ret;
    rs_new = NULL;

    if (ruleset_copy(&rs_new, rs) != 0)
        goto err;

    if (ruleset_proposal(rs_new, train_data->n_rules,
                         &ndx1, &ndx2, &stepchar, jump_prob) != 0)
            goto err;


//    DEBUG_PRINT("Given ruleset: \n");
//    DEBUG_RUN(ruleset_print(rs, train_data->rules, 0));
//    DEBUG_PRINT("Operation %c(%d)(%d) produced proposal:\n", stepchar, ndx1, ndx2);
    switch (stepchar) {
    case 'A':
        /* Add the rule whose id is ndx1 at position ndx2 */
        if (ruleset_add(train_data->rules, &rs_new, ndx1, ndx2) != 0)
            goto err;
        change_ndx = ndx2 + 1;
        n_add++;
        break;
    case 'D':
        /* Delete the rule at position ndx1. */
        change_ndx = ndx1;
        ruleset_delete(train_data->rules, rs_new, ndx1);
        n_delete++;
        break;
    case 'S':
        /* Swap the rules at ndx1 and ndx2. */
        ruleset_swap_any(rs_new, ndx1, ndx2, train_data->rules);
        change_ndx = 1 + (ndx1 > ndx2 ? ndx1 : ndx2);
        n_swap++;
        break;
    default:
        goto err;
    }

    new_log_post = compute_log_posterior(rs_new,
        train_data, params, change_ndx, &prefix_bound);

//    DEBUG_RUN(ruleset_print(rs_new, train_data->rules, 0));
//    DEBUG_PRINT("With new log_posterior = %0.6f\n", new_log_post);
    if (prefix_bound < max_log_post)
        (*cnt)++;

    if (accept_func(new_log_post, *ret_log_post, prefix_bound, max_log_post, extra)) {
//        DEBUG_PRINT("Accepted\n");
        rs_ret = rs_new;
        *ret_log_post = new_log_post;
        ruleset_destroy(rs);
    } else {
//        DEBUG_PRINT("Rejected\n");
        rs_ret = rs;
        ruleset_destroy(rs_new);
    }

    return (rs_ret);
err:
    if (rs_new != NULL)
        ruleset_destroy(rs_new);
    ruleset_destroy(rs);
    return (NULL);
}

/********** End of proposal routines *******/
// TODO

int
compute_log_gammas(int nsamples, params_t *params)
{
    int i, max;

    /* Pre-compute alpha sum for accessing the log_gammas. */
    // a0 = params->alpha[0];
    // a1 = params->alpha[1];
    // a01 = a0 + a1;
//    g_alpha = params->alpha;
    alpha_sum = arr_sum(params->n_classes, params->alpha);

    max = nsamples + 2 * (1 + alpha_sum);
    g_log_gammas = malloc(max * sizeof(double));
    if (g_log_gammas == NULL)
        return (-1);

    g_log_gammas[0] = -1e-20;
    for (i = 1; i < max; i++)
        g_log_gammas[i] = gsl_sf_lngamma((double)i);
    log_gamma_sum = 0.;
    for (i = 0; i < params->n_classes; i++)
        log_gamma_sum += g_log_gammas[params->alpha[i]];
    
    // printf("max of n: %d\n", max);
    return (0);
}

int
compute_pmf(int nrules, params_t *params)
{
    int i;
//    int pmf_size = nrules;
    if (((int) params->lambda) > nrules) {
        fprintf(stderr, "Error: lambda is %.1f, larger than nrules: %d. "
                        "Setting lambda to %d to avoid numerical issues\n",
                params->lambda, nrules, nrules - 1);
        params->lambda = (double) nrules - 1;
//        pmf_size = nrules+1;
    }
    if ((g_log_lambda_pmf = malloc(nrules * sizeof(double))) == NULL)
        return (errno);
    for (i = 0; i < nrules; i++) {
        g_log_lambda_pmf[i] = log(gsl_ran_poisson_pdf((unsigned) i, params->lambda));
//        printf("g_log_lambda_pmf[ %d ] = %6f\n", i, g_log_lambda_pmf[i]);
    }

    if ((g_log_eta_pmf = malloc((1 + MAX_RULE_CARDINALITY) * sizeof(double))) == NULL)
        return (errno);
    for (i = 0; i <= MAX_RULE_CARDINALITY; i++) {
        g_log_eta_pmf[i] = log(gsl_ran_poisson_pdf((unsigned) i, params->eta));
//        printf("g_log_eta_pmf[ %d ] = %6f\n", i, g_log_eta_pmf[i]);
    }

    /*
     * For simplicity, assume that all the cardinalities
     * <= MAX_RULE_CARDINALITY appear in the mined rules
     * not including 0-length cardinality
     */
    eta_norm = gsl_cdf_poisson_P(MAX_RULE_CARDINALITY, params->eta)
        - gsl_ran_poisson_pdf(0, params->eta);

    DEBUG_PRINT("eta_norm(Beta_Z) = %6f\n", eta_norm);

    return (0);
}

/*
 * Count the cardinality of rules
 */

void
count_cardinality(int n_rules, rule_data_t * rules)
{
    int i;
    memset(card_count, 0, MAX_RULE_CARDINALITY * sizeof(int));

    for (i = 0; i < n_rules; i++) {
        card_count[rules[i].cardinality]++;
    }

}

int
permute_cmp(const void *v1, const void *v2)
{
    return ((permute_t *)v1)->val - ((permute_t *)v2)->val;
}

int
permute_rules(int nrules)
{
    int i;
    if ((rule_permutation = malloc(sizeof(permute_t) * nrules)) == NULL)
        return (-1);
    for (i = 0; i < nrules; i++) {
        rule_permutation[i].val = (int) gsl_rng_get(RAND_GSL);
        rule_permutation[i].ndx = i;
    }
    qsort(rule_permutation, (unsigned) nrules, sizeof(permute_t), permute_cmp);
    permute_ndx = 1;
#ifdef DEBUG
    printf("DEBUG permute_rules\n");
    for (int i = 0; i < nrules; i++) {
        printf("%d: (%d, %d) ", i, rule_permutation[i].val, rule_permutation[i].ndx);;
    }
    printf("\n");
#endif
    return (0);

}

int clean_static_resources(void) {
    /* Free allocated memory. */
    if (g_log_lambda_pmf != NULL)
        free(g_log_lambda_pmf);
    g_log_lambda_pmf = NULL;
    if (g_log_eta_pmf != NULL)
        free(g_log_eta_pmf);
    g_log_eta_pmf = NULL;
    if (rule_permutation != NULL)
        free(rule_permutation);
    rule_permutation = NULL;
    if (g_log_gammas != NULL)
        free(g_log_gammas);
    g_log_gammas = NULL;
    if (RAND_GSL != NULL)
        gsl_rng_free(RAND_GSL);
    RAND_GSL = NULL;
    return 1;
}

pred_model_t *
train(data_t *train_data, params_t *params, long seed, int verbose)
{
    int chain, default_rule;
    pred_model_t *pred_model;
    rulelist_t *rs, *rs_temp;
    double max_pos, pos_temp, null_bound;

    /* initialize random number generator for some distributions. */
    if (verbose > 2)
        fprintf(stdout, "Info: initializing gsl generator");
    init_gsl_rand_gen(seed);

    pred_model = NULL;
    rs = NULL;
    VERBOSE3_PRINTF(stdout, "computing pmf");
    if (compute_pmf(train_data->n_rules, params) != 0)
        goto err;
    VERBOSE3_PRINTF(stdout, "counting cardinalities");
    count_cardinality(train_data->n_rules, train_data->rules);
    VERBOSE3_PRINTF(stdout, "computing log gammas");
    if (compute_log_gammas(train_data->n_samples, params) != 0)
        goto err;

    if ((pred_model = calloc(1, sizeof(pred_model_t))) == NULL)
        goto err;

    default_rule = 0;
    if ((rs = ruleset_init(1,
        train_data->n_samples, &default_rule, train_data->rules)) == NULL)
            goto err;

    max_pos = compute_log_posterior(rs, train_data, params, -1, &null_bound);
    if (permute_rules(train_data->n_rules) != 0)
        goto err;
    VERBOSE0_PRINTF(stdout, "Start running MCMCs, n_chain=%d\n", params->n_chains);
    for (chain = 0; chain < params->n_chains; chain++) {
        if (verbose) {
            fprintf(stdout, ".");
            fflush(stdout);
        }
        rs_temp = run_mcmc(train_data, params, max_pos);
        if (rs_temp == NULL) {
            // mcmc return null rule set
            continue;
        }
        pos_temp = compute_log_posterior(rs_temp, train_data, params, -1, &null_bound);
        if (pos_temp >= max_pos) {
            ruleset_destroy(rs);
            rs = rs_temp;
            max_pos = pos_temp;
        } else {
            ruleset_destroy(rs_temp);
        }
    }
    if (verbose)
        fprintf(stdout, "\n");

    // after nchain runs, no available rule set is founded.
    if (rs == NULL)
        goto err;

    pred_model->theta = get_theta(rs, train_data->labels, params);
    pred_model->rs = rs;
    rs = NULL;

    /*
     * THIS IS INTENTIONAL -- makes error handling localized.
     * If we branch to err, then we want to free an allocated model;
     * if we fall through naturally, then we don't.
     */
    if (0) {
err:
        if (pred_model != NULL)
            free (pred_model);
    }
    /* Free allocated memory. */
    clean_static_resources();
    if (rs != NULL)
        ruleset_destroy(rs);

    return (pred_model);
}

gsl_matrix *
get_theta(rulelist_t * rs, rule_data_t * labels, params_t *params)
{
    /* calculate captured 0's and 1's */
    int i, j, max_idx;
    int theta_dominator;

    bit_vector_t * v0 = bit_vector_init((bit_size_t) rs->n_samples);
    // theta = malloc(rs->n_rules * sizeof(double));
    gsl_matrix * theta = gsl_matrix_alloc((unsigned)(rs->n_rules), (unsigned)(params->n_classes));
    // printf("theta allocated\n");
    int * ns = malloc(params->n_classes * sizeof(int));
    if (theta == NULL)
        return (NULL);

    int total_correct = 0;
    for (j = 0; j < rs->n_rules; j++) {
        for (i = 0; i < params->n_classes; i++) {
            bit_vector_and(v0, rs->rules[j].captures, labels[i].truthtable);
            ns[i] = bit_vector_n_ones(v0);
        }
        theta_dominator = arr_sum(params->n_classes, ns) + alpha_sum;
        // n1 = rs->rules[j].ncaptured - n0;
        // TODO
        for (i = 0; i < params->n_classes; i++) {
            gsl_matrix_set(theta, (unsigned) j, (unsigned) i,
                (ns[i] + params->alpha[i] * 1.0) / theta_dominator);
        }
        gsl_vector_view theta_j = gsl_matrix_row(theta, (unsigned) j);
        max_idx = (int) gsl_vector_max_index(&(theta_j.vector));
        total_correct += ns[max_idx];
#ifdef DEBUG
        for (i = 0; i < params->n_classes; i++) {
            printf("n%d=%d, ", i, ns[i]);
        }
        printf("\ncaptured=%d, training accuracy = %.6f\n", bit_vector_n_ones(rs->rules[j].captures),
            ns[max_idx] * 1.0 / bit_vector_n_ones(rs->rules[j].captures));
        printf("theta[%d][%d] = %.6f\n", j, max_idx, gsl_matrix_get(theta, j, max_idx));
#endif
    }
    DEBUG_PRINT("Overall accuracy: %.6f\n", total_correct * 1.0 / rs->n_samples);
    free(ns);
    bit_vector_free(v0);
    return (theta);
}

rulelist_t *
run_mcmc(data_t * train_data, params_t *params, double v_star)
{
    rulelist_t *rs;
    double jump_prob, log_post_rs;
    int *rs_idarray, len, nsuccessful_rej;
    int i, rarray[2], count;
    double max_log_posterior, prefix_bound;

    rs = NULL;
    rs_idarray = NULL;
    log_post_rs = 0.0;
    nsuccessful_rej = 0;
    prefix_bound = -1e10; // This really should be -MAX_DBL
    n_add = n_delete = n_swap = 0;

    /* Initialize the ruleset. */
    DEBUG_PRINT("Prefix bound = %10f v_star = %f\n", prefix_bound, v_star);
    /*
     * Construct rulesets with exactly 2 rules -- one drawn from
     * the permutation and the default rule.
     */
    rarray[0] = 1;
    rarray[1] = 0;
    count = 0;
    while (prefix_bound < v_star) {
        // TODO Gather some stats on how much we loop in here.
        if (rs != NULL) {
            ruleset_destroy(rs);
            count++;
            if (count == (train_data->n_rules - 1)) {
                fprintf(stderr, "No ruleset with enough bound after %d runs\n", count);
                return (NULL);
            }
        }
        rarray[0] = rule_permutation[permute_ndx++].ndx;
        if (permute_ndx >= train_data->n_rules)
            permute_ndx = 1;
        DEBUG_PRINT("MCMC permutation before init %d, permute_ndx: %d, rarray[0]: %d.\n", count, permute_ndx, rarray[0]);
        rs = ruleset_init(2, train_data->n_samples, rarray, train_data->rules);
        DEBUG_PRINT("MCMC permutation after init %d.\n", count);
        if (rs == NULL) continue;
        log_post_rs = compute_log_posterior(rs, train_data, params, 1, &prefix_bound);
        DEBUG_PRINT("MCMC permutation after compute log posterior %d.\n", count);
    }
    DEBUG_PRINT("MCMC permutation loop complete.\n");

    /*
     * The initial ruleset is our best ruleset so far, so keep a
     * list of the rules it contains.
     */
    if (ruleset_backup(rs, &rs_idarray) != 0) {
        DEBUG_PRINT("MCMC errored backup.\n");
        goto err;
    }
    max_log_posterior = log_post_rs;
    len = rs->n_rules;

    for (i = 0; i < params->iters; i++) {
        if ((rs = propose(rs, train_data, &jump_prob,
            &log_post_rs, max_log_posterior, &nsuccessful_rej,
            &jump_prob, params, mcmc_accepts)) == NULL) {
            DEBUG_PRINT("MCMC errored 1.\n");
            goto err;

        }

        if (log_post_rs > max_log_posterior) {
            if (ruleset_backup(rs, &rs_idarray) != 0) {
                DEBUG_PRINT("MCMC errored 2.\n");
                goto err;
            }
            max_log_posterior = log_post_rs;
            len = rs->n_rules;
        }
    }
    DEBUG_PRINT("MCMC max iter arrived.\n");

    /* Regenerate the best rule list */
    ruleset_destroy(rs);
    rs = ruleset_init(len, train_data->n_samples, rs_idarray, train_data->rules);
    free(rs_idarray);

#ifdef DEBUG
    printf("%s%d #add=%d #delete=%d #swap=%d):\n",
        "The best rule list is (#reject=", nsuccessful_rej,
        n_add, n_delete, n_swap);

    printf("max_log_posterior = %6f\n", max_log_posterior);
    printf("max_log_posterior = %6f\n",
        compute_log_posterior(rs, train_data, params, -1, &prefix_bound));
//    ruleset_print(rs, train_data->rules, 0);
#endif
    return (rs);

err:
    if (rs != NULL)
        ruleset_destroy(rs);
    if (rs_idarray != NULL)
        free(rs_idarray);
    return (NULL);
}

//rulelist_t *
//run_simulated_annealing(data_t *train_data, params_t *params, int init_size)
//{
//    rulelist_t *rs;
//    double jump_prob;
//    int dummy, i, j, k, iter, iters_per_step, *rs_idarray = NULL, len;
//    double log_post_rs, max_log_posterior, prefix_bound = 0.0;
//
//    iters_per_step = 200;
//
//    /* Initialize the ruleset. */
//    rs = create_random_ruleset(init_size, train_data->n_samples, train_data->n_rules, train_data->rules);
//    if (rs == NULL)
//        return (NULL);
//
//    log_post_rs = compute_log_posterior(rs, train_data, params, -1, &prefix_bound);
//    if (ruleset_backup(rs, &rs_idarray) != 0)
//        goto err;
//    max_log_posterior = log_post_rs;
//    len = rs->n_rules;
//
////    printf("Initial ruleset: \n");
////    ruleset_print(rs, train_data->rules, 0);
//
//    /* Pre-compute the cooling schedule. */
//    double T[100000], tmp[50];
//    int ntimepoints = 0;
//
//    tmp[0] = 1;
//    for (i = 1; i < 28; i++) {
//        tmp[i] = tmp[i - 1] + exp(0.25 * (i + 1));
//        for (j = (int)tmp[i - 1]; j < (int)tmp[i]; j++)
//            T[ntimepoints++] = 1.0 / (i + 1);
//    }
//
//    DEBUG_PRINT("iters_per_step = %d, #timepoints = %d\n", iters_per_step, ntimepoints);
//
//    for (k = 0; k < ntimepoints; k++) {
//        double tk = T[k];
//        for (iter = 0; iter < iters_per_step; iter++) {
//                if ((rs = propose(rs, train_data, &jump_prob,
//                &log_post_rs, max_log_posterior, &dummy, &tk,
//                params, sa_accepts)) == NULL)
//                    goto err;
//
//            if (log_post_rs > max_log_posterior) {
//                if (ruleset_backup(rs, &rs_idarray) != 0)
//                    goto err;
//                max_log_posterior = log_post_rs;
//                len = rs->n_rules;
//            }
//        }
//    }
//    /* Regenerate the best rule list. */
//    ruleset_destroy(rs);
//    rs = ruleset_init(len, train_data->n_samples, rs_idarray, train_data->rules);
//    free(rs_idarray);
//    DEBUG_PRINT("\n\n/*----The best rule list is: */\n");
//    DEBUG_PRINT("max_log_posterior = %6f\n\n", max_log_posterior);
//    DEBUG_PRINT("max_log_posterior = %6f\n\n",
//        compute_log_posterior(rs, train_data, params, -1, &prefix_bound));
//    DEBUG_RUN(ruleset_print(rs, train_data->rules, 0));
//
//    return (rs);
//err:
//    if (rs != NULL)
//        ruleset_destroy(rs);
//    if (rs_idarray != NULL)
//        free(rs_idarray);
//    return (NULL);
//}

double
compute_log_posterior(const rulelist_t *rs, data_t * train_data,
    params_t *params, int length4bound, double *prefix_bound)
{

    double log_prior;
    double prefix_prior = 0.0;
    double norm_constant;
    double log_likelihood = 0.0;
    double prefix_log_likelihood = 0.0;
    int i, j, li;
    int local_cards[1 + MAX_RULE_CARDINALITY];

    for (i = 0; i < (1 + MAX_RULE_CARDINALITY); i++)
        local_cards[i] = card_count[i];

    /* Calculate log_prior and prefix_prior. */
    norm_constant = eta_norm;
    log_prior = g_log_lambda_pmf[rs->n_rules - 1];
    int lambda = (int) params->lambda;
    int max_p_lambda = MAX(rs->n_rules - 1, lambda);
    prefix_prior += g_log_lambda_pmf[max_p_lambda];
    // Don't compute the last (default) rule.
    for (i = 0; i < rs->n_rules - 1; i++) {
        li = train_data->rules[rs->rules[i].rule_id].cardinality;
        log_prior += g_log_eta_pmf[li] - log(norm_constant) - log(local_cards[li]+1e-4);
        assert(local_cards[li] > 0);
        // added for prefix_bound
        if (i < length4bound) {
            prefix_prior += g_log_eta_pmf[li] -
                log(norm_constant) - log(local_cards[li]+1e-4);
        }

        local_cards[li]--;
        if (local_cards[li] == 0)
            norm_constant -= exp(g_log_eta_pmf[li]);
    }

    /* Calculate log_likelihood */
    int * supports = malloc(params->n_classes * sizeof(int));
    int * ns = malloc(params->n_classes * sizeof(int));
    for (i = 0; i < params->n_classes; i++) {
        supports[i] = bit_vector_n_ones(train_data->labels[i].truthtable);
    }
    // int left0 = labels[0].support, left1 = labels[1].support;

    bit_vector_t *v0 = bit_vector_init((bit_size_t) rs->n_samples);
    for (j = 0; j < rs->n_rules; j++) {
        // int n0, n1;	 // Count of 0's; count of 1's
        for (i = 0; i < params->n_classes; i++) {
            bit_vector_and(v0, rs->rules[j].captures, train_data->labels[i].truthtable);
            ns[i] = bit_vector_n_ones(v0);
            supports[i] -= ns[i];
        }
        double _log_likelihood = 0;
        // printf("log_gammas:");
        for (i = 0; i < params->n_classes; i++) {
            // printf(" %d %.6f;", ns[i], log_gammas[ns[i]+alpha[i]]);
            _log_likelihood += g_log_gammas[ns[i]+params->alpha[i]];
        }
        // printf("\n");
        _log_likelihood -= g_log_gammas[arr_sum(params->n_classes, ns) + alpha_sum];
        log_likelihood += _log_likelihood;
        // Added for prefix_bound.
        if (j < length4bound) {
            prefix_log_likelihood += _log_likelihood;
            if (j == (length4bound - 1)) {
                for (i = 0; i < params->n_classes; i++) {
                    prefix_log_likelihood += 
                        log_gamma_sum - g_log_gammas[params->alpha[i]] +
                        g_log_gammas[supports[i] + params->alpha[i]]
                        - g_log_gammas[supports[i] + alpha_sum];
                }
            }
        }
    }

    *prefix_bound = prefix_prior + prefix_log_likelihood;
#ifdef DEBUG
    if (*prefix_bound < -1000. || *prefix_bound > 0.1) {
        printf("abnormal prefix_bound %.6f\n", *prefix_bound);
        printf("prior: %.6f; likelihood: %.6f\n", prefix_prior, prefix_log_likelihood);
        printf("norm_constant: %.6f, eta_norm: %.6f\n", norm_constant, eta_norm);
        printf("max_p_lambda: %d, length4bound: %d\n", max_p_lambda, length4bound);
        printf("local_cards: [");
        for (i = 0; i < MAX_RULE_CARDINALITY; i++) {
            printf("%d,", local_cards[i]);
        }
        printf("]\n");
    }
#endif
//    DEBUG_PRINT("log_prior = %6f\t log_likelihood = %6f\n", log_prior, log_likelihood);
    free(supports);
    free(ns);
    bit_vector_free(v0);
    return (log_prior + log_likelihood);
}

/* TODO: The proposal probability distributions could be improved
 * Now the three steps are chosed randomly.
 */

int
ruleset_proposal(rulelist_t * rs, int nrules,
    int *ndx1, int *ndx2, char *stepchar, double *jumpRatio){
    static double MOVEPROBS[15] = {
        0.0, 1.0, 0.0,
        0.0, 0.5, 0.5,
        0.5, 0.0, 0.5,
        1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0,
        1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0
    };
    static double JUMPRATIOS[15] = {
        0.0, 0.5, 0.0,
        0.0, 2.0 / 3.0, 2.0,
        1.0, 0.0, 2.0 / 3.0,
        1.0, 1.5, 1.0,
        1.0, 1.0, 1.0
    };

    double moveProbs[3], jumpRatios[3];
    int offset = 0;
    if (rs->n_rules <= 2) {
        offset = 0;
    } else if (rs->n_rules == 3) {
        offset = 3;
    } else if (rs->n_rules == nrules - 1) {
        offset = 6;
    } else if (rs->n_rules == nrules - 2) {
        offset = 9;
    } else {
        offset = 12;
    }
    memcpy(moveProbs, MOVEPROBS + offset, 3 * sizeof(double));
    memcpy(jumpRatios, JUMPRATIOS + offset, 3 * sizeof(double));

//    double u = ((double)rand()) / (RAND_MAX);
    double u = gsl_rng_uniform(RAND_GSL);
    int index1, index2;

    if (u < moveProbs[0]) {
        // Swap rules: cannot swap with the default rule
        index1 = (int) gsl_rng_uniform_int(RAND_GSL, (unsigned) rs->n_rules - 1);
//        index1 = rand() % (rs->n_rules - 1);

        // Make sure we do not swap with ourselves
        do {
            index2 = (int) gsl_rng_uniform_int(RAND_GSL, (unsigned) rs->n_rules - 1);
//            index2 = rand() % (rs->n_rules - 1);
        } while (index2 == index1);

        *jumpRatio = jumpRatios[0];
        *stepchar = 'S';
    } else if (u < moveProbs[0] + moveProbs[1]) {
        /* Add a new rule */
        index1 = pick_random_rule(nrules, rs);
        index2 = (int) gsl_rng_uniform_int(RAND_GSL, (unsigned) rs->n_rules);
//        index2 = rand() % rs->n_rules;
        *jumpRatio = jumpRatios[1] * (nrules - 1 - rs->n_rules);
        *stepchar = 'A';
    } else if (u < moveProbs[0] + moveProbs[1] + moveProbs[2]) {
        /* delete an existing rule */
        index1 = (int) gsl_rng_uniform_int(RAND_GSL, (unsigned) rs->n_rules - 1);
//        index1 = rand() % (rs->n_rules - 1);
        // cannot delete the default rule
        index2 = 0;
        // index2 doesn 't matter in this case
        * jumpRatio = jumpRatios[2] / (nrules - rs->n_rules);
        *stepchar = 'D';
    } else {
        //should raise exception here.
        return -1;
    }
    *ndx1 = index1;
    *ndx2 = index2;
    return (0);
}

/* seed: 0 to use default seed, -1 to use time() as the seed*/
void
init_gsl_rand_gen(long seed)
{
    if (seed < 0) {
        seed = time(NULL);
    }
//    gsl_rng_env_setup();
//    if (RAND_GSL != NULL)
//        gsl_rng_free(RAND_GSL);
    RAND_GSL = gsl_rng_alloc(gsl_rng_default);
    gsl_rng_set(RAND_GSL, (unsigned long) seed);

}


#define MAX_TRIES 10

/*
 * Given a rule set, pick a random rule (not already in the set).
 * TODO: use set to facilitate faster pick
 */
int
pick_random_rule(int n_rules, const rulelist_t *rs) {
    int cnt, j, new_rule = 1;

    cnt = 0;
    pickrule:
    if (cnt < MAX_TRIES)
        new_rule = (int) gsl_rng_uniform_int(RAND_GSL, (unsigned) (n_rules -2)) + 1;
//        new_rule = RANDOM_RANGE(1, (n_rules - 1));
    else
        new_rule = 1 + (new_rule % (n_rules - 2));

    for (j = 0; j < rs->n_rules; j++) {
        if ((int) rs->rules[j].rule_id == new_rule) {
            cnt++;
            goto pickrule;
        }
    }
    return (new_rule);
}


//int
//gen_poisson(double mu)
//{
//    return ((int)gsl_ran_poisson(RAND_GSL, mu));
//}
//
//double
//gen_poission_pdf(int k, double mu)
//{
//    return (gsl_ran_poisson_pdf(k, mu));
//}
//
//double
//gen_gamma_pdf (double x, double a, double b)
//{
//    return (gsl_ran_gamma_pdf(x, a, b));
//}
//
//void
//gsl_ran_poisson_test()
//{
//    int i, j;
//    unsigned int k1 = gsl_ran_poisson(RAND_GSL, 5);
//    unsigned int k2 = gsl_ran_poisson(RAND_GSL, 5);
//
//    printf("k1 = %u , k2 = %u\n", k1, k2);
//
//    //number of experiments
//    const int nrolls = 10000;
//
//    //maximum number of stars to distribute
//    const int nstars = 100;
//
//    int p[10] = {};
//    for (i = 0; i < nrolls; ++i) {
//        unsigned int number = gsl_ran_poisson(RAND_GSL, 4.1);
//
//        if (number < 10)
//            ++p[number];
//    }
//
//    printf("poisson_distribution (mean=4.1):\n");
//
//    for (i = 0; i < 10; ++i) {
//        printf("%d, : ", i);
//        for (j = 0; j < p[i] * nstars / nrolls; j++)
//            printf("*");
//        printf("\n");
//    }
//}
