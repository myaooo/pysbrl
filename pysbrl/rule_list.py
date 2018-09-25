from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

try:
    from typing import List, Union
except ImportError:
    pass

import os
from functools import reduce
from collections import namedtuple
import time
import tempfile

import numpy as np

from pysbrl import train_sbrl

from pysbrl.utils import categorical2pysbrl_data


class Clause(namedtuple("Clause", ["feature_idx", "category"])):
    """
    A clause is an abstraction of the basic constraint used in a rule
    """
    def __str__(self):
        return "X%d = %d" % self.feature_idx, self.category


class Rule(namedtuple("Rule", ["clauses", "output"])):
    """
    A Rule contains two members:
        clauses: a list of Clauses, denoting the antecedent of the rule (the IF part)
        output: a list of floats, denoting the probability outputs of the rule (the THEN part)
    """
    def is_default(self):
        return len(self.clauses) == 0

    def is_satisfied(self, x_cat):
        """
        :param x_cat: a 2D array of pure categorical data of shape [n_data, n_features]
        :return: a bool array of shape [n_data,] representing whether the rule is fired by each input data.
        """
        satisfied = []
        if self.is_default():
            return np.ones(x_cat.shape[0], dtype=bool)
        for idx, cat in self.clauses:
            satisfied.append(x_cat[:, idx] == cat)
        return reduce(np.logical_and, satisfied)


def print_rule(rule, feature_names=None, category_names=None, label="label", support=None):
    # type: (Rule, List[Union[str, None]], List[Union[List[str], None]], str, List[int]) -> str
    """
    print the rule in a nice way
    :param rule: An instance of Rule
    :param feature_names: a list of n_features feature names,
    :param category_names: the names of the categories of each feature.
    :param label: Can take two values, 'label': just print the label as output, or 'prob': print the prob distribution.
    :param support: optional values of the data that support the rule
    :return: str
    """
    pred_label = np.argmax(rule.output)
    if label == "label":
        output = str(pred_label) + " ({})".format(rule.output[pred_label])
    elif label == "prob":
        output = "[{}]".format(", ".join(["{:.4f}".format(prob) for prob in rule.output]))
    else:
        raise ValueError("Unknown label {}".format(label))
    output = "{}: ".format(label) + output

    default = rule.is_default()
    if default:
        s = "DEFAULT " + output
    else:
        if feature_names is None:
            _feature_names = ["X" + str(idx) for idx, _ in rule.clauses]
        else:
            _feature_names = [feature_names[idx] for idx, _ in rule.clauses]

        categories = []
        for feature_idx, category in rule.clauses:
            if category_names is None:
                _category_names = None
            else:
                _category_names = category_names[feature_idx]
            if _category_names is None:
                categories.append(" = " + str(category))
            else:
                categories.append(" in " + str(_category_names[category]))

        s = "IF "
        # conditions
        conditions = ["({}{})".format(feature, category) for feature, category in zip(_feature_names, categories)]
        s += " AND ".join(conditions)
        # results
        s += " THEN " + output

    if support is not None:
        support_list = [("+" if i == pred_label else "-") + str(supp) for i, supp in enumerate(support)]
        s += " [" + "/".join(support_list) + "]"
    return s


def rule_str2rule(rule_str, prob):
    # type: (str, float) -> Rule
    """
    A helper function that converts the resulting string returned from C function to the Rule object

    :param rule_str: a string representing the rule
    :param prob: the output probability
    :return: a Rule object
    """
    if rule_str == "default":
        return Rule([], prob)

    raw_rules = rule_str[1:-1].split(",")
    clauses = []
    for raw_rule in raw_rules:
        idx = raw_rule.find("=")
        if idx == -1:
            raise ValueError("No \"=\" find in the rule!")
        clauses.append(Clause(int(raw_rule[1:idx]), int(raw_rule[(idx + 1):])))
    return Rule(clauses, prob)


class BayesianRuleList(object):
    """
    Python Wrapper of Scalable Bayesian Rule List

    """

    def __init__(self, min_rule_len=1, max_rule_len=2, min_support=0.01, lambda_=20, eta=1, iters=30000,
                 n_chains=30, alpha=1, fim_method='eclat', feature_names=None, category_names=None, seed=None,
                 verbose=0):
        # type: (int, int, float, float, float, int, int, int) -> None
        # parameters used for the SBRL algorithm
        self.min_rule_len = min_rule_len
        self.max_rule_len = max_rule_len
        self.min_support = min_support
        self.lambda_ = lambda_
        self.alpha = alpha
        self.eta = eta
        self.iters = iters
        self.n_chains = n_chains
        self.fim_method = fim_method
        self.seed = seed
        self.verbose = verbose

        # values useful for printing
        self.feature_names = feature_names
        self.category_names = category_names

        # Meta info about the data
        self._n_classes = None
        self._n_features = None

        # Raw results from C function
        self._rule_ids = None
        self._rule_outputs = None
        self._rule_pool = None

        # model parameters
        self._rule_list = []  # type: List[Rule]
        # cached results of the support information from training data
        self._supports = None

    @property
    def rule_list(self):
        return self._rule_list

    @property
    def n_rules(self):
        return len(self._rule_list)

    @property
    def n_classes(self):
        return self._n_classes

    @property
    def n_features(self):
        return self._n_features

    def fit(self, x, y):
        """
        :param x: 2D np.ndarray (n_instances, n_features) should be categorical data, must be of type int
        :param y: 1D np.ndarray (n_instances, ) labels
        :return:
        """
        verbose = self.verbose
        # Create temporary files
        data_file = tempfile.NamedTemporaryFile("w+b", delete=False)
        label_file = tempfile.NamedTemporaryFile("w+b", delete=False)

        start = time.time()
        raw_rules = categorical2pysbrl_data(x, y, data_file.name, label_file.name, supp=self.min_support,
                                            zmin=self.min_rule_len, zmax=self.max_rule_len, method=self.fim_method)
        if verbose > 1:
            print("Info: sbrl data files saved to %s and %s temporarily" % (data_file.name, label_file.name))
        data_file.close()
        label_file.close()
        cat_time = time.time() - start
        if verbose:
            print("Info: time for rule mining: %.4fs" % cat_time)
        n_labels = int(np.max(y)) + 1
        start = time.time()
        _model = train_sbrl(data_file.name, label_file.name, self.lambda_, eta=self.eta,
                            max_iters=self.iters, n_chains=self.n_chains, seed=self.seed,
                            alpha=self.alpha, verbose=verbose)
        train_time = time.time() - start
        if verbose:
            print("Info: training time: %.4fs" % train_time)

        # update model parameters
        self._n_classes = n_labels
        self._n_features = x.shape[1]

        # convert the raw parameters to rules
        self.from_raw(_model[0], _model[1], raw_rules)
        self._supports = self.compute_support(x, y)

        # Close the temp files
        os.unlink(data_file.name)
        os.unlink(label_file.name)

    def from_raw(self, rule_ids, outputs, raw_rules):
        """
        A helper function that converts the results returned from C function
        :param rule_ids:
        :param outputs:
        :param raw_rules:
        :return:
        """
        self._rule_pool = [([], [])] + raw_rules
        self._rule_list = []
        for i, idx in enumerate(rule_ids):
            rule = Rule([Clause(f, c) for f, c in zip(*self._rule_pool[idx])], outputs[i])
            self._rule_list.append(rule)
            # self._rule_list.append(rule_str2rule(_rule_name, outputs[i]))
        self._rule_ids = rule_ids
        self._rule_outputs = outputs

    def compute_support(self, x, y):
        # type: (np.ndarray, np.ndarray) -> np.ndarray
        """
        Calculate the support for the rules.
        The support of each rule is a list of `n_classes` integers: [l1, l2, ...].
        Each integer represents the number of data of label i that is caught by this rule
        :param x: 2D np.ndarray (n_instances, n_features) should be categorical data, must be of type int
        :param y: 1D np.ndarray (n_instances, ) labels
        :return: a np.ndarray of shape (n_rules, n_classes)
        """
        caught_matrix = self.caught_matrix(x)
        if np.sum(caught_matrix.astype(np.int)) != x.shape[0]:
            raise RuntimeError("The sum of the support should equal to the number of instances!")
        support_summary = np.zeros((self.n_rules, self.n_classes), dtype=np.int)
        for i, support in enumerate(caught_matrix):
            support_labels = y[support]
            unique_labels, unique_counts = np.unique(support_labels, return_counts=True)
            if len(unique_labels) > 0 and np.max(unique_labels) > support_summary.shape[1]:
                # There occurs labels that have not seen in training
                pad_len = np.max(unique_labels) - support_summary.shape[1]
                support_summary = np.hstack((support_summary, np.zeros((self.n_rules, pad_len), dtype=np.int)))
            support_summary[i, unique_labels] = unique_counts
        return support_summary

    def caught_matrix(self, x):
        # type: (np.ndarray) -> np.ndarray
        """
        compute the caught matrix of x
        Each rule has an array of bools, showing whether each instances is caught by this rule
        :param x: 2D np.ndarray (n_instances, n_features) should be categorical data, must be of type int
        :return:
            a bool np.ndarray of shape (n_rules, n_instances)
        """
        un_satisfied = np.ones((x.shape[0],), dtype=np.bool)
        supports = np.zeros((self.n_rules, x.shape[0]), dtype=np.bool)
        for i, rule in enumerate(self._rule_list):
            is_satisfied = rule.is_satisfied(x)
            satisfied = np.logical_and(is_satisfied, un_satisfied)
            # marking new satisfied instances as satisfied
            un_satisfied = np.logical_xor(satisfied, un_satisfied)
            supports[i, :] = satisfied
        return supports

    def decision_path(self, x):
        # type: (np.ndarray) -> np.ndarray
        """
        compute the decision path of the rule list on x
        :param x: x should be already transformed
        :return:
            return a np.ndarray of shape [n_rules, n_instances] of type bool,
            representing whether an instance has consulted a rule or not
        """
        un_satisfied = np.ones([x.shape[0]], dtype=np.bool)
        paths = np.zeros((self.n_rules, x.shape[0]), dtype=np.bool)
        for i, rule in enumerate(self._rule_list):
            paths[i, :] = un_satisfied
            satisfied = rule.is_satisfied(x)
            # marking new satisfied instances as satisfied
            un_satisfied = np.logical_and(np.logical_not(satisfied), un_satisfied)
        return paths

    def predict_proba(self, x):
        """

        :param x: an instance of np.ndarray object, representing the data to be making predictions on.
        :return: `prob` if `rt_support` is `False`, `(prob, supports)` if `rt_support` is `True`.
            `prob` is a 2D array with shape `(n_instances, n_classes)`.
            `supports` is a list of (n_classes,) 1D arrays denoting the support.
        """
        _x = x

        n_classes = self._rule_outputs.shape[1]
        y = np.empty((_x.shape[0], n_classes), dtype=np.double)
        un_satisfied = np.ones([_x.shape[0]], dtype=bool)
        for rule in self._rule_list:
            is_satisfied = rule.is_satisfied(_x)
            satisfied = np.logical_and(is_satisfied, un_satisfied)
            y[satisfied] = rule.output
            # marking new satisfied instances as satisfied
            # un_satisfied = un_satisfied & (~satisfied)
            # Since there can not be unsatisfied = 0 AND satisfied = 1
            # It is the same as xor
            un_satisfied = np.logical_xor(satisfied, un_satisfied)

        return y

    def predict(self, x):
        y_prob = self.predict_proba(x)
        y_pred = np.argmax(y_prob, axis=1)
        return y_pred

    def score(self, x, y, sample_weight=None):
        y_pred = self.predict(x)
        corrects = (y == y_pred).astype(np.float32)
        if sample_weight is None:
            return np.mean(corrects)
        else:
            return np.average(corrects, weights=sample_weight)

    def _print(self, feature_names=None, category_names=None, rt_str=False):
        s = "The rule list contains {:d} of rules:\n\n     ".format(self.n_rules)

        for i, rule in enumerate(self._rule_list):
            is_last = rule.is_default()
            # categories = None if category_names is None else category_names[i]
            s += print_rule(rule, feature_names, category_names, label="prob") + "\n"
            if len(self._rule_list) > 1 and not is_last:
                s += "\nELSE "

        if rt_str:
            return s
        print(s)

    def __str__(self):
        return self._print(self.feature_names, self.category_names, rt_str=True)
