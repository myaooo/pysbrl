import numpy as np

from pysbrl import SBRL, train_sbrl


def test_train_sbrl():
    rule_ids, outputs, rule_strings = train_sbrl('./data/ttt_train.out', './data/ttt_train.label', verbose=1)


# def test_rule_list():
#     x = np.ndarray([[0, 1], [0, 1], [0, 0], [0, 0], [1, 0], [1, 0], [1, 1], [1, 1]], dtype=np.int)
#     y = np.ndarray([0, 0, 0, 0, 1, 1, 1, 1])

