from pysbrl import BayesianRuleList
from sklearn.datasets import load_iris, load_breast_cancer
from sklearn.model_selection import train_test_split
from mdlp.discretization import MDLP


def test_BayesianRuleList():
    dataset = load_iris()
    x, y = dataset['data'], dataset['target']
    x_train, x_test, y_train, y_test = train_test_split(
        x, y, test_size=0.33, random_state=42)
    discretizer = MDLP().fit(x_train, y_train)
    x_train_cat = discretizer.transform(x_train)
    rule_list = BayesianRuleList(seed=1)
    rule_list.fit(x_train_cat, y_train, verbose=2)
    print(rule_list)
    x_test_cat = discretizer.transform(x_test)

    print('acc: %.4f' % rule_list.score(x_test_cat, y_test))


def test_BayesianRuleList2():
    dataset = load_breast_cancer()
    x, y = dataset['data'], dataset['target']
    x_train, x_test, y_train, y_test = train_test_split(
        x, y, test_size=0.33, random_state=42)
    discretizer = MDLP().fit(x_train, y_train)
    x_train_cat = discretizer.transform(x_train)
    rule_list = BayesianRuleList(seed=1)
    rule_list.fit(x_train_cat, y_train, verbose=2)
    print(rule_list)
    x_test_cat = discretizer.transform(x_test)

    print('acc: %.4f' % rule_list.score(x_test_cat, y_test))
