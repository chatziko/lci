![run-tests](../../workflows/test/badge.svg)

# LCI

LCI is an interpreter for the lambda calculus. It supports many advanced
[features](#features) such as recursion, user-defined operators and multiple evaluation
strategies, all based on the pure calculus. It is free software licenced under
the [GNU General Public Licence](http://www.gnu.org/licenses/gpl.html) (GPL).

## Try it online

LCI can run in a browser via [WebAssembly](https://webassembly.org/).
Try the [demo](https://www.chatzi.org/lci/demo/lci.html)!

## Install

#### From source

The latest version is
[available here](https://github.com/chatziko/lci/releases/).
To install extract the archive, cd to that directory and run:

```
cmake -B build
cd build && make
sudo make install
```    

This will install the `lci` executable in `/usr/local/bin` and `.lcirc, queens.lci` in
`/usr/local/share/lci`. You can install then in a different location by passing
`-DCMAKE_INSTALL_PREFIX=<dir>` to `cmake`.

#### Using Homebrew on OSX

Install [Homebrew](http://brew.sh) and run:

```
brew install lci
```

#### Binaries for Windows

Windows binaries are
[available here](https://github.com/chatziko/lci/releases/).
Simply extract and run the `lci` executable.


## Features

LCI can be considered a small (but powerfull) functional progamming language
based on the pure lambda-calculus. Its features include:

- Aliases of lambda terms (that is named functions).
- Integers coded as church numerals, with the usual arithmetic operations.
- Recursion. Self-references of aliases are expanded during execution. 
  LCI can also automatically convert recursive terms to
  non-recursive ones using a fixed point combinator.
- User-defined operators. The user can declare a new
  operator with a certain precedence and associativity and define it in lambda
  calculus. Many common operators (eg. integer, logic and list operations) are
  pre-defined in `.lcirc` and are available by default.
- List syntax. `[a,b,c]` is parsed as `a:b:c:Nil` (`:` and `Nil` are defined in `.lcirc`).
- Multiple evaluation strategies. Call-by-name and call-by-value can
  coexist in the same program.
- Human-readable display of terms: for example church numerals are
  displayed as numbers and lists using the `[a,b,c]` notation.
- Tracing of execution.
- File interpretation as well as interactive usage.
- Comes with a "library" of pre-defined functions (`.lcirc`).

All features are implemented in the pure lambda calculus.
To demonstrate them, there is an implementation of the N-Queens problem
(`queens.lci`) in a way that reminds of Haskell syntax.

#### Contribution

If you have found a bug please [report it](https://github.com/chatziko/lci/issues).
Also feel free to send pull requests, or suggest features.

## Documentation

LCI's documentation covers most of the program's features and explains various
topics concerning the lambda-calculus

* [View](https://www.chatzi.org/lci/lcidoc.html) the documentation (html).
* [Download](https://www.chatzi.org/lci/lcidoc.pdf) the documentation (pdf).
