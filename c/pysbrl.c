#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include "pysbrl.h"
#include "rule.h"
#include <gsl/gsl_matrix.h>

//int verbose = 0;

int train_sbrl(const char *data_file, const char *label_file,
    double lambda, double eta, int max_iters, int nchain, int * alphas, int n_alpha,
    int verbose,
    int *ret_n_rules, int ** ret_rule_ids, 
    int *ret_n_probs, int *ret_n_classes, double ** ret_probs,
    int *ret_n_all_rules, char *** ret_all_rule_features) 
{
    data_t data;
    params_t params;
    int n_classes;
    if (verbose > 0)
        fprintf(stdout, "Info: Load data files %s and %s\n", data_file, label_file);
    int ret = load_data(data_file, label_file, &data);
    if (ret != 0) {
        fprintf(stderr, "Error %d: Failed to load data files\n", ret);
        return ret;
    }
    if (verbose > 0)
        fprintf(stdout, "Info: Data files loaded.\n");
    n_classes = data.n_classes;
    params.lambda = lambda;
    params.eta = eta;
    params.iters = max_iters;
    params.n_chains = nchain;
    params.n_classes = n_classes;
    params.alpha = malloc(sizeof(int) * n_classes);
    if (n_alpha != n_classes) {
        if (n_alpha != 1) {
            fprintf(stderr, "Error: Expect to have %d alphas, but received %d. Using the first alpha to fill all alphas...\n", n_classes, n_alpha);
        }
        for (int i = 0; i < n_classes; i++)
            params.alpha[i] = alphas[0];
    } else {
        for (int i = 0; i < n_classes; i++)
            params.alpha[i] = alphas[i];
    }
    if (verbose > 0) {
        fprintf(stdout, "Info: Alphas: ");
        for (int i = 0; i < n_classes; i++) {
            fprintf(stdout, "%d/%d ", params.alpha[i], alphas[i]);
        }
        fprintf(stdout, "\nInfo: Start the training...\n");
    }
    pred_model_t * model = train(&data, &params, 0);
    if (verbose > 0)
        fprintf(stdout, "Info: Training done.\n");
    if (verbose > 1)
        fprintf(stdout, "Preparing outputs\n");
    rulelist_t * rs = model->rs;
    int * rule_ids = malloc(rs->n_rules * sizeof(int));
    double * probs = malloc(rs->n_rules * n_classes * sizeof(double));
    char ** feature_lists = malloc(data.n_rules * sizeof(char *));

    for (int i = 0; i < rs->n_rules; i++) {
        rule_ids[i] = rs->rules[i].rule_id;
    }

    for (int i = 0; i < rs->n_rules; i++) {
        for (int j = 0; j < n_classes; j++) {
            probs[i*n_classes + j] = gsl_matrix_get(model->theta, i, j);
        }
    }

    for (int i = 0; i < rs->n_rules; i++) {
        feature_lists[i] = data.rules[i].feature_str;
    }

    *ret_n_rules = rs->n_rules;
    *ret_rule_ids = rule_ids;
    *ret_n_probs = rs->n_rules;
    *ret_n_classes = n_classes;
    *ret_probs = probs;
    *ret_n_all_rules = data.n_rules;
    *ret_all_rule_features = feature_lists;

    free(params.alpha);

    if (verbose > 1)
        fprintf(stdout, "Output prepared. Finished\n");

    return 0;
}

int sbrl_predict() {

    return 0;
}