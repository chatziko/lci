![run-tests](../../workflows/test/badge.svg)

# LCI

LCI is an interpreter for the lambda calculus. It supports many advanced
[features](#features) such as recursion, user-defined operators and multiple evaluation
strategies, all based on the pure calculus. It is free software licenced under
the [GNU General Public Licence](http://www.gnu.org/licenses/gpl.html) (GPL).

## Try it online

LCI can run in a browser via [WebAssembly](https://webassembly.org/).
Try the [demo](https://www.chatzi.org/lci/demo/lci.html)!

## Features

LCI can be considered a small (but powerfull) functional progamming language
based on the pure lambda-calculus. Its features include:

- __Aliases__ of lambda terms (that is named functions).
- __Integers__
  - An arbitrary encoding can be used by defining `0, Succ, Pred, IsZero`.
  - Church encoding is used by default in `.lcirc`.
  - Scott encoding is also available (uncomment to use it)
- __Recursion__. Self-references of aliases are expanded during execution. 
  LCI can also automatically convert recursive terms to
  non-recursive ones using a fixed point combinator.
- __User-defined operators__. The user can declare a new
  operator with a certain precedence and associativity and define it in lambda
  calculus. Many common operators (eg. integer, logic and list operations) are
  pre-defined in [`.lcirc`](src/.lcirc) and are available by default.
- __List syntax__. `[a,b,c]` is parsed as `a:b:c:Nil` (`:` and `Nil` are defined in [`.lcirc`](src/.lcirc)).
- __Let syntax__. `let x = M in N` is parsed as `(\x.N) M`.
- __Multiple evaluation strategies__. Call-by-name and call-by-value can
  coexist in the same program.
- __Human-readable display__ of terms: for example church numerals are
  displayed as numbers and lists using the `[a,b,c]` notation.
- __Tracing__ of execution.
- File interpretation as well as interactive usage.
- A __library__ of pre-defined functions ([`.lcirc`](src/.lcirc)).

All features are implemented in the pure lambda calculus.
To demonstrate them, there is an implementation of the N-Queens problem
(`queens.lci`) in a way that reminds of Haskell syntax.

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

#### Building for WebAssembly

The browser version can be built with [emscripten](https://emscripten.org/).
You first need to build `make_dparser` with a normal build, then build
again with `emcmake`. The build is created under `build/html/dist`.
```
mkdir build && cd build
cmake ..
make make_dparser

rm CMakeCache.txt
emcmake cmake ..
emmake make
```

#### Contribution

If you have found a bug please [report it](https://github.com/chatziko/lci/issues).
Also feel free to send pull requests, or suggest features.

## Documentation

LCI's documentation covers most of the program's features and explains various
topics concerning the lambda-calculus

* [View](https://www.chatzi.org/lci/lcidoc.html) the documentation (html).
* [Download](https://www.chatzi.org/lci/lcidoc.pdf) the documentation (pdf).
