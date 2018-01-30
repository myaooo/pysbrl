# PySBRL

Python interface of Scalable Bayesian Rule Lists described in the Paper: 
Yang, Hongyu, Cynthia Rudin, and Margo Seltzer. "Scalable Bayesian rule lists." arXiv preprint arXiv:1602.08610 (2016).

Original C Implementation: https://github.com/Hongyuy/sbrlmod by Hongyu Yang et al.

## Install

```bash
git clone https://github.com/myaooo/pysbrl
cd pysbrl
pip install -e .
```

## Usage

```python
import pysbrl

model = pysbrl.train_sbrl("data/ttt_train.out", "data/ttt_train.label", lambda=20.0, eta=2.0, max_iters=2000, nchains=10, alphas=[1,1])
```