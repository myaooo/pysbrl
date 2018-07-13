//
// Created by Ming Yao on 6/29/18.
//

#include <assert.h>
#include <string.h>

#include "rule.h"
#include "utils.h"


int
load_data(const char *data_file, const char *label_file, data_t *data) {
    int ret, samples_chk, n_samples, n_rules, n_classes;

    rule_data_t *rules, *labels;
    /* Load data. */
    if ((ret = rules_init_from_file(data_file, &n_rules, &n_samples, &rules, 1)) != 0)
        return (ret);

    /* Load labels. */
    if ((ret = rules_init_from_file(label_file, &n_classes, &samples_chk, &labels, 0)) != 0) {
        free(rules);
        return (ret);
    }
    assert(samples_chk == n_samples);
    data->rules = rules;
    data->labels = labels;
    data->n_samples = n_samples;
    data->n_rules = n_rules;
    data->n_classes = n_classes;
    return (0);
}

//int
//rules_init_from_data(int n_items, int n_samples, char **satisfied,
//                     rule_data_t **rules_ret, int add_default_rule) {
//    int n_rules;
//    int i, j, ret, rule_idx;
//    rule_data_t *rules = NULL;
//
//    add_default_rule = add_default_rule ? 1 : 0;
//    n_rules = n_items + add_default_rule;
//    rules = (rule_data_t *) malloc(n_rules * sizeof(rule_data_t));
//
//    if (add_default_rule) {
//        rules[0].truthtable = bit_vector_init((bit_size_t) n_samples);
//        if (rules[0].truthtable == NULL)
//            goto err;
//        bit_vector_flip_all(rules[0].truthtable);
//    }
//
//    /*
//     * Leave a space for the 0th (default) rule, which we'll add at
//     * the end.
//     */
//    for (j = 0; j < n_samples; ++j) {
//        rule_idx = j + add_default_rule;
//
//        if ((rules[rule_idx].truthtable = bit_vector_from_bytes(satisfied[j], (bit_size_t) n_samples)) == NULL)
//            goto err;
//    }
//
//    *rules_ret = rules;
//
//    return (0);
//
//    err:
//    ret = errno;
//
//    /* Reclaim space. */
//    for (i = 1; i < n_items; i++) {
//        bit_vector_free(rules[i].truthtable);
//    }
//    free(rules);
//    return (ret);
//}


int
rules_init_from_stream(FILE *fi, int *ret_n_rules, int *ret_n_samples,
                       rule_data_t **rules_ret, int add_default_rule) {
    char *cp, *features, *line, *rulestr, *tmp;
    int rule_cnt, n_rules, n_samples;
    int i, ret;
    rule_data_t *rules = NULL;
    size_t linelen, rulelen;
    long len;

    /*
     * Leave a space for the 0th (default) rule, which we'll add at
     * the end.
     */
    rule_cnt = add_default_rule != 0 ? 1 : 0;
    line = NULL;
    linelen = 0;

    // Read the first two lines
    if (_getline(&line, &linelen, fi) > 0 && strncmp(line, "n_items:", 8) == 0) {
        n_rules = (int) strtol(line + 8, &tmp, 10);
    } else {
        fprintf(stderr, "Error: data file mal-format! The first line should be n_items: xxx\n");
        return -1;
    }

    if (_getline(&line, &linelen, fi) > 0 && strncmp(line, "n_samples:", 10) == 0) {
        n_samples = (int) strtol(line + 10, &tmp, 10);
    } else {
        fprintf(stderr, "Error: data file mal-format! The second line should be n_samples: xxx\n");
        return -1;
    }

    rules = (rule_data_t *) malloc(n_rules * sizeof(rule_data_t));

    for (i = 0; i < n_rules; i++) {
        if ((len = _getline(&line, &linelen, fi)) > 0) {

            /* Get the rule string; line will contain the bits. */
            features = line;
            if ((rulestr = _strsep(&features, " ")) == NULL)
                goto err;
            if ((rules[rule_cnt].feature_str = _strdup(rulestr)) == NULL)
                goto err;
            rulelen = strlen(rulestr) + 1;
            len -= rulelen;

            /*
             * At this point features is (probably) a line terminated by a
             * newline at features[len-1]; if it is newline-terminated, then
             * let's make it NUL-terminated and shorten the line length
             * by one.
             */
            if (features[len - 1] == '\n') {
                features[len - 1] = '\0';
                len--;
            }
            if ((rules[rule_cnt].truthtable = bit_vector_from_str(features, (int) len)) == NULL)
                goto err;

            /* Now compute the number of clauses in the rule. */
            rules[rule_cnt].cardinality = 1;
            for (cp = rulestr; *cp != '\0'; cp++)
                if (*cp == ',')
                    rules[rule_cnt].cardinality++;
            rule_cnt++;
        } else {

        }

    }

    /* Now create the 0'th (default) rule. */
    if (add_default_rule) {
        rules[0].cardinality = 0;
        if ((rules[0].truthtable = bit_vector_init((unsigned) n_samples)) == NULL)
            goto err;
        rules[0].feature_str = _strdup("default");
        bit_vector_flip_all(rules[0].truthtable);
    }

    *rules_ret = rules;
    *ret_n_rules = n_rules;
    *ret_n_samples = n_samples;
    return (0);

    err:
    ret = errno;

    /* Reclaim space. */
    if (rules != NULL) {
        for (i = 1; i < rule_cnt; i++) {
            bit_vector_free(rules[i].truthtable);
        }
        free(rules);
    }
    // (void)fclose(fi);
    return (ret);
}


/*
 * Preprocessing step.
 * INPUTS: Using the python from the BRL_code.py: Call get_freqitemsets
 * to generate data files of the form:
 * 	Rule<TAB><bit vector>\n
 *
 * OUTPUTS: an array of rule_t's
 */

int rules_init_from_file(const char *infile, int *n_rules, int *n_samples,
                         rule_data_t **rules_ret, int add_default_rule) {
    FILE *fi;

    if ((fi = fopen(infile, "r")) == NULL)
        return (errno);
    int ret = rules_init_from_stream(fi, n_rules, n_samples, rules_ret, add_default_rule);
    (void) fclose(fi);
    return ret;
}
