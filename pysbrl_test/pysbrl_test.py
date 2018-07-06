import numpy as np
from pysbrl import BayesianRuleList, train_sbrl
from sklearn.datasets import load_iris, load_breast_cancer
from mdlp.discretization import MDLP


def test_train_sbrl():
    rule_ids, outputs, rule_strings = train_sbrl('./c/data/ttt_train.out', './c/data/ttt_train.label',
                                                 max_iters=10000, verbose=2)


# def test_BayesianRuleList():
#     dataset = load_iris()
#     x, y = dataset['data'], dataset['target']
#     discretizer = MDLP().fit(x, y)
#     x_cat = discretizer.transform(x)
#     rule_list = BayesianRuleList(seed=0)
#     rule_list.fit(x_cat, y, verbose=1)


# def test_rule_list():
#     x = np.ndarray([[0, 1], [0, 1], [0, 0], [0, 0], [1, 0], [1, 0], [1, 1], [1, 1]], dtype=np.int)
#     y = np.ndarray([0, 0, 0, 0, 1, 1, 1, 1])

