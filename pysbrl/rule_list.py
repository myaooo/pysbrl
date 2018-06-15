from functools import reduce
from collections import namedtuple
import time
import tempfile

import numpy as np

from pysbrl import train_sbrl

from pysbrl.utils import categorical2pysbrl_data


class Clause(namedtuple("Clause", ["feature_idx", "category"])):
    def __str__(self):
        return "X%d = %d" % self.feature_idx, self.category


class Rule(namedtuple("Rule", ["clauses", "output"])):
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
        for idx, cat in zip(self.clauses):
            satisfied.append(x_cat[:, idx] == cat)
        return reduce(np.logical_and, satisfied)


def print_rule(rule, feature_names=None, category_names=None, label="label", support=None):
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
            _feature_names = ["X" + str(idx) for idx in rule.feature_indices]
        else:
            _feature_names = [feature_names[idx] for idx in rule.feature_indices]

        categories = []
        for cat_name, cat in zip(category_names, rule.categories):
            if cat_name is None:
                categories.append(" = " + str(cat))
            else:
                categories.append(" in " + str(cat_name))

        s = "IF "
        # conditions
        conditions = ["({}{})".format(feature, category) for feature, category in zip(_feature_names, categories)]
        s += " and ".join(conditions)
        # results
        s += " THEN " + output

    if support is not None:
        support_list = [("+" if i == pred_label else "-") + str(supp) for i, supp in enumerate(support)]
        s += " [" + "/".join(support_list) + "]"
    return s


def rule_str2rule(rule_str, prob):
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


class SBRL:
    """
    Python Wrapper of Scalable Bayesian Rule List
    """

    def __init__(self, rule_minlen=1, rule_maxlen=2,
                 min_support=0.01, _lambda=50, eta=1, iters=30000, nchain=30, alpha=1):
        self._r_model = None
        self.rule_minlen = rule_minlen
        self.rule_maxlen = rule_maxlen
        self.min_support = min_support
        self._lambda = _lambda
        self.alpha = alpha
        self.eta = eta
        self.iters = iters
        self.nchain = nchain

        self._rule_indices = None
        self._rule_probs = None
        self._rule_names = None
        self._rule_list = []
        self._n_classes = None
        self._n_features = None

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

        :param x: 2D np.ndarray (n_instances, n_features) could be continuous
        :param y: 1D np.ndarray (n_instances, ) labels
        :return:
        """

        data_file = tempfile.NamedTemporaryFile("w")
        label_file = tempfile.NamedTemporaryFile("w")

        start = time.time()
        categorical2pysbrl_data(x, y, data_file.name, label_file.name, supp=self.min_support,
                                zmin=self.rule_minlen, zmax=self.rule_maxlen)
        cat_time = time.time() - start
        print("time for rule mining:", cat_time)
        n_labels = int(np.max(y)) + 1
        start = time.time()
        _model = train_sbrl(data_file.name, label_file.name, self._lambda, eta=self.eta,
                            max_iters=self.iters, nchain=self.nchain,
                            alphas=[self.alpha for _ in range(n_labels)])
        train_time = time.time() - start
        print("training time:", train_time)
        self._n_classes = n_labels
        self._n_features = x.shape[1]
        self._rule_indices = _model[0]
        self._rule_probs = _model[1]
        self._rule_names = _model[2]

        self.post_process(x, y)

        data_file.close()
        label_file.close()

    def post_process(self, x, y):
        """
        Post process function that clean the extracted rules
        :return:
        """
        # trim_threshold = 0.0005 * len(y)
        self._rule_list = []
        for i, idx in enumerate(self._rule_indices):
            _rule_name = self._rule_names[idx]
            self._rule_list.append(rule_str2rule(_rule_name, self._rule_probs[i]))
        support_summary = self.compute_support(x, y)
        for rule, support in zip(self._rule_list, support_summary):
            rule.support = support

    def compute_support(self, x, y) -> np.ndarray:
        """
        Calculate the support for the rules
        :param x:
        :param y:
        :return:
        """
        n_classes = self.n_classes
        n_rules = self.n_rules
        supports = self.decision_support(x)
        if np.sum(supports.astype(np.int)) != x.shape[0]:
            raise RuntimeError("The sum of the support should equal to the number of instances!")
        support_summary = np.zeros((n_rules, n_classes), dtype=np.int)
        for i, support in enumerate(supports):
            support_labels = y[support]
            unique_labels, unique_counts = np.unique(support_labels, return_counts=True)
            if len(unique_labels) > 0 and np.max(unique_labels) > support_summary.shape[1]:
                # There occurs labels that have not seen in training
                pad_len = np.max(unique_labels) - support_summary.shape[1]
                support_summary = np.hstack((support_summary, np.zeros((n_rules, pad_len), dtype=np.int)))
            support_summary[i, unique_labels] = unique_counts
        return support_summary

    def decision_support(self, x) -> np.ndarray:
        """
        compute the decision support of the rule list on x
        :param x: x should be already transformed
        :return:
            return a list of n_rules np.ndarray of shape [n_instances,] of type bool
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

    def decision_path(self, x) -> np.ndarray:
        """
        compute the decision path of the rule list on x
        :param x: x should be already transformed
        :return:
            return a np.ndarray of shape [n_rules, n_instances] of type bool,
            representing whether an instance has
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

        :param x: an instance of pandas.DataFrame object, representing the data to be making predictions on.
        :return: `prob` if `rt_support` is `False`, `(prob, supports)` if `rt_support` is `True`.
            `prob` is a 2D array with shape `(n_instances, n_classes)`.
            `supports` is a list of (n_classes,) 1D arrays denoting the support.
        """
        _x = x

        n_classes = self._rule_probs.shape[1]
        y = np.empty((_x.shape[0], n_classes), dtype=np.double)
        un_satisfied = np.ones([_x.shape[0]], dtype=bool)
        for rule in self._rule_list:
            is_satisfied = rule.is_satisfied(_x)
            satisfied = np.logical_and(is_satisfied, un_satisfied)
            y[satisfied] = rule.output
            # marking new satisfied instances as satisfied
            un_satisfied = np.logical_xor(satisfied, un_satisfied)
        return y

    def predict(self, x):
        y_prob = self.predict_proba(x)
        # print(y_prob[:50])
        y_pred = np.argmax(y_prob, axis=1)
        return y_pred

    def __str__(self, feature_names=None, category_names=None, rt_str=False):
        s = "The rule list has {} of rules:\n\n     ".format(self.n_rules)

        for i, rule in enumerate(self._rule_list):
            is_last = rule.is_default()
            categories = None if category_names is None else category_names[i]
            s += print_rule(rule, feature_names, categories, label="prob") + "\n"
            if len(self._rule_list) > 1 and not is_last:
                s += "\nELSE "

        if rt_str:
            return s
        print(s)
