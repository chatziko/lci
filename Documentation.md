# LCI Documentation

This document describes the use of `lci`, an advanced interpreter for the
$\lambda$-calculus. It executes pure $\lambda$-calculus terms,
extended with a variety of advanced features which
are all encoded in the pure calculus.

## Syntax

`lci` supports the syntax of the pure $\lambda$-calculus:

    <term> = <var>                         (variable)
           | λ<var>. <term>                (abstraction)
           | <term> <term>                 (application)


The symbol $\lambda$ can be written as `\` or as the unicode
character λ. Variables `<var>` can contain latin
letters (uppercase or lowercase), numbers and underscores, but
they must start with a lowercase letter or underscore.

To make programming easier, `lci` extends the $\lambda$-calculus
with several features such as __integers__, __lists__, __operators__,
__aliases__ and __bindings__:

    <term> = ...
           | <int>                         (integer)
           | [<term>, <term>, ...]         (list)
           | <term> <oper> <term>          (operator application)
           | <alias>                       (alias)
           | let <var> = <term> in <term>  (binding)

Note that all these features are simply __syntactic sugar__ since they
are encoded in the pure calculus, while the actual execution involves
pure $\lambda$-calculus reductions.

`<alias>` can contain the same characters as variables but it must start
with an uppercase letter, or it can contain arbitrary characters
if enclosed in single quotes. `<int>` is an unsigned
integer and `<oper>` is a string containing the following characters:

    + - = ! @ $ % ^ & * / \ : < > . , | ~ ?

except from the reserved operators `. \ = ~=`.

Finally, parentheses can be used to disambinguiate parsing, based on
the following rules:

-   Outmost parentheses can be avoided
-   Application is left-associative
-   The scope of an abstraction extends as far to the right as possible
-   Terms that contain [operators](#operators) are parsed according to their precedence
    and associativity.

## Executing programs

When executed with no arguments, `lci` starts an interactive REPL
(read–eval–print loop), showing the `lci>`
prompt and waiting for user input. The most simple usage is to enter a
$\lambda$-term and press return. The program performs all $\beta$ and
$\eta$ reductions and generates the term's normal form.

The result is
printed in a "readable" way, that is only the necessary parentheses are
displayed, church numerals are displayed as integers and lists using the
standard Prolog notation. However the way terms are displayed can be
modified, for example `Set showpar on` causes all parentheses to be displayed.

Terms are reduced using the __normal order__ evaluation strategy, that is
the leftmost $\beta$ or $\eta$ reduction is performed first. This
strategy guarantees that term's normal form will be computed in finite
time, if it exists. However if a term has no normal form then execution
will not terminate. After the execution the program displays the number
of reductions that were performed and the CPU usage time.

Programs can be executed from a file by running

    lci <file.lci>

which causes `lci` to execute the term in `file.lci` and then exit.
A file can contain multiple terms, separated by `;`, in which case
they are all executed.
A file can be executed from the REPL by running `Consult 'file.lci'`
(including the quotes).

## Aliases

Aliases are used to give a name to a $\lambda$-term.
For example we can define

    I = \x.x

so that any occurrence of `I` within a term is replaced by $\lambda x.x$.
Aliases provided in a file (separated by `;`)
will be loaded with `Consult 'file.lci'`.

For commonly used aliases, `lci` automatically searches on start
for a file named `.lcirc` in the following locations:

    $CMAKE_INSTALL_PREFIX/share/lci/.lcirc  (eg. /usr/local/share/lci/.lcirc)
    $HOME/.lcirc
    ./.lcirc

All files found are executed in the above order. A sample
`.lcirc` is distributed with `lci`.

Aliases are replaced by the corresponding terms during __evaluation__
(not during parsing). Thus the order of the definitions is not
significant. If an alias is not defined an error message is displayed
during evaluation.

If no alias contains itself (directly or indirectly)
then aliases are just syntactic sugar, replacing them produces
a valid $\lambda$-terms. However `lci` supports curcular references
of aliases as a way to implement [recursion](#recursion).

## Integers

`lci` supports integers using an arbitrary (used-defined) encoding in
the pure calculus, provided by the following aliases:

    '0'       The encoding of 0
    Succ      Function mapping <N> to <N+1>
    Pred      Function mapping <N+1> to <N> and 0 to 0
    IsZero    Function mapping 0 to True and <N+1> to False.

An integer $n$ is parsed as `Succ (... (Succ 0))`, applying $n$ times
`Succ` to `0`.
The default encoding defined in `.lcirc` uses __Church numerals__,
which represents $n$ by:

$$\lambda f.\lambda x.f^n(x)$$

All standard arithmetic operators are supported (see [operators](#operators)).
An encoding-agnostic implementation is provided in `.lcirc` which
only depends on `0, Succ, Pred, IsZero`, as well as improved implementations for
Church numerals.

An alternative implementation using the Scott encoding is commented out in `.lcirc`,
you can simply uncomment to use it instead of the Church encoding.

Note that both Church and Scott encodings are recognized by the term printer
which will display the corresponding integer (unless the `readable` option is
`off`). Using a custom encoding is possible, but the printer will not recognize
the encoding so the terms will not be shown in readable form.

Finally note that the complexity of arithmetic  operations depends on the encoding and can
be high. For instance, the power-of operator (`**`) for the Church encoding requires an
exponential number of reductions.

## Lists

`lci` supports lists using an arbitrary (used-defined) encoding in
the pure calculus, provided by the following aliases:

    Nil    Encoding of an empty list
    Cons   Function constructing a list from its <head> and <tail>
    Head   Function returning the first element of a list
    Tail   Function returning a list with the first element removed
    IsNil  Function returning True if the list is empty

A term `[<T1>, ..., <Tn>]` is parsed as `Cons <T1> (... (Cons <tn> Nil))`.

A reference implementation is given in `.lcirc` in which `Cons` is defined
as a Church pair, and `Head, Tail` return the first and second element of the pair.
The term printer recognizes this encoding and displays lists in readable form.

## Recursion

Recursion is an essential programming tool provided by all serious
programming languages. `lci` supports two methods of implementing
recursion: using __infinite terms__ or using __fixed point combinators__.

### Recursion using infinite terms

If we allow an alias to contain itself then we can write terms like

$$M = \lambda x.M\ y$$

which essentially represents the "infinite" term:

$$\lambda x.(\lambda x.(\lambda x.(...)\ y)\ y)\ y$$


Although this is not a valid $\lambda$-term, we can explot the fact that,
if $M$ is __closed__ (contains no free variables), then
we can perform variable substitution as well as $\eta$-reduction
without the need to replace $M$ by its definition, since:

$$\begin{array}{lcl}
        M[x:=N] & \equiv & M \\
        (\lambda y.N)[x:=M] & \equiv & \lambda y.N[x:=M] \\
        \lambda x.M\ x & \rightarrow_\eta & M
\end{array}$$
    
Uring the evaluation of a term, $M$ must be
replaced only if we reach one of the following terms:

$$\begin{aligned}
    & M\ N \\
    & M
\end{aligned}$$

If the first case $M$ may contain an abstraction and in the second any
redex. If we expand an alias only when necessary, we can finish the
evaluation without performing all the replacements. For example the
abstraction $\lambda x.y$ does not use its argument, so the following
reduction

$$(\lambda x.y)\ M \rightarrow_\beta y$$

eliminates $M$ without computing it.

`lci` handles aliases using this techique, expanding them
only when necessary and only
once at a time, so we can reach a normal form even if a term is recursive.
`.lcirc` contains several such recursive definitions, eg for manipulating
list.

<!-- This technique is not compatible with the pure calculus, as it uses
invalid $\lambda$-terms. However the following must be noted: suppose
that in term $M = \lambda x.M\ x$ we need to replace $M$ only twice
until we reach it's normal form. This means that in term
$\lambda x.(\lambda x.(\lambda x.M\ y)\ y)\ y$ no replacement will be
performed. $M$. So we can substitute $M$ with an arbitary valid
$\lambda$-term $N$ and we get

$$M' = \lambda x.(\lambda x.(\lambda x.N\ y)\ y)\ y$$

$M'$ behaves
exactly like $M$ but it is a valid term. Of course in a different
situation more replacements could be needed, producing a different $M'$.
So $M$ could be considered as a "term generator" that produces an
appropriate $M'$ each time. -->

### Recursion using a fixed point combinator

An alternative to using infinite $\lambda$-terms is to implement
recursion in the pure $\lambda$-calculus. This can be achieved in an elegant
way using a __fixed point combinator__, that is a
term $Y$ such that

$$Y t \rightarrow t\ (Y t)$$

for any term $t$ (i.e. $Y t$ is a fixed point of $t$).

Now let $f=E$ be a recursive term that contains a copy of itself, that is $f$
appears inside $E$. Our goal is to transform it to a term $F$ that behaves similarly
to $f$ but which is not recursive.

We first
convert $f$ to a variable, obtaining the (non-recursive) term $\lambda f.E$
($f$ is now just a variable, not a copy of the whole term).
Then the term we are looking for is $F = Y (\lambda f.E)$ for which we have:

$$ F \rightarrow (\lambda f.E)\ F \rightarrow E[f:= F]. $$

In other words $F$ behaves similarly to $f$ (reduces to $E$ containing a copy of
itself), although $F$ itself is not recursive.

Things get a little bit more complicated if we have the following set of
mutually recursive terms

$$\begin{aligned}
    f_1 & = & E_1 \\
    & \vdots \\
    f_n & = & E_n
\end{aligned}$$

where any $f_i$ can be contained in any $E_j$. Now,
before applying $Y$, we must join all terms in one. This can be done
using the functions TUPLE $n$ and INDEX $k$. The former packages $n$
terms into a $n$-tuple, the latter returns the $k$-th element of a
tuple. We build the recursive term
$f = \textrm{TUPLE } n\ f_i \ \dots\ f_n$
and replace any occurences of terms $f_i$ with
$f_i = \textrm{INDEX}\ i\ f$.

Finally the non-recursive variant $F$ of $f$ is obtained using the fixed point combinator

$$F = Y\ (\lambda f.\textrm{TUPLE } n\ f_i \ \dots\ f_n )$$

`lci` provides the command `FixedPoint` which removes circular
references from aliases using the above procedure
(note that $Y$ must be defined in `.lcirc`).
The modified definition of recursive aliases can be displayed using the `ShowAlias`
command.

    lci> Length = \l.If (IsNil l) 0 (Succ (Length (Tail l)))

    lci> FixedPoint
    14 cycles removed using fixed point combinator Y.

    lci> ShowAlias Length
    Length = Y \_me.\l.If (IsNil l) 0 (Succ (_me (Tail l)))

In the above example `Length` is no longer recursive, but uses `Y` to obtain a copy
of itself in the variable `_me`.

## Operators

Operators is another tool that is provided by almost all programming
languages. `lci` supports operators as a special kind of function that
takes two arguments and syntactically appears between them. Using an
operator requires two steps. The first is it's *declaration* together
with it's *precedece* and *associativity*, in a way similar to Prolog.
This can be done with the command

    DefOp operator preced assoc

Quotes are necessary so that operator's name is recognized as an
alias. Precedence is an integer between 0 and 255 and is used
during parsing when no parentheses are present. Associativity takes one
of the following values:

    yfx  Left-associative operator
    xfy  Right-associative operator
    xfx  Non-associative operator

Character `x` corresponds to a term with lower precedence than the
operator, while `y` to one with higher or equal. Thus expression
`a+b+c*d` will be recognized as `(a+b)+(c*d)`, for operator `*` has
lower precedence than `+` (lower precedence operators are applied first)
and `+` is left-associative. Terms that
are not the result of an operator, or are enclosed in parentheses, are
considered to have precedence 0. Moreover application is considered as a
left-associative operator with precedence 100. So if an operator
$\texttt{\$}$ is declared with precedence 110 then the expression
`a b$c` will be recognized as `(a b)$c`.

The second step is operator's *definition* which is performed by
defining an alias with the same name:

    operator = ...

Operator definitions must be placed in a file (as all alias definitions)
and quotes are required. During parsing, `lci` replaces operators with
aliases, thas is expression `a+b` will be transformed to `’+’ a b`.
Now `+` is an alias, not an operator, and will be replaced with the
corresponding term during term's evaluation.

In `.lcirc` many common operators are declared and defined, mainly
concerning integers and list manipulation. These include the
right-associative operator `:` to write lists as `a:b:c:Nil`, operator
`++` to append lists, operator `,` to build ordered pairs `(a,b)`,
integer operations, integer comparisons etc.

## Evaluation strategies

An evaluation strategy determines the choice of a redex when there are
more than one in a term. `lci` uses the *normal order* strategy, which
selects term's leftmost redex. The main advantage of this strategy is
that it always leads to term's normal form, if it exists. However it has
a serious drawback which is the multiple computation of terms. For
example in the following series of reductions

$$\begin{aligned}
    && (\lambda f.f(f\ y))((\lambda x.x)(\lambda x.x)) \\
    & \rightarrow & (\lambda x.x)(\lambda x.x)((\lambda x.x)(\lambda x.x)y) \\
    & \rightarrow & (\lambda x.x)((\lambda x.x)(\lambda x.x)y) \\
    & \rightarrow & (\lambda x.x)(\lambda x.x)y \\
    & \rightarrow & (\lambda x.x)y \\
    & \rightarrow & y
\end{aligned}$$

the term $(\lambda x.x)(\lambda x.x)$ was computed
twice. An alternative strategy is *call-by-value*, in which all
arguments are computed before applied to a function. This method can
avoid multiple computation.

$$\begin{aligned}
    && (\lambda f.f(f\ y))((\lambda x.x)(\lambda x.x)) \\
    & \rightarrow & (\lambda f.f(f\ y))(\lambda x.x) \\
    & \rightarrow & (\lambda x.x)((\lambda x.x)y) \\
    & \rightarrow & (\lambda x.x)y \\
    & \rightarrow & y
\end{aligned}$$

This strategy, however, does not guarantee that normal
form will be found. There are also some other strategies like
*call-by-need* that is used in some functional languages like Haskell.

`lci` does not implement any such technique, but there has been an
effort to overcome this problem using a special operator $\sim$. This
operator does not behave like ordinary operators. The expression
$M\sim N$ denotes the application of $M$ to $N$ which, however, uses
call-by-value. So, if $M\sim N$ is the leftmost redex then all
reductions of $N$ are performed before the application. Thus the term
$(\lambda f.f(f\ y))\sim((\lambda x.x)(\lambda x.x))$ will be reduced
according to the second of the previous ways. Operator $\sim$ has the
same precedence and associativity as the application operator, so it can
be easily combined with it.

This operator, however, should be used with caution since the normal
form of $(\lambda x.y)\sim ((\lambda x.x\ x)(\lambda x.x\ x))$ will
never be found, yet it exists. In file `queens.lci` there is an
implementation of the well-known $n$-queens problem, using
experimentally this operator. Without the use of the operator the
program is impossible to terminate, even for 3 queens where the
combinations that must be examined are very few. This is due to the fact
that terms are extremely complex and cause a lot of recomputation. Using
the operator $\sim$ and testing in an Athlon 1800, all solutions for the
3 queens where found in 0.3 seconds, for 4 queens in 4.4 and for 5 in
190. For 6 queens after many hours of testing the program did not
terminate. This is not strange, though, since Haskell (with the same
implementation and using lazy-evaluation and constant time arithmetic)
needs 1799705 reductions for the 8 queens and extremely much time for
$n>12$.

## Tracing

`lci` supports evaluation tracing. This function is enabled using the
following command

    Set trace on

or pressing `Ctrl-C` during the evaluation of a term. When tracing is
enabled, the current term is displayed after each reduction and the
program waits for user input. Available commands are `step`, `continue`
and `abort`. The first one performs the next reduction, the second
continues the reductions without tracing and the last one stops the
evaluation. An alternative function is to display all intermediate terms
without interrupting the evaluation. This can be enabled using the
following command

    Set showexec on

## System commands

In previous paragraphs we have already mentioned some commands that are
supported by `lci`. These commands are functions that may have
arguments. If such a function is the leftmost in a term, then, instead
of evaluating the term, a system command is executed. All system
commands are described below:

    FixedPoint         Removes circular references from aliases using a fixed point combinator Y
    DefOp op prec ass  Declares an operator with the given precedence and associativity.
    ShowAlias [name]   Displays the definition of the given alias, or a lists of all aliases.
    Print term         Displays a term. Useful to check parsing.
    Consult file       Reads and processes the given file.
    Set option on/off  Changes one of the following parameters.
    
    Help               Displays a help message.
    Quit               Terminates the program.

## Examples

    lci> 3+5*2
    13

    lci> Sum 1..10
    55

    lci> Take 10 (Nats 5)
    [5, 6, 7, 8, 9, 10, 11, 12, 13, 14]

    lci> Map (Add 3) 1..5
    [4, 5, 6, 7, 8]

    lci> Map (\n.n**2) 1..5
    [I, 4, 9, 16, 25]

    lci> Filter (Leq 6) [3,6,10,11]
    [6, 10, 11]

    lci> Length 1..10 ++ [4,5]
    12

    lci> (Member 3 1..10) && (Length [3,4,5]) >= 3
    \x.\y.x

Note that term `I` (i.e. the identity function $\lambda y.y$) is the normal form of number 1. In file
`queens.lci` there is an implementation of $n$-queens problem.

    lci> Consult 'queens.lci'
    Successfully consulted queens.lci
    lci> Queens 4
    [[2, 4, I, 3], [3, I, 4, 2]]

All of the above functions can be also evaluated using the `FixedPoint`
command, which removes circular references using the fixed point
combinator $Y$. Using `ShowAlias` you can see an alias definition after
the modification.

    > FixedPoint
    > ShowAlias Sum
    Sum = Y \_me.\l.If (IsNil l) 0 (+ (Head l) (_me (Tail l)))
