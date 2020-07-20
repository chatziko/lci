#LCI

LCI is an interpreter for the lambda calculus. It supports many advanced
features like recursion, user-defined operators and multiple evaluation
strategies, all based on the pure calculus. It is FREE SOFTWARE licenced under
the [GNU General Public Licence](http://www.gnu.org/licenses/gpl.html) (GPL).

##Install

####From source

The latest version (v0.6) is
[available here](https://github.com/chatziko/lci/releases/download/v0.6/lci-0.6.tar.gz).
To install extract the archive, cd to that directory and run:

```
./configure
make
sudo make install
```    

It is recommended to install the **readline** library (and development files)
before compiling. On ubuntu/debian install the ```libreadline-dev``` package.
Check the output of ```./configure``` to see if you have it.

You can also checkout the code from [github](https://github.com/chatziko/lci/).
In this case you need to install autotools and run ```./bootstrap```.

####Using Homebrew on OSX

Install [Homebrew](http://brew.sh) and run ```brew install lci```.

####Binaries for Windows

Windows binaries are
[available here](https://github.com/chatziko/lci/releases/download/v0.6/lci-0.6-win32.zip).
Simply extract and run the `lci` executable.


##Documentation

LCI's documentation covers most of the program's features and explains various
topics concerning the lambda-calculus

* [View](http://chatziko.github.io/lci/doc/index.html) documentation online.
* [Download](http://chatziko.github.io/lci/lcidoc.pdf) documentation (pdf)

