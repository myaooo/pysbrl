# PySBRL

Python interface of Scalable Bayesian Rule Lists described in the Paper: 
Yang, Hongyu, Cynthia Rudin, and Margo Seltzer. "Scalable Bayesian rule lists." arXiv preprint arXiv:1602.08610 (2016).

Original C Implementation: https://github.com/Hongyuy/sbrlmod by Hongyu Yang et al.

## Prerequisite

A few C libraries are required to compile this package

1. GNU GSL: You can download from here: https://www.gnu.org/software/gsl/

2. GMP: https://gmplib.org/

On macOS, you can easily install them via Homebrew

```bash
brew install gsl gmp
```

## Install

```bash
git clone https://github.com/myaooo/pysbrl
cd pysbrl
pip install -e .
```

## Usage

```python
import pysbrl

rule_ids, outputs, rule_strings = pysbrl.train_sbrl("data/ttt_train.out", "data/ttt_train.label", 20.0, eta=2.0, max_iters=2000, nchain=10, alphas=[1,1])
```