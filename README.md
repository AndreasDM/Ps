# Haskell-like parser combinators for C++

This is a small header-only library that allows for combining parsers,
haskell-style.
It comes with an expression parser, a Json parser, and some example code.

The Json parser is not fully compliant; specifically, parsing exponents and
`\uxxx` were the things that I didn't bother doing.


## To run an example when in an example folder:

```makefile
make run
```

## Parsers

The basic parsers are sitting in the namespace `ps`; the ones starting with a
capital letter are already tokenized.

Here are some of the parsers and the utilities:

* `Symbol(char c)`
* `Integer`
* `Character`
* `Digit`
* `token(Parser<T>)` (takes care of whitespace on both side of a parser)
* `Word`
* `space`
* `many(Parser<T>)`
* `some(Parser<T>)`
* `lift(T) -> Parser<T>`
* `fail(T) -> Parser<T>`

## Operators

Operators `>>`, `>>=` and `|` are overloaded.

`operator>>=` binds a function to the result of a parser if it succeeds, and it
expects that a parser is returned (nasty compiler errors otherwise). This lets
you compose parsers and take care of the result however you like.

`operator|` selects the succeeding parser, if any.

`operator>>` compose parsers.

## Running a parser

The parser expects a string, and if bound to a function, it will return a pair [parsed, rest].
If not bound to a function, it's an `optional<pair<T, string>>`:

```cpp
Integer("1234") >>= [](auto x) {
  cout << x.first;
  return x;
};
```

```cpp
if (auto x = Integer("1234"); x)
  cout << x->first;
```

## Creating a parser

Use the operators to compose parsers:

```cpp
// To parse ':' immediately followed by a digit, then a ':'
Parser<int> p =
  ps::symbol(':') >> ps::digit >>= [](int i) {
    return ps::symbol(':')     >>= [i](char) {
      return ps::lift(i);
    };
  };
```

To tokenize the parser:

```cpp
Parser<int> P = ps::token(p);
```

