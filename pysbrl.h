
void train_sbrl(const char *data_file, const char *label_file,
    double lambda, double eta, int max_iters, int nchains, int n_alpha, int * alphas,
    int verbose,
    int *ret_n_rules, int ** ret_rule_ids, 
    int *ret_n_probs, int *ret_n_classes, double ** ret_probs,
    int *ret_n_all_rules, char *** ret_all_rule_features);