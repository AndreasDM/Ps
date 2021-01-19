#include "../Parser.h"

#include <iostream>
#include <string>

using namespace ps;

extern Parser<int> expr;
Parser<int> parenExpr = (Symbol('(') >>= [](auto) {
  return expr >>= [](auto i) {
    return Symbol(')') >>= [i](auto) {
      return lift(i);
    };
  };
}) | Integer;

Parser<int> factor = parenExpr | Integer;

Parser<int> term = (factor >>= [](auto x) {
  return (Symbol('*') >>= [x](auto) {
    return term >>= [x](auto y) {
      return lift(x * y);
    };
  }) | (Symbol('/') >>= [x](auto) {
    return term >>= [x](auto y) {
      return lift(x / y);
    };
  });
}) | factor;

Parser<int> expr = (term >>= [](auto x) {
  return (Symbol('+') >>= [x](auto) {
    return expr >>= [x](auto y) {
      return lift(x + y);
    };
  }) | (Symbol('-') >>= [x](auto) {
    return expr >>= [x](auto y) {
      return lift(x - y);
    };
  });
}) | term;

int main()
{
  auto print = [](auto x) {
    std::cout << x.first << '\n';
    return x;
  };

  // monad laws:
  // left identity
  auto foo = lift(Word) >>= [](auto x) {
    return x >>= [x](auto y) {
      return lift(y);
    };
  };
  // right identity
  auto bar = Word >>= [](auto x) {
    return lift(x);
  };

  // associativity
  auto zyz = (Digit >>= [](auto) { return Character; }) >>= [](auto){ return Digit; };
  auto yzy =  Digit >>= [](auto) { return Character     >>= [](auto){ return Digit; }; };

  expr("(3 * (-1 - -3))/2") >>= print;
  expr("5 + 3 * 2")         >>= print;
}

