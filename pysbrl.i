/* pysbrl.i */
%module pysbrl
%{
/* Put header files here or function declarations like below */
#define SWIG_FILE_WITH_INIT
#include "pysbrl.h"
%}

%include "typemaps.i"

%include "numpy.i"
%init %{
import_array();
%}

%include "typemaps.i"

%typemap(in,numinputs=0)
    (int* N_STRINGS, char*** STRINGS)
    (int tmp_n=1, char** tmp_strings=NULL)
{
    $2 = &tmp_strings;
    $1 = &tmp_n;
}

%typemap(argout)
  (int* N_STRINGS, char*** STRINGS)
{
    char ** _strings = *$2;
    int _n_strings = *$1;
    PyObject *o = PyList_New(_n_strings);
    for (int i = 0; i < _n_strings; i++) {
        PyList_SetItem(o, i, PyUnicode_FromString(_strings[i]));
    }
    $result = SWIG_Python_AppendOutput($result, o);
}

%typemap(out) int {
    if ($1 == 2) {
        PyErr_SetString(PyExc_LookupError, "No such file or directory");
        SWIG_fail;
    }
}

/* %typemap(default) int VERBOSE {
  $1 = 0;
} */

%apply (const char * STRING) {const char * data_file, const char * label_file};
%apply (int* IN_ARRAY1, int DIM1) {(int* alphas, int n_alpha)};
/* %apply (int VERBOSE) {(int verbose)}; */

%apply (int* DIM1, int** ARGOUTVIEWM_ARRAY1)
{(int* ret_n_rules, int** ret_rule_ids)};

%apply (int* DIM1, int* DIM2, double** ARGOUTVIEWM_ARRAY2) 
{(int* ret_n_probs, int* ret_n_classes, double** ret_probs)};

%apply (int* N_STRINGS, char*** STRINGS)
{(int* ret_n_all_rules, char*** ret_all_rule_features)};

int train_sbrl(const char *data_file, const char *label_file,
    double lambda, double eta, int max_iters, int nchain, int * alphas, int n_alpha,
    int *ret_n_rules, int ** ret_rule_ids, 
    int *ret_n_probs, int *ret_n_classes, double ** ret_probs,
    int *ret_n_all_rules, char *** ret_all_rule_features);

/*
extern pred_model_t *train(data_t *, int, int, params_t *);
extern gsl_matrix *predict(pred_model_t *, rule_t *labels, params_t *);
extern int load_data(const char *, const char *, int *, int *, int *, rule_t **, rule_t **);
*/

