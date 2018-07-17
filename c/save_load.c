//
// Created by Ming Yao on 6/29/18.
//

#include <assert.h>
#include <string.h>
#include <limits.h>

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
    char *cp, *features, *rulestr;
//    int rule_cnt;
    int n_rules = 0, n_samples = 0, i;
    long tmpl;
    int ret;
    rule_data_t *rules = NULL;
    unsigned buffer_size = 64;
    char *buffer = calloc(buffer_size, sizeof(char));
    char *end;
    char delim = ' ', delim2 = '\t';

    /*
     * Leave a space for the 0th (default) rule, which we'll add at
     * the end.
     */
    add_default_rule = add_default_rule ? 1 : 0;
//    rule_cnt = add_default_rule != 0 ? 1 : 0;

    // Read the first two lines
    if (fgets(buffer, buffer_size, fi) == NULL || strncmp(buffer, "n_items:", 8)!= 0) {
        fprintf(stderr, "Error: data file mal-format! The first line should be n_items: xxx\n");
        goto err;
    }
    tmpl = strtol(buffer+8, &end, 10);
    if (tmpl > INT_MAX){
        fprintf(stderr, "Error: n_items range error, got %ld\n", tmpl);
        goto err;
    }
    n_rules = (int) tmpl;
    if (fgets(buffer, buffer_size, fi) == NULL || strncmp(buffer, "n_samples:", 10)!= 0) {
        fprintf(stderr, "Error: data file mal-format! The first line should be n_samples: xxx\n");
        goto err;
    }
    tmpl = strtol(buffer+10, &end, 10);
    if (tmpl > INT_MAX){
        fprintf(stderr, "Error: n_samples range error, got %ld\n", tmpl);
        goto err;
    }
    n_samples = (int) tmpl;
    rules = calloc((unsigned long) (n_rules + add_default_rule), sizeof(rule_data_t));

    buffer_size = (unsigned) n_samples * 3 + 100;
    buffer = realloc(buffer, buffer_size * sizeof(char));

    for (i = 0; i < n_rules; i++) {
        if(fgets(buffer, buffer_size, fi) == NULL) {
            if (feof(fi))
                fprintf(stderr, "Error: Unexpected end of file at line %d, expected %d lines\n", i+2, n_rules+2);
            if (ferror(fi))
                fprintf(stderr, "Error: fgets error!\n");
            errno = ENOEXEC;
            goto err;
        }
        end = buffer;

        while((*end) != delim && (*end) != delim2 && (*end) != '\0') {
            end++;
        }
        if(*end == '\0') {
            fprintf(stderr, "%s", buffer);
//            fprintf(stderr, "Line length: %d\n", (int) strlen(buffer));
            fprintf(stderr, "Error: cannot find '%c' or '%c' to split the line!\n", delim, delim2);
            errno = ENOEXEC;
            goto err;
        }
        features = end+1;
        *end = '\0';
        rulestr = buffer;
        if ((rules[i + add_default_rule].feature_str = _strdup(rulestr)) == NULL)
            goto err;

        if ((rules[i + add_default_rule].truthtable = bit_vector_from_str(features)) == NULL)
            goto err;

        rules[i + add_default_rule].cardinality = 1;
        for (cp = rulestr; *cp != '\0'; cp++)
            if (*cp == ',')
                rules[i + add_default_rule].cardinality++;

    }

    /* Now create the 0'th (default) rule. */
    if (add_default_rule) {
        rules[0].cardinality = 0;
        if ((rules[0].truthtable = bit_vector_init((bit_size_t) n_samples)) == NULL)
            goto err;
        rules[0].feature_str = _strdup("default");
        bit_vector_flip_all(rules[0].truthtable);
    }

    *rules_ret = rules;
    *ret_n_rules = n_rules + add_default_rule;
    *ret_n_samples = n_samples;
    return (0);

err:
    ret = errno;
    free(buffer);
    /* Reclaim space. */
    if (rules != NULL) {
        rules_free(rules, n_rules);
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
