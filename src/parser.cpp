#include "compiler.h"

#include <cctype>
#include <climits>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {
struct Token { std::string text; int line; };

std::vector<Token> lex(const std::string& source) {
  std::vector<Token> tokens;
  size_t pos = 0;
  int line = 1;
  while (pos < source.size()) {
    char ch = source[pos];
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (ch == '\n') ++line;
      ++pos;
    } else if (ch == '{' || (ch == '(' && pos + 1 < source.size() && source[pos + 1] == '*')) {
      bool brace = ch == '{';
      pos += brace ? 1 : 2;
      while (pos < source.size() && !(brace ? source[pos] == '}' : source[pos] == '*' && pos + 1 < source.size() && source[pos + 1] == ')')) {
        if (source[pos++] == '\n') ++line;
      }
      if (pos == source.size()) throw std::runtime_error("line " + std::to_string(line) + ": unterminated comment");
      pos += brace ? 1 : 2;
    } else if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
      size_t start = pos++;
      while (pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_')) ++pos;
      std::string word = source.substr(start, pos - start);
      for (char& letter : word) letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
      tokens.push_back({word, line});
    } else if (std::isdigit(static_cast<unsigned char>(ch))) {
      size_t start = pos++;
      while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos]))) ++pos;
      // Real literal: digits '.' digits, avoiding '..' range tokens.
      if (pos < source.size() && source[pos] == '.' && !(pos + 1 < source.size() && source[pos + 1] == '.')) {
        ++pos;
        while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos]))) ++pos;
        if (pos < source.size() && (source[pos] == 'e' || source[pos] == 'E')) {
          ++pos;
          if (pos < source.size() && (source[pos] == '+' || source[pos] == '-')) ++pos;
          if (pos == source.size() || !std::isdigit(static_cast<unsigned char>(source[pos])))
            throw std::runtime_error("line " + std::to_string(line) + ": invalid real literal");
          while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos]))) ++pos;
        }
      }
      tokens.push_back({source.substr(start, pos - start), line});
    } else if (ch == '\'') {
      int startLine = line;
      size_t start = pos++;
      bool closed = false;
      while (pos < source.size()) {
        if (source[pos] == '\'') {
          ++pos;
          if (pos < source.size() && source[pos] == '\'') { ++pos; continue; }
          closed = true; break;
        }
        if (source[pos++] == '\n') ++line;
      }
      if (!closed) throw std::runtime_error("line " + std::to_string(startLine) + ": unterminated string");
      tokens.push_back({source.substr(start, pos - start), startLine});
    } else {
      std::string two = source.substr(pos, 2);
      if (two == ":=" || two == "<=" || two == ">=" || two == "<>" || two == "..") { tokens.push_back({two, line}); pos += 2; }
      else if (std::string(";:,.()[]+-*/=<>@").find(ch) != std::string::npos) { tokens.push_back({std::string(1, ch), line}); ++pos; }
      else throw std::runtime_error("line " + std::to_string(line) + ": invalid character");
    }
  }
  tokens.push_back({"<eof>", line});
  return tokens;
}

class Parser {
  std::vector<Token> tokens;
  std::function<Routine*(const std::string&)> loadUnit;
  size_t pos = 0;
  const Token& current() const { return tokens.at(pos); }
  bool is(const std::string& text) const { return current().text == text; }
  bool eat(const std::string& text) { if (!is(text)) return false; ++pos; return true; }
  [[noreturn]] void fail(const std::string& message) const { throw std::runtime_error("line " + std::to_string(current().line) + ": " + message + " (found '" + current().text + "')"); }
  void need(const std::string& text) { if (!eat(text)) fail("expected '" + text + "'"); }
  std::string ident() {
	    static const std::unordered_set<std::string> reserved = {"program", "unit", "interface", "implementation", "initialization", "finalization", "uses", "const", "type", "record", "class", "array", "of", "var", "begin", "end", "procedure", "function", "constructor", "destructor", "cdecl", "external", "forward", "if", "then", "else", "while", "do", "repeat", "until", "for", "to", "downto", "break", "continue", "exit", "try", "except", "finally", "raise", "div", "mod", "and", "or", "not", "true", "false", "nil", "write", "writeln", "readln"};
    std::string name = current().text;
    if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_') || reserved.count(name)) fail("expected identifier");
    ++pos; return name;
  }
  Type type(const Routine& routine, const std::string& typeBeingDefined = {}, const std::string& selfError = "a type alias cannot refer to itself") {
    std::string name = ident();
    if (eat(".")) {
      std::string typeName = ident();

      auto findUnit = [&](auto&& self, const Routine* scope) -> Routine* {
        if (scope && scope->name == name && scope->isUnit) return const_cast<Routine*>(scope);
        for (; scope; scope = scope->parent) {
          for (auto* unit : scope->imports) {
            if (unit->name == name) return unit;
            // Interface dependencies are visible for qualified names.
            for (size_t index = 0; index < unit->interfaceImportCount; ++index) {
              if (auto* found = self(self, unit->imports[index]); found) return found;
            }
          }
        }
        return nullptr;
      };

      auto* unit = findUnit(findUnit, &routine);
      if (!unit) fail("unknown unit '" + name + "'");
      Type imported;
      bool found = false;
      for (auto& record : unit->records) if (record->name == typeName) { found = true; imported = {record->isClass ? Type::Class : Type::Record, record.get()}; }
      for (auto& array : unit->arrays) if (array->name == typeName) { found = true; imported = {Type::Array, nullptr, array.get()}; }
      for (auto& callback : unit->callbacks) if (callback->name == typeName) { found = true; imported = {Type::Callback, nullptr, nullptr, callback.get()}; }
      for (auto& alias : unit->aliases) if (alias.name == typeName) { found = true; imported = alias.type; }
      if (!found || imported == Type::Void) fail("unknown type '" + typeName + "'");
      return imported;
    }
    if (name == typeBeingDefined) fail(selfError);
    for (auto scope = &routine; scope; scope = scope->parent) {
      for (auto& record : scope->records) if (record->name == name) return {record->isClass ? Type::Class : Type::Record, record.get()};
      for (auto& array : scope->arrays) if (array->name == name) return {Type::Array, nullptr, array.get()};
      for (auto& callback : scope->callbacks) if (callback->name == name) return {Type::Callback, nullptr, nullptr, callback.get()};
      for (auto& alias : scope->aliases) if (alias.name == name) return alias.type;
      for (auto& constant : scope->constants) if (constant.name == name) fail("unknown type '" + name + "'");
      for (auto& variable : scope->variables) if (variable->name == name) fail("unknown type '" + name + "'");
      for (auto& child : scope->routines) if (child->name == name) fail("unknown type '" + name + "'");

      std::unordered_set<const Routine*> seen;
      std::vector<const Routine*> units;
      auto visit = [&](auto&& self, const Routine* unit) -> void {
        if (!unit || !seen.insert(unit).second) return;
        units.push_back(unit);
        for (size_t index = 0; index < unit->interfaceImportCount && index < unit->imports.size(); ++index)
          self(self, unit->imports[index]);
      };
      for (auto* unit : scope->imports) visit(visit, unit);

      bool found = false;
      Type imported;
      for (auto* unit : units) {
        if (!unit->exports.count(name)) continue;
        bool matches = false;
        Type candidate;
        for (auto& record : unit->records) if (record->name == name) { matches = true; candidate = {record->isClass ? Type::Class : Type::Record, record.get()}; }
        for (auto& array : unit->arrays) if (array->name == name) { matches = true; candidate = {Type::Array, nullptr, array.get()}; }
        for (auto& callback : unit->callbacks) if (callback->name == name) { matches = true; candidate = {Type::Callback, nullptr, nullptr, callback.get()}; }
        for (auto& alias : unit->aliases) if (alias.name == name) { matches = true; candidate = alias.type; }
        for (auto& constant : unit->constants) if (constant.name == name) matches = true;
        for (auto& variable : unit->variables) if (variable->name == name) matches = true;
        for (auto& child : unit->routines) if (child->name == name) matches = true;
        if (matches) {
          if (found) fail("ambiguous imported identifier '" + name + "'");
          found = true; imported = candidate;
        }
      }
      if (found) {
        if (imported == Type::Void) fail("unknown type '" + name + "'");
        return imported;
      }
    }
    if (name == "integer" || name == "longint") return Type::Int32;
    if (name == "int64" || name == "nativeint") return Type::Int64;
    if (name == "real") return Type{Type::Real};
    if (name == "boolean") return Type::Boolean;
    if (name == "string") return Type::ManagedString;
    if (name == "pointer") return Type::Pointer;
    if (name == "pchar") return Type::PChar;
    fail("unknown type '" + name + "'");
  }
  std::int64_t arrayBound() {
    bool negative = eat("-");
    if (!negative) eat("+");
    if (!std::isdigit(static_cast<unsigned char>(current().text[0]))) fail("array bound must be an integer literal");
    std::string text = (negative ? "-" : "") + current().text; ++pos;
    try {
      auto value = std::stoll(text);
      if (value < INT32_MIN || value > INT32_MAX) fail("array bound out of range");
      return value;
    } catch (const std::out_of_range&) { fail("array bound out of range"); }
  }
  void parameters(const Routine& scope, std::vector<std::unique_ptr<Variable>>& variables) {
    if (!eat("(")) return;
    if (!is(")")) {
      do {
        int line = current().line;
        bool byReference = eat("var");
        std::vector<std::string> names{ident()};
        while (eat(",")) names.push_back(ident());
        need(":"); Type valueType = type(scope);
        for (const auto& name : names) {
          auto parameter = std::make_unique<Variable>(Variable{name, valueType, line});
          parameter->byReference = byReference;
          variables.push_back(std::move(parameter));
        }
      } while (eat(";"));
    }
    need(")");
  }
  void declarations(Routine& routine, bool prototypes = false, bool implementations = false) {
    if (eat("const")) {
      while (!is("type") && !is("var") && !is("begin") && !is("procedure") && !is("function") && !is("constructor") && !is("destructor") && !is("implementation") && !is("initialization") && !is("finalization") && !is("end")) {
        int line = current().line;
        std::string name = ident(); need("=");
        auto value = expression(); need(";");
        routine.constants.push_back({name, line, std::move(value)});
      }
    }
    if (eat("type")) {
      while (!is("var") && !is("begin") && !is("procedure") && !is("function") && !is("constructor") && !is("destructor") && !is("implementation") && !is("initialization") && !is("finalization") && !is("end")) {
        int declarationLine = current().line;
        std::string name = ident(); need("=");
        if (is("procedure") || is("function")) {
          bool function = eat("function");
          if (!function) need("procedure");
          auto callback = std::make_unique<CallbackType>();
          callback->name = name; callback->line = declarationLine;
          parameters(routine, callback->parameters);
          if (function) { need(":"); callback->result = type(routine, name); }
          need(";"); need("cdecl"); need(";");
          routine.callbacks.push_back(std::move(callback));
          continue;
        }
        if (eat("array")) {
          need("["); auto lower = arrayBound(); need(".."); auto upper = arrayBound(); need("]"); need("of");
          if (upper < lower || upper - lower >= INT32_MAX) fail("invalid array bounds");
          Type element = type(routine, name, "an array cannot contain itself by value"); need(";");
          routine.arrays.push_back(std::make_unique<ArrayType>(ArrayType{name, declarationLine, lower, upper, element}));
          continue;
        }
        bool isClass = eat("class");
        if (!isClass && !eat("record")) {
          Type target = type(routine, name); need(";");
          routine.aliases.push_back({name, target, declarationLine});
          continue;
        }
        auto record = std::make_unique<RecordType>();
        record->line = declarationLine; record->name = name; record->isClass = isClass;
        if (isClass && eat("(")) {
          Type base = type(routine, name, "a class cannot inherit from itself"); need(")");
          if (base.kind != Type::Class) fail("class base must be a class type");
          record->base = base.record;
        }
        RecordType* definition = record.get();
        if (isClass) routine.records.push_back(std::move(record));
        while (!is("end")) {
          if (isClass && (is("procedure") || is("function") || is("constructor") || is("destructor"))) {
            bool function = eat("function");
            bool constructor = !function && eat("constructor");
            bool destructor = !function && !constructor && eat("destructor");
            if (!function && !constructor && !destructor) need("procedure");
            auto method = std::make_unique<Routine>();
            method->constructor = constructor; method->destructor = destructor;
            method->line = current().line; method->name = ident(); method->parent = &routine;
            method->methodOwner = definition; method->prototype = true;
            method->variables.push_back(std::make_unique<Variable>(Variable{"self", {Type::Class, definition}, method->line}));
            parameters(routine, method->variables);
            method->parameterCount = method->variables.size();
            if (function) { need(":"); method->result = type(routine); }
            need(";");
            if (eat("virtual")) { method->virtualMethod = true; need(";"); }
            else if (eat("override")) { method->overrideMethod = true; need(";"); }
            definition->methods.push_back(std::move(method));
            continue;
          }
          int line = current().line;
          std::vector<std::string> names{ident()};
          while (eat(",")) names.push_back(ident());
          need(":"); Type fieldType = isClass ? type(routine) : type(routine, definition->name, "a record cannot contain itself by value");
          for (const auto& name : names) definition->fields.push_back(std::make_unique<Field>(Field{name, fieldType, line}));
          if (!eat(";")) break;
        }
        need("end"); need(";"); if (!isClass) routine.records.push_back(std::move(record));
      }
    }
    if (eat("var")) {
      while (!is("begin") && !is("procedure") && !is("function") && !is("constructor") && !is("destructor") && !is("implementation") && !is("initialization") && !is("finalization") && !is("end")) {
        int line = current().line;
        std::vector<std::string> names{ident()};
        while (eat(",")) names.push_back(ident());
        need(":"); Type valueType = type(routine); need(";");
        for (const auto& name : names) routine.variables.push_back(std::make_unique<Variable>(Variable{name, valueType, line}));
      }
    }
    while (is("procedure") || is("function") || is("constructor") || is("destructor")) {
      bool function = eat("function");
      bool constructor = !function && eat("constructor");
      bool destructor = !function && !constructor && eat("destructor");
      if (!function && !constructor && !destructor) need("procedure");
      auto child = std::make_unique<Routine>();
      child->constructor = constructor; child->destructor = destructor;
      child->line = current().line; child->name = ident(); child->parent = &routine;
      if (eat(".")) {
        std::string className = child->name;
        for (auto& record : routine.records) if (record->isClass && record->name == className) { child->methodOwner = record.get(); break; }
        if (!child->methodOwner) fail("method definition requires a class declared in this scope");
        child->name = ident();
        child->variables.push_back(std::make_unique<Variable>(Variable{"self", {Type::Class, child->methodOwner}, child->line}));
      }
      if ((constructor || destructor) && !child->methodOwner) fail("constructor or destructor must be a class method");
      parameters(routine, child->variables);
      child->parameterCount = child->variables.size();
      if (function) { need(":"); child->result = type(routine); }
      need(";");
      if (eat("cdecl")) {
        if (child->methodOwner) fail("class methods cannot be external");
        need(";"); need("external"); child->external = true; child->externalName = child->name;
        if (eat("name")) {
          std::string literal = current().text;
          if (literal.size() < 2 || literal.front() != '\'') fail("expected external symbol string");
          ++pos; child->externalName = literal.substr(1, literal.size() - 2);
        }
        need(";");
      } else if (eat("forward")) {
        child->prototype = true;
        need(";");
      } else if (prototypes) {
        child->prototype = true;
      } else {
        declarations(*child); if (!is("begin")) fail("expected 'begin'"); child->body = statement(); need(";");
      }
      bool replaced = false;
      auto& declarations = child->methodOwner ? child->methodOwner->methods : routine.routines;
      if (implementations || child->methodOwner || !child->prototype) for (auto& existing : declarations) {
        if (existing->name != child->name || !existing->prototype) continue;
        if (existing->result != child->result || existing->parameterCount != child->parameterCount || existing->constructor != child->constructor || existing->destructor != child->destructor) fail("unit routine signature mismatch '" + child->name + "'");
        for (size_t index = 0; index < child->parameterCount; ++index)
          if (existing->variables[index]->type != child->variables[index]->type || existing->variables[index]->byReference != child->variables[index]->byReference)
            fail("unit routine signature mismatch '" + child->name + "'");
        child->virtualMethod = existing->virtualMethod;
        child->overrideMethod = existing->overrideMethod;
        existing = std::move(child); replaced = true; break;
      }
      if (!replaced) {
        if (child->methodOwner) fail("class method was not declared");
        declarations.push_back(std::move(child));
      }
    }
    if (!prototypes) for (auto& record : routine.records)
      for (auto& method : record->methods) if (method->prototype) fail("missing implementation for class method '" + method->name + "'");
    if (!prototypes) for (auto& child : routine.routines)
      if (child->prototype) fail("missing implementation for routine '" + child->name + "'");
  }
  std::unique_ptr<Expr> node(Expr::Kind kind, const Token& token) { return std::make_unique<Expr>(kind, token.text, token.line); }
  std::vector<Selector> selectors() {
    std::vector<Selector> result;
    while (is(".") || is("[")) {
      int line = current().line;
      if (eat(".")) result.push_back({ident(), line, nullptr});
      else { need("["); auto index = expression(); need("]"); result.push_back({{}, line, std::move(index)}); }
    }
    return result;
  }
  std::vector<std::unique_ptr<Expr>> arguments() {
    std::vector<std::unique_ptr<Expr>> result;
    if (eat("(")) { if (!is(")")) { do { result.push_back(expression()); } while (eat(",")); } need(")"); }
    return result;
  }
  std::unique_ptr<Expr> primary() {
    if (eat("(")) { auto result = expression(); need(")"); return result; }
    Token token = current();
    if (eat("@")) { Token name = current(); name.text = ident(); return node(Expr::Kind::RoutineAddress, name); }
    if (eat("-")) {
      if (std::isdigit(static_cast<unsigned char>(current().text[0]))) { Token number = current(); ++pos; number.text = "-" + number.text; return node(Expr::Kind::Integer, number); }
      auto result = node(Expr::Kind::Unary, token); result->operands.push_back(primary()); return result;
    }
    if (eat("+") || eat("not")) { auto result = node(Expr::Kind::Unary, token); result->operands.push_back(primary()); return result; }
    if (std::isdigit(static_cast<unsigned char>(token.text[0]))) {
      ++pos;
      return node(token.text.find_first_of(".eE") == std::string::npos ? Expr::Kind::Integer : Expr::Kind::Real, token);
    }
    if (eat("true") || eat("false")) return node(Expr::Kind::Boolean, token);
    if (eat("nil")) return node(Expr::Kind::Nil, token);
    if (token.text[0] == '\'') { ++pos; return node(Expr::Kind::String, token); }
    ident();
    auto result = node(is("(") ? Expr::Kind::Call : Expr::Kind::Name, token);
    if (result->kind == Expr::Kind::Call) {
      result->operands = arguments();
      result->selectors = selectors();
    }
    else {
      result->selectors = selectors();
      if (!result->selectors.empty() && is("(")) { result->kind = Expr::Kind::MemberCall; result->operands = arguments(); }
    }
    return result;
  }
  std::unique_ptr<Expr> binary(int level) {
    if (level == 3) return primary();
    auto left = binary(level + 1);
    while ((level == 0 && (is("=") || is("<>") || is("<") || is(">") || is("<=") || is(">="))) ||
           (level == 1 && (is("+") || is("-") || is("or"))) ||
           (level == 2 && (is("*") || is("/") || is("div") || is("mod") || is("and")))) {
      Token token = current(); ++pos; auto result = node(Expr::Kind::Binary, token);
      result->operands.push_back(std::move(left)); result->operands.push_back(binary(level + 1)); left = std::move(result);
      if (level == 0) break;
    }
    return left;
  }
  std::unique_ptr<Expr> expression() { return binary(0); }
  std::unique_ptr<Stmt> statement() {
    Token token = current();
    auto result = std::make_unique<Stmt>(Stmt::Kind::Empty, token.text, token.line);
    if (eat("begin")) {
      result->kind = Stmt::Kind::Block;
      if (!is("end")) { result->statements.push_back(statement()); while (eat(";")) { if (is("end")) break; result->statements.push_back(statement()); } }
      need("end");
	    } else if (eat("write") || eat("writeln")) {
	      result->kind = Stmt::Kind::Write; result->expressions = arguments();
	    } else if (eat("readln")) {
	      result->kind = Stmt::Kind::ReadLn; result->expressions = arguments();
	    } else if (eat("if")) {
      result->kind = Stmt::Kind::If; result->expressions.push_back(expression()); need("then"); result->statements.push_back(statement());
      if (eat("else")) result->statements.push_back(statement());
    } else if (eat("while")) {
      result->kind = Stmt::Kind::While; result->expressions.push_back(expression()); need("do"); result->statements.push_back(statement());
    } else if (eat("repeat")) {
      result->kind = Stmt::Kind::Repeat;
      if (!is("until")) {
        result->statements.push_back(statement());
        while (eat(";")) { if (is("until")) break; result->statements.push_back(statement()); }
      }
      need("until"); result->expressions.push_back(expression());
    } else if (eat("break")) result->kind = Stmt::Kind::Break;
    else if (eat("continue")) result->kind = Stmt::Kind::Continue;
    else if (eat("exit")) {
      result->kind = Stmt::Kind::Exit;
      if (eat("(")) {
        result->expressions.push_back(expression());
        need(")");
      }
    }
    else if (eat("raise")) result->kind = Stmt::Kind::Raise;
    else if (eat("try")) {
      auto parseStatementList = [&](const auto& isTerminator) {
        auto block = std::make_unique<Stmt>(Stmt::Kind::Block, "", token.line);
        if (!isTerminator()) {
          block->statements.push_back(statement());
          while (eat(";")) {
            if (isTerminator()) break;
            block->statements.push_back(statement());
          }
        }
        return block;
      };

      auto body = parseStatementList([&] { return is("except") || is("finally"); });
      if (eat("except")) {
        result->kind = Stmt::Kind::Try;
        auto handler = parseStatementList([&] { return is("end"); });
        result->statements.push_back(std::move(body));
        result->statements.push_back(std::move(handler));
        need("end");
      } else {
        need("finally");
        result->kind = Stmt::Kind::TryFinally;
        auto handler = parseStatementList([&] { return is("end"); });
        result->statements.push_back(std::move(body));
        result->statements.push_back(std::move(handler));
        need("end");
      }
    }
    else if (eat("for")) {
      result->kind = Stmt::Kind::For; result->text = ident(); result->selectors = selectors(); need(":="); result->expressions.push_back(expression());
      if (eat("to")) result->expressions.push_back(node(Expr::Kind::Boolean, Token{"false", token.line}));
      else if (eat("downto")) result->expressions.push_back(node(Expr::Kind::Boolean, Token{"true", token.line}));
      else fail("expected 'to' or 'downto'");
      result->expressions.push_back(expression()); need("do"); result->statements.push_back(statement());
    } else if (!is(";") && !is("end") && !is("else") && !is("until") && !is(".")) {
      result->text = ident();
      result->selectors = selectors();
      if (eat(":=")) { result->kind = Stmt::Kind::Assign; result->expressions.push_back(expression()); }
      else if (!result->selectors.empty() && !result->selectors.back().index && result->selectors.back().name == "free") {
        result->selectors.pop_back(); result->kind = Stmt::Kind::Destroy; result->expressions = arguments();
      } else { result->kind = result->selectors.empty() ? Stmt::Kind::Call : Stmt::Kind::MethodCall; result->expressions = arguments(); }
    }
    return result;
  }
public:
  Parser(const std::string& source, std::function<Routine*(const std::string&)> loader) : tokens(lex(source)), loadUnit(std::move(loader)) {}
  void uses(Routine& scope) {
    if (!eat("uses")) return;
    std::unordered_set<std::string> names;
    do {
      auto name = ident();
      if (!names.insert(name).second) fail("duplicate unit '" + name + "'");
      for (auto* imported : scope.imports) if (imported->name == name) fail("duplicate unit '" + name + "'");
      scope.imports.push_back(loadUnit(name));
    } while (eat(","));
    need(";");
  }
  void program(Routine& program) {
    need("program"); program.name = ident(); need(";"); uses(program);
    declarations(program); if (!is("begin")) fail("expected 'begin'"); program.body = statement(); need("."); if (!is("<eof>")) fail("unexpected trailing tokens");
  }
  void unit(Routine& unit, const std::string& expected) {
    need("unit"); unit.name = ident(); unit.isUnit = true;
    if (unit.name != expected) fail("unit name does not match '" + expected + "'");
    need(";"); need("interface"); uses(unit); declarations(unit, true);
    for (auto& record : unit.records) unit.exports.insert(record->name);
    for (auto& array : unit.arrays) unit.exports.insert(array->name);
    for (auto& callback : unit.callbacks) unit.exports.insert(callback->name);
    for (auto& alias : unit.aliases) unit.exports.insert(alias.name);
    for (auto& constant : unit.constants) unit.exports.insert(constant.name);
    for (auto& variable : unit.variables) unit.exports.insert(variable->name);
    for (auto& routine : unit.routines) unit.exports.insert(routine->name);
    unit.interfaceConstantCount = unit.constants.size();
    unit.interfaceImportCount = unit.imports.size();
    need("implementation"); uses(unit); declarations(unit, false, true);
    for (auto& routine : unit.routines) if (routine->prototype) fail("missing implementation for '" + routine->name + "'");
    if (eat("initialization")) unit.body = unitStatements();
    else if (is("begin")) unit.body = statement();
    if (eat("finalization")) unit.finalization = unitStatements();
    if (!unit.body && !unit.finalization) need("end");
    else if (unit.finalization || !is(".")) need("end");
    need(".");
    if (!is("<eof>")) fail("unexpected trailing tokens");
  }
  std::unique_ptr<Stmt> unitStatements() {
    auto block = std::make_unique<Stmt>(Stmt::Kind::Block, "", current().line);
    while (!is("end") && !is("finalization")) {
      if (is("<eof>")) fail("expected end of unit");
      block->statements.push_back(statement());
      if (!eat(";") && !is("end") && !is("finalization")) fail("expected ';'");
    }
    return block;
  }
};
}
std::unique_ptr<Routine> parse(const std::string& source, const std::string& sourcePath, const std::vector<std::string>& unitPaths) {
  auto program = std::make_unique<Routine>();
  program->sourcePath = std::filesystem::absolute(sourcePath).string();
  std::vector<std::filesystem::path> paths{std::filesystem::path(sourcePath).parent_path()};
  if (paths[0].empty()) paths[0] = ".";
  for (const auto& path : unitPaths) paths.emplace_back(path);
  std::unordered_map<std::string, Routine*> loaded;
  std::unordered_set<std::string> loading;
  std::function<Routine*(const std::string&)> load;
  load = [&](const std::string& name) -> Routine* {
    if (auto found = loaded.find(name); found != loaded.end()) return found->second;
    if (!loading.insert(name).second) throw std::runtime_error("circular unit dependency involving '" + name + "'");
    std::filesystem::path filePath;
    for (const auto& path : paths) {
      std::error_code errorCode;
      for (const auto& entry : std::filesystem::directory_iterator(path, errorCode)) {
        std::string filename = entry.path().filename().string();
        for (char& letter : filename) letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
        if (filename == name + ".pas") {
          if (!filePath.empty()) throw std::runtime_error("ambiguous unit file for '" + name + "'");
          filePath = entry.path();
        }
      }
      if (!filePath.empty()) break;
    }
    if (filePath.empty()) throw std::runtime_error("cannot find unit '" + name + "'");
    std::ifstream file(filePath);
    if (!file) throw std::runtime_error("cannot open unit file " + filePath.string());
    std::stringstream text; text << file.rdbuf();
    auto unit = std::make_unique<Routine>();
    unit->sourcePath = std::filesystem::absolute(filePath).string();
    try { Parser(text.str(), load).unit(*unit, name); }
    catch (const std::exception& error) { throw std::runtime_error("unit " + filePath.string() + ": " + error.what()); }
    auto* result = unit.get();
    program->units.push_back(std::move(unit));
    loaded.emplace(name, result); loading.erase(name);
    return result;
  };
  Parser(source, load).program(*program);
  return program;
}
