#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include "pysbrl.h"
#include "rule.h"
#include <gsl/gsl_matrix.h>

int verbose = 0;

int train_sbrl(const char *data_file, const char *label_file,
    double lambda, double eta, int max_iters, int nchain, int * alphas, int n_alpha,
    // int verbose,
    int *ret_n_rules, int ** ret_rule_ids, 
    int *ret_n_probs, int *ret_n_classes, double ** ret_probs,
    int *ret_n_all_rules, char *** ret_all_rule_features) 
{
    data_t data;
    params_t params;
    int n_samples, n_rules, n_classes;
    rule_t *rules, *labels;
    if (verbose > 0)
        fprintf(stdout, "Info: Laod data files %s and %s\n", data_file, label_file);
    int ret = load_data(data_file, label_file, 
        &n_samples, &n_rules, &n_classes, &rules, &labels);
    if (ret != 0) {
        fprintf(stderr, "Error %d: Failed to load data files\n", ret);
        return ret;
    }
    if (verbose > 0)
        fprintf(stdout, "Info: Data files loaded.\n");
    data.rules = rules;
    data.labels = labels;
    data.nrules = n_rules;
    data.nsamples = n_samples;

    params.lambda = lambda;
    params.eta = eta;
    params.iters = max_iters;
    params.nchain = nchain;
    params.n_classes = n_classes;
    int alpha = -1;
    if (n_alpha != n_classes) {
        if (n_alpha == 1) {
            n_alpha = n_classes;
        }
        else {
            fprintf(stderr, "Error: Expect to have %d alphas, but received %d. Using the first alpha to fill all alphas...\n", n_classes, n_alpha);
        }
        params.alpha = malloc(sizeof(int) * n_alpha);
        alpha = alphas[0];
        memset(params.alpha, alpha, n_alpha * sizeof(int));
    }
    else {
        params.alpha = alphas;
    }
    if (verbose > 0)
        fprintf(stdout, "Info: Start the training...\n");
    pred_model_t * model = train(&data, 0, 0, &params);
    if (verbose > 0)
        fprintf(stdout, "Info: Training done.\n");
    if (verbose > 1)
        fprintf(stdout, "Preparing outputs\n");
    ruleset_t * rs = model->rs;
    int * rule_ids = malloc(rs->n_rules * sizeof(int));
    double * probs = malloc(rs->n_rules * n_classes * sizeof(double));
    char ** feature_lists = malloc(n_rules * sizeof(char *));

    for (int i = 0; i < rs->n_rules; i++) {
        rule_ids[i] = rs->rules[i].rule_id;
    }

    for (int i = 0; i < rs->n_rules; i++) {
        for (int j = 0; j < n_classes; j++) {
            probs[i*n_classes + j] = gsl_matrix_get(model->theta, i, j);
        }
    }

    for (int i = 0; i < n_rules; i++) {
        feature_lists[i] = rules[i].features;
    }

    *ret_n_rules = rs->n_rules;
    *ret_rule_ids = rule_ids;
    *ret_n_probs = rs->n_rules;
    *ret_n_classes = n_classes;
    *ret_probs = probs;
    *ret_n_all_rules = n_rules;
    *ret_all_rule_features = feature_lists;

    if (verbose > 1)
        fprintf(stdout, "Output prepared\n");
    if (alpha != -1)
        free(params.alpha);
    if (verbose > 1)
        fprintf(stdout, "Returning\n");
    return 0;
}