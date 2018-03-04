# PySBRL

Python interface of Scalable Bayesian Rule Lists described in the Paper: 
Yang, Hongyu, Cynthia Rudin, and Margo Seltzer. "Scalable Bayesian rule lists." arXiv preprint arXiv:1602.08610 (2016).

Original C Implementation: https://github.com/Hongyuy/sbrlmod by Hongyu Yang et al.

## Install

Now pysbrl is available through PyPi (with wheels for macOS and linux), you can install it via pip.

```bash
pip install pysbrl
```

## Usage

```python
import pysbrl

rule_ids, outputs, rule_strings = pysbrl.train_sbrl("data/ttt_train.out", "data/ttt_train.label", 20.0, eta=2.0, max_iters=2000, nchain=10, alphas=[1,1])
```

`rule_ids` is the list of ids of rules
`outputs` is the outputs matrix (prob distribution as a vector per rule)
`rule_strings` is the whole list of rules in the format of strings like `u'{c2=x,c4=o,c5=b}'`. 

You can retrieve the selected rules by 

```python
rules = [rule_strings[i] for i in rule_ids]
```

Also note that the last rule is always "default".

## Development

### Prerequisite

A few C libraries are required to compile this package

1. GNU GSL: You can download from here: https://www.gnu.org/software/gsl/

2. GMP: https://gmplib.org/

On macOS, you can easily install them via Homebrew

```bash
brew install gsl gmp
```

### Install

```bash
git clone https://github.com/myaooo/pysbrl
cd pysbrl
pip install -e .
```
