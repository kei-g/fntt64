# fntt64 [![license][license-image]][license-url]

`fntt64` - Fast Number Theoretic Transform 64x64

## :magic_wand: Abstract

Define a prime number $2^{64}-2^{32}+1$ as $p$, and $7$ is $p-1$ -th primitive root on $\mathbb Z/p\mathbb Z$

Define $r_{c,n}$ as $\left(\left(7^{2^{32}-1}\right)^{2^{(32-c)}}\right)^{n+1}\equiv r_{c,n}\mod p\quad\forall(c,n)\in\mathbb Z,1\leq c\leq 32,0\leq n\leq 2^c-1$

For two inputs $f(x),g(x)$, the convolution of them, $\left(f\ast g\right)$, is able to be calculated as below.

$$
F_{c,j}=\sum_{i=0}^{2^c-1}{r_{c,ij}f(i)}
\quad
G_{c,j}=\sum_{i=0}^{2^c-1}{r_{c,ij}g(i)}
$$

$$
\lparen f\ast g\rparen_{c,j}=\frac{1}{2^c}\sum_{i=0}^{2^c-1}{\dfrac{F_{c,j}G_{c,j}}{r_{c,ij}}}
$$

Please note that, $\dfrac{1}{2^c}$ is a value of $x$ which satisfies $2^cx\equiv 1\mod p$,

and also $\dfrac{1}{r_{c,ij}}$ is implied from $r_{c,ij}x\equiv 1\mod p$

`fntt64` depends on $r_{6,n}$ because $7^{2^{-6}\left(p-1\right)}\equiv 2^{39}\mod p$ is established.

I've described more detail in [高速数論変換で多倍長整数の畳み込み乗算][qiita-article-url]

## :green_heart: CI Status

| Action | Status |
|:-:|:-:|
| **Build** | [![GitHub CI (Build)][github-build-image]][github-build-url] |
| **CodeQL** | [![GitHub CI (CodeQL)][github-codeql-image]][github-codeql-url] |
| **Coverage** | [![GitHub CI (Coverage)][github-coverage-image]][github-coverage-url] |
| **Run** | [![GitHub CI (Run)][github-run-image]][github-run-url] |

## :page_facing_up: License

The scripts and documentation in this project are released under the [BSD-3-Clause License](https://github.com/kei-g/fntt64/blob/main/LICENSE)

[github-build-image]:https://github.com/kei-g/fntt64/actions/workflows/build.yml/badge.svg
[github-build-url]:https://github.com/kei-g/fntt64/actions/workflows/build.yml
[github-codeql-image]:https://github.com/kei-g/fntt64/actions/workflows/codeql.yml/badge.svg
[github-codeql-url]:https://github.com/kei-g/fntt64/actions/workflows/codeql.yml
[github-coverage-image]:https://github.com/kei-g/fntt64/actions/workflows/coverage.yml/badge.svg
[github-coverage-url]:https://github.com/kei-g/fntt64/actions/workflows/coverage.yml
[github-run-image]:https://github.com/kei-g/fntt64/actions/workflows/run.yml/badge.svg
[github-run-url]:https://github.com/kei-g/fntt64/actions/workflows/run.yml
[license-image]:https://img.shields.io/github/license/kei-g/fntt64
[license-url]:https://opensource.org/licenses/BSD-3-Clause
[qiita-article-url]:https://qiita.com/kei-g/items/1c389d00a3226c87ec29
