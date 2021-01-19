#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "../Parser.h"
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <iostream>

struct Json {
  using json_array  = std::vector<Json>;
  using json_object = std::map<std::string, Json>;
  std::variant<int, bool, double, std::string, void*, json_array, json_object> data;
};

namespace json_parser {

inline static Parser<Json> Fraction = ps::Int >>= [](auto i) {
  return ps::token(ps::symbol('.') >>= [i](auto) {
    return ps::Int >>= [i](auto f) {
      double ret = i;
      ret += std::pow(10, -(long(std::log10(f) + 1))) * f;
      return ps::lift(Json{ret});
    };
  });
};

inline static Parser<Json> json_fraction = ps::token(Fraction);

inline static Parser<char> Json_char = (ps::item >>= [](char c) {
  if (c != '"' && c != '\\' && !std::iscntrl(c))
    return ps::lift(c);
  return ps::fail(c);
}) | (ps::symbol('\\') >>= [](auto) {
       return (ps::symbol('n') >>= [](auto) { return ps::lift('\n'); })
       | (ps::symbol('b') >>= [](auto) { return ps::lift('\b'); })
       | (ps::symbol('f') >>= [](auto) { return ps::lift('\f'); })
       | (ps::symbol('r') >>= [](auto) { return ps::lift('\r'); })
       | (ps::symbol('t') >>= [](auto) { return ps::lift('\t'); })
       | (ps::symbol('"') >>= [](auto) { return ps::lift('\"'); });
});

inline static Parser<Json> Json_string =
  ps::symbol('"') >> ps::many(Json_char) >>= [](std::vector<char> const& v) {
    return ps::symbol('"') >>= [&v](char) {
      std::string ret{v.begin(), v.end()};
      return ps::lift(Json{ret});
    };
  };

inline static Parser<Json> json_string = ps::token(Json_string);

inline static Parser<Json> json_int = ps::Integer >>= [](auto i) {
  return ps::lift(Json{i});
};

inline static Parser<Json> json_true
  =  ps::symbol('t')
  >> ps::symbol('r')
  >> ps::symbol('u')
  >> ps::symbol('e') >>= [](auto) {
    return ps::lift(Json{true});
  };

inline static Parser<Json> json_false
  =  ps::symbol('f')
  >> ps::symbol('a')
  >> ps::symbol('l')
  >> ps::symbol('s')
  >> ps::symbol('e') >>= [](auto) {
    return ps::lift(Json{false});
  };

inline static Parser<Json> json_null
  =  ps::symbol('n')
  >> ps::symbol('u')
  >> ps::symbol('l')
  >> ps::symbol('l') >>= [](auto) {
    return ps::lift(Json{nullptr});
  };

struct Proxy {
  Proxy(Parser<Json>(*f)())
    : f{f}
  {}

  Parser<Json> operator()()
  {
    return f();
  }

  Parser<Json>(*f)();
};

inline static Parser<Json> operator | (Proxy a, Proxy b)
{
  return Parser<Json>{[a = std::move(a),b = std::move(b)](std::string const& s) mutable {
    if (auto x = a()(s)) return x;
    else                 return b()(s);
  }};
}

inline static Parser<Json> operator | (Parser<Json> a, Proxy b)
{
  return Parser<Json>{[a = std::move(a), b = std::move(b)](std::string const& s) mutable {
    if (auto x = a(s)) return x;
    else               return b()(s);
  }};
}

inline static Parser<Json> object_parser();
inline static Parser<Json> objects_parser();
inline static Parser<Json> array_parser();
inline static Parser<Json> value()
{
  return json_fraction | json_int | json_true | json_false | json_null |
         json_string | Proxy{ objects_parser } | Proxy{ array_parser };
}

inline static Parser<Json> element() { return value(); }

inline static Parser<Json> elements()
{
  return (element() >>= [](auto const& x) {
    return ps::Symbol(',') >>= [&](auto) {
      return ps::lift(x);
    };
  }) | element();
}

inline static Parser<Json> array_parser()
{
  return ps::Symbol('[') >> (ps::many(elements())) >>= [](auto elem) {
    return ps::Symbol(']') >>= [elem = std::move(elem)](auto) {
      return ps::lift(Json{std::move(elem)});
    };
  };
}

inline static Parser<Json> object_parser()
{
  return (json_string >>= [&](auto const& name) {
    return ps::Symbol(':') >> (element()) >>= [&name](auto elem) {
      return ps::Symbol(',') >>= [&name, elem = std::move(elem)](auto) {
        std::map<std::string, Json> ret;
        ret[std::get<std::string>(name.data)] = std::move(elem);
        return ps::lift(Json{std::move(ret)});
      };
    };
  }) | (json_string >>= [&](auto const& name) {
    return ps::Symbol(':') >> (element()) >>= [&name](auto elem) {
      std::map<std::string, Json> ret;
      ret[std::get<std::string>(name.data)] = std::move(elem);
      return ps::lift(Json{std::move(ret)});
    };
  });
}

inline static Parser<Json> objects_parser()
{
  return ps::Symbol('{') >> ps::many(object_parser()) >>= [](auto xs) {
    return ps::Symbol('}') >>= [xs = std::move(xs)](auto) {
      return ps::lift(Json{std::move(xs)});
    };
  };
}

inline static Parser<Json> parse = value();

template <typename... Xs> struct visitors : Xs... { using Xs::operator()...; };
template <typename... Xs> visitors(Xs...) -> visitors<Xs...>;

inline static void print(Json const& json)
{
  std::visit(visitors{
    [](int const i) { std::cout << i; },
    [](double const d) { std::cout << std::setprecision(16) << d; },
    [](bool const b) { std::cout << std::boolalpha << b; },
    [](std::string const& s) { std::cout << '"' << s << '"'; },
    [](void*) { std::cout << "null"; },
    [](std::vector<Json> const& v) {
      std::cout << '[';
      for (std::size_t i = 0; i < v.size(); ++i) {
        print(v[i]);
        if (i != v.size() - 1)
          std::cout << ',';
      }
      std::cout << ']';
    },
    [](std::map<std::string, Json> const& m) {
      for (auto beg = m.begin(); beg != m.end(); ++beg) {
        std::cout << "{ \"" << beg->first << "\": ";
        print(beg->second);
        std::cout << " }";

        if (auto temp = beg; ++temp != m.end())
          std::cout << ',';
      }
    }
  },json.data);
}

}

#endif // JSONPARSER_H
