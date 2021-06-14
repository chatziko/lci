### TODO

This is a roughly made TODO list based on some ideas to improve the program.

- Make parser handle errors more friendly.

- Implement call-by-need evaluation strategy (that is lazy evaluation) and support
  coexistence of the three strategies.

- Implement some basic operations (eg. integer ops) internally to speed up execution.
  - And/or try some faster (eg binary or ternary) encoding of numerals.

- Syntax highlighting of terms.

- Auto-completion.

- Add more debugging features (eg. breakpoints).
	
- Add some support for data types.

- Add input/output capabilities (maybe LCI could be used for scripting ?).

### DONE

- Replace the custom parser with the more advanced one using lex/yacc to
  implement some "syntactic conveniences".
  
- Use automake/autoconf, test on more systems.

- Use readline library for input (history, auto-complete, ...)
