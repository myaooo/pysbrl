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
 * Scalable ruleset test program.
 * 
 * This consists of two types of tests:
 * 1. Low level tests to make sure the basic rule library is working
 * 2. A run of MCMC on known data to make sure the library pieces are all working.
 *
 * Debug levels:
 * > 100 print everything imaginable
 * > 10 Trace general execution
 * > 1  Main parameters and results
 */

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "mytime.h"
#include "rule.h"

/* Convenient macros. */
#define DEFAULT_RULESET_SIZE  4
#define DEFAULT_RULE_CARDINALITY 3
#define NLABELS 2


pred_model_t *read_model(const char *, rule_data_t *, int);
void run_experiment(int, int, int, int, rule_data_t *);
gsl_matrix *test_model(const char *, const char *, pred_model_t *, params_t *);
int write_model(const char *, pred_model_t *);

int debug;

int
usage(void)
{
    (void)fprintf(stderr, "%s %s %s\n",
        "Usage: sbrlmod [-c chains] [-d debug-level] [-e eta] [-l lambda] ",
        "[-m model] [-s ruleset-size] [-i iterations] [-t test] [-S seed] [-a alpha]",
        "train.out [train.label] [test.out] [test.label]");
    return (-1);
}

#define	TEST_TEST	1
#define	TEST_MCMC	2

int
main (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind, optopt, opterr;
    int ret, size = DEFAULT_RULESET_SIZE;
    int i, iters, tnum, alpha;
    char ch, *modelfile;
    data_t train_data;
    gsl_matrix *p;
    pred_model_t *model;
    struct timeval tv_acc, tv_start, tv_end;
    params_t params = {9.0, 2.0, 100, 11, 2, NULL};

    debug = 0;
    alpha = 1;
    p = NULL;
    iters = params.iters;
    tnum = TEST_TEST;
    modelfile = NULL;
    srand(time(0) + clock());
    while ((ch = getopt(argc, argv, "c:d:e:i:l:m:s:S:t:a:")) != EOF)
        switch (ch) {
        case 'c':
            params.n_chains = atoi(optarg);
            break;
        case 'd':
            debug = atoi(optarg);
            break;
        case 'e':
            params.eta = strtod(optarg, NULL);
            break;
        case 'i':
            iters = atoi(optarg);
            params.iters = iters;
            break;
        case 'l':
            params.lambda = strtod(optarg, NULL);
            break;
        case 'm':
            modelfile = optarg;
            break;
        case 's':
            size = atoi(optarg);
            params.iters = size;
            break;
        case 'S':
            srandom((unsigned)(atoi(optarg)));
            break;
        case 't':
            tnum = atoi(optarg);
            break;
        case 'a':
            alpha = atoi(optarg);
            break;
        case '?':
        default:
            return (usage());
        }

    argc -= optind;
    argv += optind;

    if (argc < 2)
        return (usage());

    /*
     * We treat the label file as a separate ruleset, since it has
     * a similar format.
     */
    INIT_TIME(tv_acc);
    START_TIME(tv_start);
    if ((ret = load_data(argv[0], argv[1], &train_data)) != 0) {
        return (ret);
    }
    params.alpha = (int *) malloc(train_data.n_classes * sizeof(int));
    for (i = 0; i < train_data.n_classes; i++) {
        params.alpha[i] = alpha;
    }
    params.n_classes = train_data.n_classes;
    END_TIME(tv_start, tv_end, tv_acc);
    REPORT_TIME("Initialize time", "per rule", tv_end, train_data.n_rules);
    
    if (debug)
        printf("%d rules %d samples\n\n", train_data.n_rules, train_data.n_samples);
    
    if (debug > 100) {
        printf("Labels for %d samples\n\n", train_data.n_samples);
        // rule_print_all(labels, nsamples, nsamples);
       } 
    /*
     * Testing:
     * 1. Test basic rule manipulation.
     * 2. Test training.
     * 3. Train model and then run test.
     * 4. Read in previous model and test on it.
     */
    switch(tnum) {
        case 1:
            run_experiment(iters, size, train_data.n_samples, train_data.n_rules, train_data.rules);
            break;
        case 2:
        case 3:
//            train_data.rules = rules;
//            train_data.labels = labels;
//            train_data.n_rules = nrules;
//            train_data.n_samples = nsamples;
            INIT_TIME(tv_acc);
            START_TIME(tv_start);
            model = train(&train_data, &params, 0, 0);
            END_TIME(tv_start, tv_end, tv_acc);
            REPORT_TIME("Time to train", "", tv_end, 1);

            if (model == NULL) {
                fprintf(stderr, "Error: Train failed\n");
                break;
            }

            printf("\nThe best rulelist for %d MCMC chains is: ",
                params.n_chains);
            ruleset_print(model->rs, train_data.rules, 0);
            for (i = 0; i < model->rs->n_rules; i++) {
                gsl_vector_view theta_i = gsl_matrix_row(model->theta, i);
                double max_theta = gsl_vector_max(&(theta_i.vector));
                printf("rule %d, theta: %.8f\n",
                    model->rs->rules[i].rule_id,
                    max_theta);
            }

//            printf("Lambda = %.6f\n", params.lambda);
//            printf("Eta = %.6f\n", params.eta);
//            printf("Alpha[0] = %d\n", params.alpha[0]);
//            printf("Alpha[1] = %d\n", params.alpha[1]);
//            printf("Number of chains = %d\n", params.n_chains);
//            printf("Iterations = %d\n", params.iters);

            if (tnum == 3) {
                /* Now test the model */
                if (argc < 4) {
                    usage();
                    break;
                }
                p = test_model(argv[2],
                    argv[3], model, &params);
            }

            if (modelfile != NULL)
                (void)write_model(modelfile, model);

            ruleset_destroy(model->rs);
            free(model->theta);
            free(model);
            break;
        case 4:	/*
             * Test model from previous run; requires model
             * file as well as testdata.
             */
            if (modelfile == NULL || argc < 2) {
                usage();
                break;
            }
            // Read Modelfile
            model = read_model(modelfile, train_data.rules, train_data.n_samples);
            p = predict(model, train_data.labels, &params);
            break;
        default:
            usage();
            break;
    }
    free(params.alpha);

    data_free(&train_data);
    if (p != NULL)
        free(p);
    return (0);
}

/* ========= Simple test utility routines ======= */

/*
 * Given a rule set, pick a random rule (not already in the set) and
 * add it at the ndx-th position.
 */
int
add_random_rule(rule_data_t *rules, int nrules, rulelist_t *rs, int ndx)
{
    int r;
    r = pick_random_rule(nrules, rs);
    if (debug > 100)
        printf("Selected %d for new rule\n", r);

    return (ruleset_add(rules, &rs, r, ndx));
}

/*
 * Generate a random ruleset and then do some number of adds, removes,
 * swaps, etc.
 */
void
run_experiment(int iters, int size, int nsamples, int nrules, rule_data_t *rules)
{
    int i, j, k;
    rulelist_t *rs;
    struct timeval tv_acc, tv_start, tv_end;

    for (i = 0; i < iters; i++) {
        rs = create_random_ruleset(size, nsamples, nrules, rules);
        if (rs == NULL)
            return;
        if (debug) {
            printf("Initial ruleset\n");
            ruleset_print(rs, rules, 0);
        }

        /* Now perform-(size-2) squared swaps */
        INIT_TIME(tv_acc);
        START_TIME(tv_start);
        for (j = 0; j < size; j++)
            for (k = 1; k < (size-1); k++) {
                if (debug>10)
                    printf("\nSwapping rules %d and %d\n",
                        rs->rules[k-1].rule_id,
                        rs->rules[k].rule_id);
                ruleset_swap(rs, k - 1, k, rules);
                if (debug>10)
                    ruleset_print(rs, rules, 0);
            }
        END_TIME(tv_start, tv_end, tv_acc);
        REPORT_TIME("analyze", "per swap", tv_end, ((size-1) * (size-1)));

        /*
         * Now remove a rule from each position, replacing it
         * with a random rule at the end.
         */
        INIT_TIME(tv_acc);
        START_TIME(tv_start);
        for (j = 0; j < (size - 1); j++) {
            if (debug > 10)
                printf("\nDeleting rule %d\n", j);
            ruleset_delete(rules, rs, j);
            // if (debug) 
                // ruleset_print(rs, rules, (debug > 100));
            add_random_rule(rules, nrules, rs, j);
            // if (debug)
                // ruleset_print(rs, rules, (debug > 100));
        }
        END_TIME(tv_start, tv_end, tv_acc);
        REPORT_TIME("analyze", "per add/del", tv_end, ((size-1) * 2));
        ruleset_destroy(rs);
    }

}

gsl_matrix *
test_model(const char *data_file,
    const char *label_file, pred_model_t *model, params_t *params)
{
    gsl_matrix *p;
    int *idarray;
    data_t test_data;
//    rule_data_t *rules, *labels;
    rulelist_t *test_rs, *tmp_rs;

    idarray = NULL;
    test_rs = NULL;
    p = NULL;

    /* Make an array of the rules comprising this model. */
    if ((ruleset_backup(model->rs, &idarray)) != 0)
        goto err;

    /* Load test data. */
    if (load_data(data_file, label_file, &test_data) != 0)
        goto err;

    /* Create new ruleset with test data. */
    if ((test_rs = ruleset_init(model->rs->n_rules,
        test_data.n_samples, idarray, test_data.rules)) == NULL)
        goto err;

    tmp_rs = model->rs;
    model->rs = test_rs;
    p = predict(model, test_data.labels, params);
    model->rs = tmp_rs;

err:
    if (idarray != NULL)
        free (idarray);
    if (test_rs != NULL)
        ruleset_destroy(test_rs);

    return (p);
}

int
write_model(const char *file, pred_model_t *model)
{
    FILE *fi;
    int i;

    if ((fi = fopen(file, "w")) == NULL) {
        fprintf(stderr, "%s %s: %s\n",
            "Unable to write model file", file, strerror(errno));
        return (-1);
    } 
    fprintf(fi, "%d %zu\n", model->rs->n_rules, model->theta->size2);
    for (i = 0; i < model->rs->n_rules; i++) {
        fprintf(fi, "%d ", model->rs->rules[i].rule_id);
    }
    fprintf(fi, "\n");
    gsl_matrix_fwrite(fi, model->theta);
    // for (j = 0; j < model->theta->size2; j++) {
    // 	fprintf(fi, "%.8f", )
    // }
    fclose(fi);
    return (0);
}

pred_model_t *
read_model(const char *file, rule_data_t *rules, int nsamples)
{
    // double theta; 
    gsl_matrix *theta_array;
    FILE *fi;
    int *idarray, n_classes;
    pred_model_t *model;
    rulelist_t *rs;

    model = NULL;
    rs = NULL;
    // idarray = NULL;
    // theta_array = NULL;

    if ((fi = fopen(file, "r")) == NULL) {
        fprintf(stderr, "%s %s: %s\n",
            "Unable to read model file", file, strerror(errno));
        return (NULL);
    } 
    int n_rules;
    fscanf(fi, "%d %d", &n_rules, &n_classes);

    idarray = (int *) malloc(n_rules * sizeof(int));
    theta_array = gsl_matrix_alloc(n_rules, n_classes);
    for (int j = 0; j < n_rules; j++) {
        fscanf(fi, "%d", idarray+j);
    }
    gsl_matrix_fread(fi, theta_array);
    // while ((tmp = fscanf(fi, "%d,%lf\n", &id, &theta)) == 2) {
        // if (debug > 10)
        // 	printf("tmp = %d id = %d theta = %f\n", tmp, id, theta);
        // if (i >= nslots) {
        // 	nslots += 50;
        // 	idarray = realloc(idarray, nslots * sizeof(int));
        // 	theta_array =
        // 	    realloc(theta_array, nslots * sizeof(double));
        // 	if (idarray == NULL || theta_array == NULL) {
        // 		fprintf(stderr,
        // 		    "Unable to malloc space: %s\n",
        // 		        strerror(ENOMEM));
        // 		goto err;
        // 	}
        // }

        // idarray[i] = id;
        // theta_array[i++] = theta;
    // }

    /* Create the ruleset. */
    if ((rs = ruleset_init(n_rules, nsamples, idarray, rules)) == NULL)
        goto err;
    /* Create the model. */
    if ((model = (pred_model_t *)malloc(sizeof(pred_model_t))) == NULL)
        goto err;
    model->rs = rs;
    model->theta = theta_array;

    if (0) {
err:
        if (rs != NULL)
            ruleset_destroy(rs);
    }
    if (idarray != NULL)
        free(idarray);
    if (theta_array != NULL)
        free(theta_array);
    fclose(fi);
    return (model);
}
