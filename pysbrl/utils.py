from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import functools

import numpy as np
import fim


def before_save(file_or_dir):
    """
    make sure that the dedicated path exists (create if not exist)
    :param file_or_dir:
    :return: None
    """
    dir_name = os.path.dirname(os.path.abspath(file_or_dir))
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)


def get_fim_method(method='eclat'):
    if hasattr(fim, method):
        return getattr(fim, method, fim.eclat)
    return fim.eclat


def categorical2pysbrl_data(
        x,
        y,
        data_filename,
        label_filename,
        method='eclat',
        supp=0.05,
        zmin=1,
        zmax=3):
    """
    Run a frequent item mining algorithm to extract candidate rules.
    :param x: 2D np.ndarray, categorical data of shape [n_instances, n_features]
    :param y: 1D np.ndarray, label array of shape [n_instances, ]
    :param data_filename: the path to store data file
    :param label_filename: the path to store label file
    :param method: a str denoting the method to use, default to 'eclat'
    :param supp: the minimum support of a rule (item)
    :param zmin:
    :param zmax:
    :return:
    """

    # Safely cast data types
    x = x.astype(np.int, casting='safe')
    y = y.astype(np.int, casting='safe')

    labels = np.unique(y)
    labels = np.arange(np.max(labels) + 1)
    # assert max(labels) + 1 == len(labels)

    mine = get_fim_method(method)

    x_by_labels = []
    for label in labels:
        x_by_labels.append(x[y == label])
    transactions_by_labels = [categorical2transactions(_x) for _x in x_by_labels]
    itemsets = transactions2freqitems(transactions_by_labels, mine, supp=supp, zmin=zmin, zmax=zmax)
    rules = [itemset2feature_categories(itemset) for itemset in itemsets]
    data_by_rule = []
    for features, categories in rules:
        satisfied = rule_satisfied(x, features, categories)
        data_by_rule.append(satisfied)

    # Write data file
    # data_filename = get_path(_datasets_path, data_name+'.data')
    before_save(data_filename)
    with open(data_filename, 'w') as f:
        f.write('n_items: %d\n' % len(itemsets))
        f.write('n_samples: %d\n' % len(y))
        for itemset, data in zip(itemsets, data_by_rule):
            rule_str = '{' + ','.join(itemset) + '}' + '  '
            f.write(rule_str)
            bit_s = ' '.join(['1' if bit else '0' for bit in data])
            f.write(bit_s)
            f.write('\n')

    # Write label file
    # label_filename = get_path(_datasets_path, data_name+'.label')
    before_save(label_filename)
    with open(label_filename, 'w') as f:
        f.write('n_items: %d\n' % len(labels))
        f.write('n_samples: %d\n' % len(y))
        for label in labels:
            f.write('{label=%d} ' % label)
            bits = np.equal(y, label)
            bit_s = ' '.join(['1' if bit else '0' for bit in bits])
            f.write(bit_s)
            f.write('\n')
    return rules


def categorical2transactions(x):
    # type: (np.ndarray) -> List
    """
    Convert a 2D int array into a transaction list:
        [
            ['x0=1', 'x1=0', ...],
            ...
        ]
    :param x:
    :return:
    """
    assert len(x.shape) == 2

    transactions = []
    for entry in x:
        transactions.append(['x%d=%d' % (i, val) for i, val in enumerate(entry)])

    return transactions


def itemset2feature_categories(itemset):
    features = []
    categories = []
    for item in itemset:
        idx = item.find('=')
        if idx == '-1':
            raise ValueError("No '=' find in the rule!")
        features.append(int(item[1:idx]))
        categories.append(int(item[(idx + 1):]))
    return features, categories


def transactions2freqitems(transactions_by_labels, mine, supp=0.05, zmin=1, zmax=3):

    supp = int(supp*100)
    itemsets = set()
    for trans in transactions_by_labels:
        itemset = [tuple(sorted(r[0])) for r in mine(trans, supp=supp, zmin=zmin, zmax=zmax)]
        itemsets |= set(itemset)

    itemsets = list(itemsets)

    # print("Total {:d} itemsets mined".format(len(itemsets)))
    return itemsets


def rule_satisfied(x, features, categories):
    """
    return a logical array representing whether entries in x satisfied the rules denoted by features and categories
    :param x: a categorical 2D array
    :param features: a list of feature indices
    :param categories: a list of categories
    :return:
    """
    satisfied = []
    if features[0] == -1 and len(features) == 1:
        # Default rule, all satisfied
        return np.ones(x.shape[0], dtype=bool)
    for idx, cat in zip(features, categories):
        # Every single condition needs to be satisfied.
        satisfied.append(x[:, idx] == cat)
    return functools.reduce(np.logical_and, satisfied)
