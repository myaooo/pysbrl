import numpy as np
from pysbrl import BayesianRuleList
from sklearn.datasets import load_iris, load_breast_cancer
from sklearn.model_selection import train_test_split
from mdlp.discretization import MDLP


def compute_intervals(mdlp_discretizer):
    category_names = []
    for i, cut_points in enumerate(mdlp_discretizer.cut_points_):
        idxs = np.arange(len(cut_points) + 1)
        names = mdlp_discretizer.assign_intervals(idxs, i)
        category_names.append(names)
    return category_names


def test_BayesianRuleList():
    dataset = load_iris()
    x, y = dataset['data'], dataset['target']
    feature_names = dataset['feature_names']
    x_train, x_test, y_train, y_test = train_test_split(
        x, y, test_size=0.33, random_state=42)
    discretizer = MDLP(random_state=42).fit(x_train, y_train)
    x_train_cat = discretizer.transform(x_train)
    category_names = compute_intervals(discretizer)
    rule_list = BayesianRuleList(seed=1, feature_names=feature_names, category_names=category_names, verbose=2)
    rule_list.fit(x_train_cat, y_train)
    print(rule_list)
    x_test_cat = discretizer.transform(x_test)

    print('acc: %.4f' % rule_list.score(x_test_cat, y_test))


def test_BayesianRuleList2():
    dataset = load_breast_cancer()
    x, y = dataset['data'], dataset['target']
    feature_names = dataset['feature_names']
    x_train, x_test, y_train, y_test = train_test_split(
        x, y, test_size=0.33, random_state=42)
    discretizer = MDLP(random_state=42).fit(x_train, y_train)
    x_train_cat = discretizer.transform(x_train)
    category_names = compute_intervals(discretizer)
    rule_list = BayesianRuleList(seed=1, feature_names=feature_names, category_names=category_names, verbose=2)
    rule_list.fit(x_train_cat, y_train)
    print(rule_list)
    x_test_cat = discretizer.transform(x_test)

    print('acc: %.4f' % rule_list.score(x_test_cat, y_test))
