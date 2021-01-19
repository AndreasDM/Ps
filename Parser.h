#ifndef PARSER_H
#define PARSER_H

#include <utility>
#include <optional>
#include <string>
#include <type_traits>
#include <functional>
#include <vector>

template <typename T>
using Thingy = std::optional<std::pair<T, std::string>>;

template <typename A>
struct Parser {
  Parser() = default;
  Parser(std::function<Thingy<A>(std::string)> f)
    : f{f} {}
  Thingy<A> operator()(std::string s) { return f(s); }

  std::function<Thingy<A>(std::string)> f;
  using type = A;
};

namespace ps {

template <typename A, typename F>
std::optional<std::result_of_t<F(A)>> fmap(F&& f, std::optional<A> const& a)
{
  if (a) {
    std::result_of_t<F(A)> ret{f(a.value())};
    return ret;
  }

  return {};
}

template <typename A, typename F, typename B>
std::optional<A> put(A a, std::optional<B> const&)
{
  return a;
}

template <typename A, typename F>
std::pair<std::result_of_t<F(A)>, std::string> fmap(F&& f, std::pair<A, std::string> const& a)
{
  return {f(a.first), a.second};
}

template <typename A, typename B>
std::pair<A, std::string> put(A a, std::pair<B, std::string> const& b)
{
  return {a, b.second};
}

template <typename A>
std::optional<A> pure(A a)
{
  return std::make_optional(std::move(a));
}

template <typename A>
std::pair<A, std::string> pure(A a)
{
  return {std::move(a), ""};
}

template <typename A, typename F>
std::optional<std::result_of_t<F(A)>> operator>>=(std::optional<A> const& a, F&& f)
{
  return fmap(std::forward<F>(f), a);
}

template <typename A, typename F>
auto operator>>=(std::pair<A, std::string> const& a, F&& f)
{
  return fmap(std::forward<F>(f), a);
}


template <typename T>
using ParseT = std::optional<std::pair<T, std::string>>;

template <typename A, typename F>
Parser<std::result_of_t<F(A)>> fmap(F&& f, Parser<A> const& a)
{
  return {[f = std::forward<F>(f),&a](std::string s) mutable {
    return fmap([f = std::forward<F>(f)](auto z) {
      return fmap([f = std::forward<F>(f)](auto h) {
        return f(h);
      }, z);
    }, a(s));
  }};
}

template <typename A>
Parser<A> lift (A something)
{
  return Parser<A>([something = std::move(something)](std::string s) {
    return std::make_optional(std::make_pair(std::move(something), std::move(s)));
  });
}

template <typename A>
Parser<A> fail (A)
{
  return Parser<A>([](std::string) {
    return Thingy<A>{};
  });
}

template <typename A, typename F>
Parser<A> condition (A a, F&& f)
{
  if (f(a))
    return lift(a);
  else
    return fail(a);
}

template <typename A, typename F>
auto operator >>= (Parser<A> a, F&& f)
{
  std::result_of_t<F(A)> ret {
    [a = std::move(a), f = std::forward<F>(f)](std::string s) mutable {
      auto x     = a(s);
      using type = typename std::result_of_t<F(A)>::type;
      if (x) {
        auto [first, second] = x.value();
        return f(first)(second);
      }
      else {
        return Thingy<type> {std::nullopt};
      }
    }
  };

  return ret;
}

template <typename A, typename B>
Parser<B> operator>>(Parser<A> a, Parser<B> b)
{
  return {
    [a = std::move(a), b = std::move(b)](std::string s) mutable {
      auto x = a(std::move(s));
      if (x) {
        auto [first,second] = x.value();
        return b(second);
      }
      else {
        return Thingy<B>{};
      }
    }
  };
}

inline static Parser<char> item {[](std::string const& input) -> ParseT<char> {
  if (!input.empty()) return {{input[0], input.substr(1)}};
  else                return {};
}};

inline static Parser<char> space = item >>= [](auto c) {
  return condition(c, [](auto x) { return std::isspace(x); });
};

inline static Parser<char> character = item >>= [](auto c) {
  if (std::isalpha(c))
    return lift(c);
  else
    return fail(c);
};

inline static Parser<char> manySpace {[](std::string input) -> ParseT<char> {
  auto s           = space(input);
  std::string last = input;
  while (s) {
    last = s.value().second;
    s    = space(s.value().second);
  }

  return std::make_optional(std::make_pair(' ', last));
}};

inline static Parser<int> digit = item >>= [](auto c) {
  int ret{};
  if (std::isdigit(c)) {
    ret = c - 48;
    return lift(ret);
  }
  return fail(ret);
};

inline static Parser<int> manyDigit {[](std::string input) -> ParseT<int> {
  auto s = digit(input);
  std::string ret;
  std::string last = input;
  while (s) {
    last  = s.value().second;
    ret  += std::to_string(s.value().first);
    s     = digit(s.value().second);
  }

  if (!ret.empty())
    return std::make_optional(std::make_pair(std::atoi(ret.c_str()), last));
  else
    return std::nullopt;
}};

template <typename A>
Parser<A> token (Parser<A> a)
{
  return manySpace >> std::move(a) >>= [](auto x) {
    return manySpace >>= [x](auto) {
      return lift(x);
    };
  };
}

inline static Parser<char> symbol(char c)
{
  return item >>= [c](auto x) {
    if (x == c)
      return lift(x);
    else
      return fail(x);
  };
}

inline static Parser<char> Character = token(character);
inline static Parser<int>  Digit     = token(digit);

inline static Parser<int> Int { [](std::string input) -> ParseT<int> {
  std::string last = input;
  bool negative    = false;

  if (auto a = token(symbol('-'))(input)) {
    negative = true;
    last     = a.value().second;
  }

  auto b = manyDigit(last);
  if (b) {
    if (negative)
      return std::make_optional(std::make_pair(-(b.value().first), b.value().second));
    else
      return std::make_optional(std::make_pair((b.value().first), b.value().second));
  }
  else
    return b;
}
};

inline static Parser<int> Integer = token(Int);

inline static Parser<char> Symbol(char c)
{
  return Parser<char>{[c](std::string s) {
    return token(symbol(c))(s);
  }};
}

inline static Parser<std::string> word {[](std::string input) -> ParseT<std::string> {
  std::string ret;
  while (auto x = character(input)) {
    ret += x.value().first;
    input = x.value().second;
  }

  if (!ret.empty())
    return std::make_optional(std::make_pair(ret, input));
  return {};
}};

inline static Parser<std::string> Word = token(word);

template <typename A>
std::optional<A> join(std::optional<std::optional<A>> const& a)
{
  if (a)
    return a.value();
  else
    return std::nullopt;
}

template <typename A>
Parser<A> operator | (Parser<A> a, Parser<A> b)
{
  return Parser<A>{[a = std::move(a), b = std::move(b)](std::string s) mutable {
    if (auto x = a(s))
      return x;
    else return b(s);
  }};
}

template <typename A>
Parser<std::vector<A>> many(Parser<A> a)
{
  return Parser<std::vector<A>>{[a = std::move(a)](std::string input) mutable {
    std::vector<A> result;
    Thingy<A> thing;
    while ((thing = a(input))) {
      result.push_back(thing.value().first);
      input = thing.value().second;
    }

    return std::make_optional(std::make_pair(std::move(result), input));
  }};
}

template <typename A>
Parser<std::vector<A>> some(Parser<A> a)
{
  return Parser<std::vector<A>>{[a = std::move(a)](std::string input) mutable {
    std::vector<A> result;
    while (Thingy<A> thing = a(input)) {
      result.push_back(thing.value().first);
      input = thing.value().second;
    }
    if (!result.empty())
      return std::make_optional(std::make_pair(std::move(result), input));

    return Thingy<std::vector<A>>{std::nullopt};
  }};
}

} // namespace ps

using ps::operator>>;
using ps::operator>>=;
using ps::operator|;

#endif // PARSER_H
