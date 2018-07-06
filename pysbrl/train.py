from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from pysbrl.pysbrl import train_sbrl as _train


def train_sbrl(data_file, label_file, lambda_=20, eta=2, max_iters=300000, n_chains=20, alpha=1, verbose=0):
    """
    The basic training function of the scalable bayesian rule list.
    Users are suggested to use SBRL instead of this function.
    It takes the paths of the pre-processed data and label files as input,
    and return the parameters of the trained rule list.

    Check pysbrl.utils:categorical2pysbrl_data to see how to convert categorical data to the required format
    :param data_file: The data file
    :param label_file: The label file
    :param lambda_: A hyper parameter, the prior representing the expected length of the rule list
    :param eta: A hyper parameter, the prior representing the expected length of each rule
    :param max_iters: The maximum iteration of the algo
    :param n_chains: The number of markov chains to run
    :param alpha: The prior of the output probability distribution, see the paper for more detail.
    :return: A tuple of (`rule_ids`, `outputs`, `rule_strings`)
        `rule_ids`: the list of ids of rules
        `outputs`: the outputs matrix (prob distribution as a vector per rule)
        `rule_strings`: the whole list of rules in the format of strings like `u'{c2=x,c4=o,c5=b}'`.

    """
    if isinstance(alpha, int):
        alphas = [alpha]
    elif isinstance(alpha, list):
        for a in alpha:
            assert isinstance(a, int)
        alphas = alpha
    else:
        raise ValueError('the argument alpha can only be int or List[int]')
    return _train(data_file, label_file, lambda_, eta, max_iters, n_chains, alphas, verbose)

