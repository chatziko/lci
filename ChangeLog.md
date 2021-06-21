## LCI

Real men use only pure calculus

- v? (in progress)
  - Add webassembly/emscripten support for running lci in a browser.
  - Add auto-completion of aliases and system commands.
  - Add 'let x = N in M' syntax, syntactic sugar for (\x.M) N.

- v1.0 (15 Jun 2021)

  First major release, 18 years in the making :smile: 

  - Replace custom parser with [dparser](https://github.com/jplevyak/dparser) (much easier to extend).
  - Use sting interning, gives 2x speedup (thanks to @baziotis).
    We can now solve 8-queens in the pure calculus.
  - Support `[a,b,c]` list notation in parser (syntactic sugar for `a:b:c:Nil`).
  - Migrate from autotools to cmake.
  - Replace readline by [replxx](https://github.com/AmokHuginnsson/replxx) (no external dependencies, windows support).
  - Use same syntax in files and REPL (eg aliases can be defined in the REPL).
  - Print `\x.x` as `I` (looks good since `1` reduces to `I`).
  - Various polishing, cleanup and performance enhancements.
  - Add simple tests, run them in gh-actions.

- v0.6 (14 Feb 2008)

  This is a bugfix release which corrects a bug (#1546641) in alpha-conversion. The bug
  was affecting the power operator (**) giving a wrong result in some cases.

- v0.5 (13 Mar 2006)

  First 'official' release of lci, the code was there for some time but
  I wanted to add an autotooled build system and some cool extra features.

  So we now use autotools to build, type `./configure && make && make install`
  and you're done. Tested in linux, win32 (cygwin) and OSX, it compiles like a charm :)

  There is also support for readline now so if you have it installed when
  compiling you'll be able to edit the lines that you type, have history, etc.

  Finally some impressive optimizations have been done in this version.
  Queens 5 takes just 2 seconds instead of 50 and we can even solve Queens 6
  in 40 seconds which was unsolvable before.

  Enjoy!

