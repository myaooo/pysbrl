import numpy as np
from pysbrl import BayesianRuleList, train_sbrl
from sklearn.datasets import load_iris, load_breast_cancer
from mdlp.discretization import MDLP


def test_train_sbrl():
    rule_ids, outputs, rule_strings = train_sbrl('./data/ttt_train.out', './data/ttt_train.label',
                                                 max_iters=10000, verbose=1)
    # Test for a second run
    rule_ids, outputs, rule_strings = train_sbrl('./data/ttt_train.out', './data/ttt_train.label',
                                                 max_iters=10000, verbose=0)





# def test_rule_list():
#     x = np.ndarray([[0, 1], [0, 1], [0, 0], [0, 0], [1, 0], [1, 0], [1, 1], [1, 1]], dtype=np.int)
#     y = np.ndarray([0, 0, 0, 0, 1, 1, 1, 1])

