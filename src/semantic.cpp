#include "compiler.h"

#include <climits>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {
[[noreturn]] void error(int line, const std::string& message) { throw std::runtime_error("line " + std::to_string(line) + ": " + message); }
bool integer(Type type) { return type == Type::Int32 || type == Type::Int64; }
bool real(Type type) { return type.kind == Type::Real; }
bool pointer(Type type) { return type == Type::Pointer || type == Type::PChar; }
bool stringType(Type type) { return type == Type::String || type == Type::ManagedString; }
bool cValue(Type type) { return integer(type) || real(type) || pointer(type); }

std::string decodeStringLiteral(const std::string& literal) {
  std::string value;
  for (size_t index = 1; index + 1 < literal.size(); ++index) {
    if (literal[index] == '\'' && index + 1 < literal.size() - 1 && literal[index + 1] == '\'') ++index;
    value += literal[index];
  }
  return value;
}

std::string encodeStringLiteral(const std::string& value) {
  std::string result;
  result += "'";
  for (char ch : value) {
    if (ch == '\'') result += "''";
    else result += ch;
  }
  result += "'";
  return result;
}

std::string encodeRealLiteral(double value) {
  if (value == 0.0) value = 0.0;
  std::ostringstream out;
  out << std::setprecision(17) << std::showpoint << value;
  return out.str();
}
bool classAssignable(Type destination, Type source) {
  if (destination.kind != Type::Class || source.kind != Type::Class) return false;
  for (auto* record = source.record; record; record = record->base) if (record == destination.record) return true;
  return false;
}
bool assignable(Type destination, Type source) {
  return destination == source || (destination == Type::Int64 && source == Type::Int32) ||
         (destination.kind == Type::Real && integer(source)) ||
         ((pointer(destination) || destination.kind == Type::Callback || destination.kind == Type::Class) && source == Type::Nil) || (destination == Type::PChar && source == Type::String) ||
         (destination == Type::ManagedString && (source == Type::String || source == Type::PChar)) ||
         classAssignable(destination, source) ||
         (destination == Type::Pointer && source == Type::PChar);
}

class Analyzer {
  int nextId = 0;
  int nextRecordId = 0;
  int nextArrayId = 0;
  Routine* checkingInterface = nullptr;
  std::unordered_set<std::string> externalNames;
  int loopDepth = 0;
  std::vector<const Variable*> loopControlVariables;
  std::unordered_set<std::string> routineNames;
  struct ImportedSymbol { bool found = false; Routine* routine = nullptr; Constant* constant = nullptr; Variable* variable = nullptr; Type classType; };
  struct ImportedRoutines { bool found = false; std::vector<Routine*> routines; };

  std::vector<Routine*> visibleUnits(Routine& scope) {
    std::vector<Routine*> result;
    std::unordered_set<Routine*> seen;
    auto visit = [&](auto&& self, Routine* unit) -> void {
      if (!unit || !seen.insert(unit).second) return;
      result.push_back(unit);
      for (size_t index = 0; index < unit->interfaceImportCount && index < unit->imports.size(); ++index)
        self(self, unit->imports[index]);
    };

    for (size_t index = 0; index < scope.imports.size(); ++index) {
      if (&scope == checkingInterface && index >= scope.interfaceImportCount) break;
      visit(visit, scope.imports[index]);
    }
    return result;
  }

  Routine* findImportedUnit(Routine& scope, const std::string& name) {
    auto visit = [&](auto&& self, Routine& scope) -> Routine* {
      for (size_t index = 0; index < scope.imports.size(); ++index) {
        if (&scope == checkingInterface && index >= scope.interfaceImportCount) break;
        auto* unit = scope.imports[index];
        if (unit->name == name) return unit;
        // Interface dependencies are visible for qualified names.
        for (size_t dep = 0; dep < unit->interfaceImportCount; ++dep) {
          auto* child = unit->imports[dep];
          if (child->name == name) return child;
          if (auto* found = self(self, *child); found) return found;
        }
      }
      return nullptr;
    };
    return visit(visit, scope);
  }

  Variable* findExportedVariable(Routine& unit, const std::string& name) {
    if (!unit.exports.count(name)) return nullptr;
    for (auto& variable : unit.variables) if (variable->name == name) return variable.get();
    return nullptr;
  }

  Constant* findExportedConstant(Routine& unit, const std::string& name) {
    if (!unit.exports.count(name)) return nullptr;
    for (auto& constant : unit.constants) if (constant.name == name) return &constant;
    return nullptr;
  }

  std::vector<Routine*> findExportedRoutines(Routine& unit, const std::string& name) {
    std::vector<Routine*> result;
    if (!unit.exports.count(name)) return result;
    for (auto& routine : unit.routines) if (routine->name == name) result.push_back(routine.get());
    return result;
  }
  ImportedSymbol importedSymbol(Routine& scope, const std::string& name) {
    ImportedSymbol result;
    for (auto* unit : visibleUnits(scope)) {
      if (!unit->exports.count(name)) continue;
      ImportedSymbol candidate;
      for (auto& routine : unit->routines) if (routine->name == name) { candidate.found = true; candidate.routine = routine.get(); }
      for (auto& constant : unit->constants) if (constant.name == name) { candidate.found = true; candidate.constant = &constant; }
      for (auto& variable : unit->variables) if (variable->name == name) { candidate.found = true; candidate.variable = variable.get(); }
      for (auto& record : unit->records) if (record->name == name) { candidate.found = true; if (record->isClass) candidate.classType = {Type::Class, record.get()}; }
      for (auto& array : unit->arrays) if (array->name == name) candidate.found = true;
      for (auto& callback : unit->callbacks) if (callback->name == name) candidate.found = true;
      for (auto& alias : unit->aliases) if (alias.name == name) { candidate.found = true; if (alias.type.kind == Type::Class) candidate.classType = alias.type; }
      if (candidate.found) {
        if (result.found) error(scope.line, "ambiguous imported identifier '" + name + "'");
        result = candidate;
      }
    }
    return result;
  }

  ImportedRoutines importedRoutines(Routine& scope, const std::string& name) {
    ImportedRoutines result;
    for (auto* unit : visibleUnits(scope)) {
      if (!unit->exports.count(name)) continue;
      ImportedRoutines candidate;
      for (auto& routine : unit->routines) if (routine->name == name) { candidate.found = true; candidate.routines.push_back(routine.get()); }
      if (candidate.found) {
        if (result.found) error(scope.line, "ambiguous imported identifier '" + name + "'");
        result = std::move(candidate);
      }
    }
    return result;
  }
  Variable* findVariable(Routine* scope, const std::string& name) {
    for (; scope; scope = scope->parent) {
      if (scope->resultVariable && (name == scope->name || name == "result")) return scope->resultVariable;
      for (auto& variable : scope->variables) if (variable->name == name) return variable.get();
      for (auto& routine : scope->routines) if (routine->name == name) return nullptr;
      for (auto& record : scope->records) if (record->name == name) return nullptr;
      for (auto& array : scope->arrays) if (array->name == name) return nullptr;
      for (auto& callback : scope->callbacks) if (callback->name == name) return nullptr;
      for (auto& alias : scope->aliases) if (alias.name == name) return nullptr;
      for (auto& constant : scope->constants) if (constant.name == name) return nullptr;
      auto imported = importedSymbol(*scope, name);
      if (imported.found) return imported.variable;
    }
    return nullptr;
  }
  Routine* findRoutine(Routine* scope, const std::string& name) {
    auto candidates = findRoutines(scope, name);
    if (candidates.size() != 1) return nullptr;
    return candidates[0];
  }

  std::vector<Routine*> findRoutines(Routine* scope, const std::string& name) {
    for (; scope; scope = scope->parent) {
      std::vector<Routine*> found;
      for (auto& routine : scope->routines) if (routine->name == name) found.push_back(routine.get());
      if (!found.empty()) return found;
      for (auto& variable : scope->variables) if (variable->name == name && variable.get() != scope->resultVariable) return {};
      for (auto& record : scope->records) if (record->name == name) return {};
      for (auto& array : scope->arrays) if (array->name == name) return {};
      for (auto& callback : scope->callbacks) if (callback->name == name) return {};
      for (auto& alias : scope->aliases) if (alias.name == name) return {};
      for (auto& constant : scope->constants) if (constant.name == name) return {};
      auto imported = importedRoutines(*scope, name);
      if (imported.found) return imported.routines;
    }
    return {};
  }

  static int conversionCost(Type destination, Type source) {
    if (destination == source) return 0;
    if (destination == Type::Int64 && source == Type::Int32) return 1;
    if (destination.kind == Type::Real && integer(source)) return 1;
    if ((destination == Type::Pointer || destination == Type::PChar || destination.kind == Type::Callback || destination.kind == Type::Class) && source == Type::Nil) return 1;
    if (destination == Type::Pointer && source == Type::PChar) return 1;
    if (destination == Type::PChar && source == Type::String) return 1;
    if (destination == Type::PChar && source == Type::ManagedString) return 2;
    if (destination == Type::ManagedString && source == Type::String) return 1;
    if (destination == Type::ManagedString && source == Type::PChar) return 1;
    if (classAssignable(destination, source)) return 1;
    return 1000;
  }
  Constant* findConstant(Routine* scope, const std::string& name) {
    for (; scope; scope = scope->parent) {
      if (scope->resultVariable && (name == scope->name || name == "result")) return nullptr;
      bool visibleLocal = scope != checkingInterface || scope->exports.count(name);
      if (visibleLocal) {
        for (size_t index = 0; index < scope->constants.size(); ++index) {
          if (scope == checkingInterface && index >= scope->interfaceConstantCount) break;
          if (scope->constants[index].name == name) return &scope->constants[index];
        }
        for (auto& variable : scope->variables) if (variable->name == name) return nullptr;
        for (auto& routine : scope->routines) if (routine->name == name) return nullptr;
        for (auto& record : scope->records) if (record->name == name) return nullptr;
        for (auto& array : scope->arrays) if (array->name == name) return nullptr;
        for (auto& callback : scope->callbacks) if (callback->name == name) return nullptr;
        for (auto& alias : scope->aliases) if (alias.name == name) return nullptr;
      }
      auto imported = importedSymbol(*scope, name);
      if (imported.found) return imported.constant;
    }
    return nullptr;
  }
  Type findClass(Routine* scope, const std::string& name) {
    for (; scope; scope = scope->parent) {
      if (scope->resultVariable && (name == scope->name || name == "result")) return Type::Void;
      for (auto& record : scope->records) if (record->name == name) return record->isClass ? Type{Type::Class, record.get()} : Type::Void;
      for (auto& alias : scope->aliases) if (alias.name == name) return alias.type.kind == Type::Class ? alias.type : Type::Void;
      for (auto& array : scope->arrays) if (array->name == name) return Type::Void;
      for (auto& callback : scope->callbacks) if (callback->name == name) return Type::Void;
      for (auto& variable : scope->variables) if (variable->name == name) return Type::Void;
      for (auto& routine : scope->routines) if (routine->name == name) return Type::Void;
      for (auto& constant : scope->constants) if (constant.name == name) return Type::Void;
      auto imported = importedSymbol(*scope, name);
      if (imported.found) return imported.classType;
    }
    return Type::Void;
  }
  void prepare(Routine& routine) {
    routine.id = nextId++;
    if (routine.external) {
      if (!routine.parent || routine.parent->parent) error(routine.line, "external declarations must be at program scope");
      const auto& symbol = routine.externalName;
      if (symbol.empty() || !(std::isalpha(static_cast<unsigned char>(symbol[0])) || symbol[0] == '_')) error(routine.line, "invalid external symbol");
      for (char ch : symbol) if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) error(routine.line, "invalid external symbol");
      if (symbol == "main" || symbol.starts_with("p64_") || symbol.starts_with("p64fn") || symbol.starts_with("p64cb")) error(routine.line, "reserved external symbol");
      if (!externalNames.insert(symbol).second) error(routine.line, "duplicate external symbol '" + symbol + "'");
      if (routine.result != Type::Void && !cValue(routine.result) && routine.result.kind != Type::Callback) error(routine.line, "external result must be Integer, Int64, Pointer, PChar or a callback");
      std::unordered_set<std::string> parameterNames;
      for (auto& parameter : routine.variables) {
        if (!parameterNames.insert(parameter->name).second) error(parameter->line, "duplicate declaration '" + parameter->name + "'");
        if (!cValue(parameter->type) && parameter->type.kind != Type::Callback) error(parameter->line, "external parameter must be Integer, Int64, Pointer, PChar or a callback");
      }
      return;
    }
    if (routine.result != Type::Void) {
      auto result = std::make_unique<Variable>(Variable{"result", routine.result, routine.line});
      routine.resultVariable = result.get(); routine.variables.push_back(std::move(result));
    }
    std::unordered_set<std::string> names;
    std::unordered_map<std::string, std::unordered_set<std::string>> overloads;
    if (routine.resultVariable) names.insert(routine.name);
    for (auto& record : routine.records) {
      if (!names.insert(record->name).second) error(record->line, "duplicate declaration '" + record->name + "'");
      record->id = nextRecordId++;
      std::unordered_set<std::string> fieldNames;
      int inheritedFields = 0;
      for (auto* base = record->base; base; base = base->base) {
        inheritedFields += static_cast<int>(base->fields.size());
        for (auto& field : base->fields) fieldNames.insert(field->name);
        for (auto& method : base->methods) fieldNames.insert(method->name);
      }
      for (size_t index = 0; index < record->fields.size(); ++index) {
        auto& field = *record->fields[index];
        if (!fieldNames.insert(field.name).second) error(field.line, "duplicate field '" + field.name + "'");
        field.owner = record.get(); field.index = static_cast<int>(index) + (record->isClass ? inheritedFields + 1 : 0);
      }
      if (record->base) record->virtualMethods = record->base->virtualMethods;
      std::unordered_set<std::string> ownMembers;
      for (auto& field : record->fields) ownMembers.insert(field->name);
      for (auto& method : record->methods) {
        if ((method->name == "create" && !method->constructor) || method->name == "free" || (method->name == "destroy" && !method->destructor) || !ownMembers.insert(method->name).second)
          error(method->line, "duplicate or reserved class member '" + method->name + "'");
        if ((method->constructor && method->name != "create") || (method->destructor && method->name != "destroy"))
          error(method->line, "lifecycle methods must be named Create or Destroy");
        if ((method->constructor || method->destructor) && (method->virtualMethod || method->overrideMethod))
          error(method->line, "lifecycle methods do not accept virtual or override");
        if (method->destructor && method->parameterCount != 1) error(method->line, "Destroy takes no arguments");
        if (method->destructor && routine.parent) error(method->line, "destructors require a program- or unit-scope class");
        Routine* inherited = nullptr;
        for (auto* base = record->base; base && !inherited; base = base->base)
          for (auto& candidate : base->methods) if (candidate->name == method->name) { inherited = candidate.get(); break; }
        if (method->overrideMethod) {
          if (!inherited || !inherited->virtualMethod) error(method->line, "override requires an inherited virtual method");
          if (method->result != inherited->result || method->parameterCount != inherited->parameterCount) error(method->line, "override signature mismatch");
          for (size_t index = 1; index < method->parameterCount; ++index)
            if (method->variables[index]->type != inherited->variables[index]->type || method->variables[index]->byReference != inherited->variables[index]->byReference)
              error(method->line, "override signature mismatch");
          method->virtualMethod = true; method->virtualIndex = inherited->virtualIndex;
          record->virtualMethods.at(static_cast<size_t>(method->virtualIndex)) = method.get();
        } else {
          if (fieldNames.count(method->name) && !(inherited && ((method->constructor && inherited->constructor) || (method->destructor && inherited->destructor)))) error(method->line, "duplicate or reserved class member '" + method->name + "'");
          if (method->virtualMethod) {
            method->virtualIndex = static_cast<int>(record->virtualMethods.size());
            record->virtualMethods.push_back(method.get());
          }
        }
        if (method->virtualMethod && routine.parent) error(method->line, "virtual methods require a program- or unit-scope class");
        prepare(*method);
      }
    }
    for (auto& alias : routine.aliases)
      if (!names.insert(alias.name).second) error(alias.line, "duplicate declaration '" + alias.name + "'");
    for (auto& array : routine.arrays) {
      if (!names.insert(array->name).second) error(array->line, "duplicate declaration '" + array->name + "'");
      array->id = nextArrayId++;
    }
    for (auto& callback : routine.callbacks) {
      if (!names.insert(callback->name).second) error(callback->line, "duplicate declaration '" + callback->name + "'");
      if (callback->result != Type::Void && !cValue(callback->result)) error(callback->line, "unsupported callback result type");
      std::unordered_set<std::string> parameterNames;
      for (auto& parameter : callback->parameters) {
        if (!parameterNames.insert(parameter->name).second) error(parameter->line, "duplicate declaration '" + parameter->name + "'");
        if (!cValue(parameter->type)) error(parameter->line, "unsupported callback parameter type");
      }
    }
    for (auto& constant : routine.constants)
      if (!names.insert(constant.name).second) error(constant.line, "duplicate declaration '" + constant.name + "'");
    for (size_t index = 0; index < routine.variables.size(); ++index) {
      auto& variable = *routine.variables[index];
      if (!names.insert(variable.name).second) error(variable.line, "duplicate declaration '" + variable.name + "'");
      variable.owner = &routine; variable.index = static_cast<int>(index + 1);
    }
    for (auto& child : routine.routines) {
      if (names.count(child->name) && !routineNames.count(child->name))
        error(child->line, "duplicate declaration '" + child->name + "'");
      names.insert(child->name);
      routineNames.insert(child->name);

      std::string key = std::to_string(child->result.kind) + ":";
      for (size_t index = 0; index < child->parameterCount; ++index) {
        const auto& parameter = *child->variables[index];
        key += std::to_string(parameter.type.kind);
        if (parameter.byReference) key += "&";
        key += ",";
      }
      if (!overloads[child->name].insert(key).second)
        error(child->line, "duplicate declaration '" + child->name + "'");

      prepare(*child);
    }
  }
  Type selectedType(Routine& scope, Type type, std::vector<Selector>& selectors) {
    for (auto& selector : selectors) {
      if (selector.index) {
        if (type.kind != Type::Array) error(selector.line, "indexing requires an array");
        if (!integer(expression(scope, *selector.index))) error(selector.line, "array index must be an integer");
        selector.array = type.array; type = type.array->element;
      } else {
        if (type.kind != Type::Record && type.kind != Type::Class) error(selector.line, "field access requires a record or class");
        selector.classField = type.kind == Type::Class;
        for (auto* record = type.record; record && !selector.field; record = record->base)
          for (auto& field : record->fields) if (field->name == selector.name) { selector.field = field.get(); break; }
        if (!selector.field) error(selector.line, "unknown field '" + selector.name + "'");
        type = selector.field->type;
      }
    }
    return type;
  }
  int checkArguments(Routine& scope, const std::string& name, int line, std::vector<std::unique_ptr<Expr>>& arguments, Routine* target, bool value, size_t offset) {
    if (value && target->result == Type::Void) error(line, "procedure cannot be used as a value");
    if (arguments.size() + offset != target->parameterCount) error(line, "wrong number of arguments for '" + name + "'");
    int cost = 0;
    for (size_t index = 0; index < arguments.size(); ++index) {
      const auto& parameter = *target->variables[index + offset];
      Type actual = expression(scope, *arguments[index], parameter.type);
      if (parameter.byReference && (arguments[index]->kind != Expr::Kind::Name || !arguments[index]->variable))
        error(arguments[index]->line, "var argument must be a variable");
      if (parameter.byReference && arguments[index]->variable) {
        for (const auto* control : loopControlVariables)
          if (control == arguments[index]->variable)
            error(arguments[index]->line, "loop control variable must not be modified");
      }
      if (!parameter.byReference && parameter.type == Type::PChar && actual == Type::ManagedString) {
        // Allow passing managed strings to PChar parameters (C APIs).
        // The pointer is only guaranteed valid during the call.
      } else if (parameter.byReference ? parameter.type != actual : !assignable(parameter.type, actual))
        error(arguments[index]->line, "incompatible argument type for '" + name + "'");
      if (!parameter.byReference) cost += conversionCost(parameter.type, actual);
    }
    return cost;
  }
  void call(Routine& scope, const std::string& name, int line, std::vector<std::unique_ptr<Expr>>& arguments, Routine*& target, bool value) {
    auto candidates = findRoutines(&scope, name);
    if (candidates.empty()) error(line, "unknown routine '" + name + "'");

    int bestCost = INT_MAX;
    Routine* best = nullptr;
    std::string firstError;
    for (auto* candidate : candidates) {
      try {
        int cost = checkArguments(scope, name, line, arguments, candidate, value, 0);
        if (cost < bestCost) { bestCost = cost; best = candidate; }
        else if (cost == bestCost) best = nullptr;
      } catch (const std::runtime_error& e) {
        if (firstError.empty()) firstError = e.what();
      }
    }
    if (!best) {
      if (!firstError.empty()) throw std::runtime_error(firstError);
      error(line, "ambiguous call to '" + name + "'");
    }
    target = best;
    (void)checkArguments(scope, name, line, arguments, target, value, 0);
  }

  void callCandidates(Routine& scope, const std::string& name, int line, std::vector<std::unique_ptr<Expr>>& arguments, const std::vector<Routine*>& candidates, Routine*& target, bool value) {
    if (candidates.empty()) error(line, "unknown routine '" + name + "'");

    int bestCost = INT_MAX;
    Routine* best = nullptr;
    std::string firstError;
    for (auto* candidate : candidates) {
      try {
        int cost = checkArguments(scope, name, line, arguments, candidate, value, 0);
        if (cost < bestCost) { bestCost = cost; best = candidate; }
        else if (cost == bestCost) best = nullptr;
      } catch (const std::runtime_error& e) {
        if (firstError.empty()) firstError = e.what();
      }
    }
    if (!best) {
      if (!firstError.empty()) throw std::runtime_error(firstError);
      error(line, "ambiguous call to '" + name + "'");
    }
    target = best;
    (void)checkArguments(scope, name, line, arguments, target, value, 0);
  }
  void methodCall(Routine& scope, const std::string& name, int line, std::vector<Selector>& selectors,
                  std::vector<std::unique_ptr<Expr>>& arguments, Variable*& variable, Routine*& target, bool value) {
    if (selectors.empty() || selectors.back().index) error(line, "method call requires a method name");
    std::string methodName = selectors.back().name; selectors.pop_back();
    variable = findVariable(&scope, name);
    if (!variable) error(line, "unknown variable '" + name + "'");
    Type receiver = selectedType(scope, variable->type, selectors);
    if (receiver.kind != Type::Class) error(line, "method call requires a class instance");
    for (auto* record = receiver.record; record && !target; record = record->base)
      for (auto& method : record->methods) if (method->name == methodName) { target = method.get(); break; }
    if (!target) error(line, "unknown class method '" + methodName + "'");
    if (target->constructor || target->destructor) error(line, "lifecycle methods cannot be called on an instance");
    (void)checkArguments(scope, methodName, line, arguments, target, value, 1);
  }
  void construct(Routine& scope, Expr& expr) {
    for (auto* record = expr.type.record; record && !expr.routine; record = record->base)
      for (auto& method : record->methods) if (method->constructor) { expr.routine = method.get(); break; }
    if (expr.routine) (void)checkArguments(scope, "Create", expr.line, expr.operands, expr.routine, false, 1);
    else if (!expr.operands.empty()) error(expr.line, "Create takes no arguments");
    expr.kind = Expr::Kind::NewObject;
  }
  Type expression(Routine& scope, Expr& expr, Type expected = Type::Void) {
    switch (expr.kind) {
    case Expr::Kind::Integer: {
      try {
        size_t used = 0;
        auto value = std::stoll(expr.text, &used);
        if (used != expr.text.size()) error(expr.line, "invalid integer literal");
        expr.type = value >= INT32_MIN && value <= INT32_MAX ? Type::Int32 : Type::Int64;
      } catch (const std::out_of_range&) { error(expr.line, "integer literal out of range"); }
      break;
    }
    case Expr::Kind::Real: {
      try {
        size_t used = 0;
        (void)std::stod(expr.text, &used);
        if (used != expr.text.size()) error(expr.line, "invalid real literal");
        expr.type = {Type::Real};
      } catch (const std::exception&) { error(expr.line, "invalid real literal"); }
      break;
    }
    case Expr::Kind::Boolean: expr.type = Type::Boolean; break;
    case Expr::Kind::String: expr.type = Type::String; break;
    case Expr::Kind::Nil: expr.type = Type::Nil; break;
    case Expr::Kind::MemberCall: {
      if (auto* unit = findImportedUnit(scope, expr.text)) {
        if (expr.selectors.size() != 1 || expr.selectors[0].index) error(expr.line, "qualified unit call requires a routine name");
        std::string name = expr.selectors[0].name;
        auto candidates = findExportedRoutines(*unit, name);
        callCandidates(scope, name, expr.line, expr.operands, candidates, expr.routine, true);
        expr.type = expr.routine->result;
        expr.kind = Expr::Kind::Call;
        expr.text = name;
        expr.selectors.clear();
        break;
      }
      if (!findVariable(&scope, expr.text) && expr.selectors.size() == 1 && !expr.selectors[0].index && expr.selectors[0].name == "create") {
        expr.type = findClass(&scope, expr.text);
        if (expr.type.kind != Type::Class) error(expr.line, "unknown class '" + expr.text + "'");
        construct(scope, expr);
      } else {
        methodCall(scope, expr.text, expr.line, expr.selectors, expr.operands, expr.variable, expr.routine, true);
        expr.type = expr.routine->result;
      }
      break;
    }
    case Expr::Kind::NewObject: break;
    case Expr::Kind::RoutineAddress: {
      if (expected.kind != Type::Callback) error(expr.line, "routine address requires a callback type");
      auto candidates = findRoutines(&scope, expr.text);
      if (candidates.empty()) error(expr.line, "unknown routine '" + expr.text + "'");
      auto* signature = expected.callback;
      Routine* match = nullptr;
      for (auto* candidate : candidates) {
        if (candidate->result != signature->result || candidate->parameterCount != signature->parameters.size()) continue;
        bool ok = true;
        for (size_t index = 0; index < candidate->parameterCount; ++index)
          if (candidate->variables[index]->type != signature->parameters[index]->type || candidate->variables[index]->byReference != signature->parameters[index]->byReference)
            ok = false;
        if (!ok) continue;
        if (candidate->external || !candidate->parent || candidate->parent->parent)
          error(expr.line, "callbacks require a program- or unit-scope Pascal routine");
        if (match) error(expr.line, "incompatible callback signature");
        match = candidate;
      }
      if (!match) error(expr.line, "incompatible callback signature");
      match->addressTaken = true; expr.routine = match; expr.type = expected; break;
    }
    case Expr::Kind::Name:
      if (auto* constant = findConstant(&scope, expr.text)) {
        if (!expr.selectors.empty()) error(expr.line, "field access requires a record");
        if (!constant->resolved) error(expr.line, "constant must refer to an earlier constant");
        expr.kind = constant->value->kind; expr.text = constant->value->text; expr.type = constant->value->type;
        break;
      }
      expr.variable = findVariable(&scope, expr.text);
      if (expr.variable) expr.type = selectedType(scope, expr.variable->type, expr.selectors);
      else {
        if (!expr.selectors.empty() && !expr.selectors[0].index) {
          if (auto* unit = findImportedUnit(scope, expr.text)) {
            std::string name = expr.selectors[0].name;
            expr.selectors.erase(expr.selectors.begin());
            if (auto* constant = findExportedConstant(*unit, name)) {
              if (!expr.selectors.empty()) error(expr.line, "field access requires a record");
              if (!constant->resolved) error(expr.line, "constant must refer to an earlier constant");
              expr.kind = constant->value->kind; expr.text = constant->value->text; expr.type = constant->value->type;
              expr.selectors.clear();
              break;
            }
            if (auto* variable = findExportedVariable(*unit, name)) {
              expr.variable = variable;
              expr.type = selectedType(scope, expr.variable->type, expr.selectors);
              break;
            }
            auto routines = findExportedRoutines(*unit, name);
            if (!routines.empty()) {
              expr.kind = Expr::Kind::Call;
              expr.text = name;
              callCandidates(scope, name, expr.line, expr.operands, routines, expr.routine, true);
              expr.type = expr.routine->result;
              if (!expr.selectors.empty()) expr.type = selectedType(scope, expr.type, expr.selectors);
              break;
            }
            error(expr.line, "unknown imported identifier '" + name + "'");
          }
        }
        if (expr.selectors.size() == 1 && !expr.selectors[0].index && expr.selectors[0].name == "create") {
          expr.type = findClass(&scope, expr.text);
          if (expr.type.kind == Type::Class) { construct(scope, expr); break; }
        }
        if (!expr.selectors.empty()) {
          if (!findRoutines(&scope, expr.text).empty()) {
            expr.kind = Expr::Kind::Call;
            call(scope, expr.text, expr.line, expr.operands, expr.routine, true);
            expr.type = expr.routine->result;
            expr.type = selectedType(scope, expr.type, expr.selectors);
            break;
          }
          error(expr.line, "unknown variable '" + expr.text + "'");
        }
        if (findRoutines(&scope, expr.text).empty()) error(expr.line, "unknown variable '" + expr.text + "'");
        expr.kind = Expr::Kind::Call;
        call(scope, expr.text, expr.line, expr.operands, expr.routine, true);
        expr.type = expr.routine->result;
      }
      break;
    case Expr::Kind::Call:
      call(scope, expr.text, expr.line, expr.operands, expr.routine, true);
      expr.type = expr.routine->result;
      if (!expr.selectors.empty()) expr.type = selectedType(scope, expr.type, expr.selectors);
      break;
    case Expr::Kind::Unary: {
      Type operand = expression(scope, *expr.operands[0]);
      if (expr.text == "not" ? operand != Type::Boolean : !(integer(operand) || real(operand))) error(expr.line, "invalid unary operand");
      expr.type = operand; break;
    }
    case Expr::Kind::Binary: {
      Type left = expression(scope, *expr.operands[0]); Type right = expression(scope, *expr.operands[1]);
      if (expr.text == "and" || expr.text == "or") {
        if (left != Type::Boolean || right != Type::Boolean) error(expr.line, "boolean operands required");
        expr.type = Type::Boolean;
      } else if (expr.text == "=" || expr.text == "<>" || expr.text == "<" || expr.text == ">" || expr.text == "<=" || expr.text == ">=") {
        bool equality = expr.text == "=" || expr.text == "<>";
        bool pointerComparison = ((pointer(left) || left == Type::Nil) && (pointer(right) || right == Type::Nil)) ||
                                 (left.kind == Type::Callback && (right == left || right == Type::Nil)) ||
                                 (right.kind == Type::Callback && left == Type::Nil) ||
                                 (left.kind == Type::Class && (classAssignable(left, right) || classAssignable(right, left) || right == Type::Nil)) ||
                                 (right.kind == Type::Class && left == Type::Nil);
        bool numericComparison = (integer(left) || real(left)) && (integer(right) || real(right));
        if (!numericComparison && !(stringType(left) && stringType(right)) && !(equality && ((left == Type::Boolean && right == Type::Boolean) || pointerComparison))) error(expr.line, "incompatible comparison operands");
        expr.type = Type::Boolean;
      } else if (expr.text == "+" && stringType(left) && stringType(right)) {
        expr.type = Type::ManagedString;
      } else {
        if (!(integer(left) || real(left)) || !(integer(right) || real(right))) error(expr.line, "numeric operands required");
        if (real(left) || real(right) || expr.text == "/") expr.type = {Type::Real};
        else expr.type = left == Type::Int64 || right == Type::Int64 ? Type::Int64 : Type::Int32;
      }
      break;
    }
    }
    return expr.type;
  }
  void statement(Routine& scope, Stmt& stmt) {
    switch (stmt.kind) {
    case Stmt::Kind::Block: for (auto& child : stmt.statements) statement(scope, *child); break;
    case Stmt::Kind::Assign:
      stmt.variable = findVariable(&scope, stmt.text);
      if (!stmt.variable && !stmt.selectors.empty() && !stmt.selectors[0].index) {
        if (auto* unit = findImportedUnit(scope, stmt.text)) {
          std::string name = stmt.selectors[0].name;
          if (auto* variable = findExportedVariable(*unit, name)) {
            stmt.variable = variable;
            stmt.selectors.erase(stmt.selectors.begin());
          }
        }
      }
      if (!stmt.variable) error(stmt.line, "unknown variable '" + stmt.text + "'");
      for (const auto* control : loopControlVariables)
        if (control == stmt.variable && stmt.selectors.empty())
          error(stmt.line, "loop control variable must not be modified");
      stmt.targetType = selectedType(scope, stmt.variable->type, stmt.selectors);
      if (!assignable(stmt.targetType, expression(scope, *stmt.expressions[0], stmt.targetType))) error(stmt.line, "incompatible assignment to '" + stmt.text + "'");
      break;
    case Stmt::Kind::Call:
      call(scope, stmt.text, stmt.line, stmt.expressions, stmt.routine, false);
      break;
    case Stmt::Kind::MethodCall: {
      if (auto* unit = findImportedUnit(scope, stmt.text)) {
        if (stmt.selectors.size() != 1 || stmt.selectors[0].index) error(stmt.line, "qualified unit call requires a routine name");
        std::string name = stmt.selectors[0].name;
        auto candidates = findExportedRoutines(*unit, name);
        callCandidates(scope, name, stmt.line, stmt.expressions, candidates, stmt.routine, false);
        stmt.kind = Stmt::Kind::Call;
        stmt.text = name;
        stmt.selectors.clear();
        break;
      }
      methodCall(scope, stmt.text, stmt.line, stmt.selectors, stmt.expressions, stmt.variable, stmt.routine, false);
      break;
    }
    case Stmt::Kind::Destroy:
      if (!stmt.expressions.empty()) error(stmt.line, "Free takes no arguments");
      stmt.variable = findVariable(&scope, stmt.text);
      if (!stmt.variable) error(stmt.line, "unknown variable '" + stmt.text + "'");
      stmt.targetType = selectedType(scope, stmt.variable->type, stmt.selectors);
      if (stmt.targetType.kind != Type::Class) error(stmt.line, "Free requires a class instance");
      break;
    case Stmt::Kind::Write:
      for (auto& expr : stmt.expressions) {
        Type type = expression(scope, *expr);
        if (!integer(type) && !stringType(type) && !real(type))
          error(expr->line, "Write supports integer, real and string values");
      }
      break;
    case Stmt::Kind::If:
      if (expression(scope, *stmt.expressions[0]) != Type::Boolean) error(stmt.line, "boolean condition required");
      for (auto& child : stmt.statements) statement(scope, *child);
      break;
    case Stmt::Kind::While: case Stmt::Kind::Repeat:
      if (expression(scope, *stmt.expressions[0]) != Type::Boolean) error(stmt.line, "boolean condition required");
      ++loopDepth;
      for (auto& child : stmt.statements) statement(scope, *child);
      --loopDepth;
      break;
    case Stmt::Kind::For:
      stmt.variable = findVariable(&scope, stmt.text);
      if (!stmt.variable) error(stmt.line, "for control variable must be an integer");
      stmt.targetType = selectedType(scope, stmt.variable->type, stmt.selectors);
      if (!integer(stmt.targetType)) error(stmt.line, "for control variable must be an integer");
      if (!assignable(stmt.targetType, expression(scope, *stmt.expressions[0])) || !assignable(stmt.targetType, expression(scope, *stmt.expressions[2]))) error(stmt.line, "incompatible for bounds");
      ++loopDepth;
      if (stmt.selectors.empty()) loopControlVariables.push_back(stmt.variable);
      statement(scope, *stmt.statements[0]);
      if (stmt.selectors.empty()) loopControlVariables.pop_back();
      --loopDepth;
      break;
    case Stmt::Kind::Break: case Stmt::Kind::Continue:
      if (!loopDepth) error(stmt.line, stmt.text + " requires a loop");
      break;
    case Stmt::Kind::Raise: break;
    case Stmt::Kind::Try:
    case Stmt::Kind::TryFinally:
      for (auto& child : stmt.statements) statement(scope, *child);
      break;
    case Stmt::Kind::Empty: break;
    }
  }
  void validateConstant(Routine& scope, Expr& value) {
    switch (value.kind) {
    case Expr::Kind::Integer: case Expr::Kind::Real: case Expr::Kind::Boolean: case Expr::Kind::String: case Expr::Kind::Nil:
      return;
    case Expr::Kind::Name: {
      auto* referenced = findConstant(&scope, value.text);
      if (!referenced || !referenced->resolved || !value.selectors.empty()) error(value.line, "constant must refer to an earlier constant");
      return;
    }
    case Expr::Kind::Unary: case Expr::Kind::Binary:
      for (auto& operand : value.operands) validateConstant(scope, *operand);
      return;
    case Expr::Kind::Call: case Expr::Kind::RoutineAddress: case Expr::Kind::MemberCall: case Expr::Kind::NewObject:
      error(value.line, "constant expression cannot call or reference a routine");
    }
  }
  void foldConstant(Expr& value) {
    for (auto& operand : value.operands) foldConstant(*operand);
    if (value.kind != Expr::Kind::Unary && value.kind != Expr::Kind::Binary) return;
    const auto& left = *value.operands[0];
    const auto& operation = value.text;
    if (stringType(left.type)) {
      const auto& right = *value.operands[1];
      if (!stringType(right.type)) error(value.line, "incompatible operands in constant expression");
      std::string leftValue = decodeStringLiteral(left.text);
      std::string rightValue = decodeStringLiteral(right.text);
      if (operation == "+") {
        value.kind = Expr::Kind::String;
        value.type = Type::String;
        value.text = encodeStringLiteral(leftValue + rightValue);
        value.operands.clear();
        return;
      }
      int comparison = leftValue.compare(rightValue);
      bool result = false;
      if (operation == "=") result = comparison == 0;
      else if (operation == "<>") result = comparison != 0;
      else if (operation == "<") result = comparison < 0;
      else if (operation == ">") result = comparison > 0;
      else if (operation == "<=") result = comparison <= 0;
      else if (operation == ">=") result = comparison >= 0;
      else error(value.line, "invalid string constant expression");
      value.kind = Expr::Kind::Boolean;
      value.type = Type::Boolean;
      value.text = result ? "true" : "false";
      value.operands.clear();
      return;
    }

    if (value.type.kind == Type::Real || left.type.kind == Type::Real || (value.kind == Expr::Kind::Binary && value.operands[1]->type.kind == Type::Real)) {
      if (value.kind == Expr::Kind::Unary) error(value.line, "invalid real constant expression");
      const auto& right = *value.operands[1];
      if (!(integer(left.type) || left.type.kind == Type::Real) || !(integer(right.type) || right.type.kind == Type::Real))
        error(value.line, "incompatible operands in constant expression");
      double leftValue = left.type.kind == Type::Real ? std::stod(left.text) : static_cast<double>(std::stoll(left.text));
      double rightValue = right.type.kind == Type::Real ? std::stod(right.text) : static_cast<double>(std::stoll(right.text));
      const auto& operation = value.text;
      if (operation == "+" || operation == "-" || operation == "*" || operation == "/") {
        if (operation == "/" && rightValue == 0.0) error(value.line, "division by zero in constant expression");
        double result = operation == "+" ? leftValue + rightValue : operation == "-" ? leftValue - rightValue : operation == "*" ? leftValue * rightValue : leftValue / rightValue;
        value.kind = Expr::Kind::Real;
        value.type = {Type::Real};
        value.text = encodeRealLiteral(result);
        value.operands.clear();
        return;
      }
      int comparison = leftValue < rightValue ? -1 : leftValue > rightValue ? 1 : 0;
      bool result = false;
      if (operation == "=") result = comparison == 0;
      else if (operation == "<>") result = comparison != 0;
      else if (operation == "<") result = comparison < 0;
      else if (operation == ">") result = comparison > 0;
      else if (operation == "<=") result = comparison <= 0;
      else if (operation == ">=") result = comparison >= 0;
      else error(value.line, "invalid real constant expression");
      value.kind = Expr::Kind::Boolean;
      value.type = Type::Boolean;
      value.text = result ? "true" : "false";
      value.operands.clear();
      return;
    }
    std::int64_t result = 0;
    bool booleanResult = value.type == Type::Boolean;
    if (left.type == Type::Boolean) {
      bool leftValue = left.text == "true";
      if (value.kind == Expr::Kind::Unary) result = !leftValue;
      else {
        bool rightValue = value.operands[1]->text == "true";
        if (operation == "and") result = leftValue && rightValue;
        else if (operation == "or") result = leftValue || rightValue;
        else if (operation == "=") result = leftValue == rightValue;
        else result = leftValue != rightValue;
      }
    } else if (left.type == Type::Nil) {
      result = operation == "=";
    } else {
      std::int64_t leftValue = std::stoll(left.text);
      bool overflow = false;
      if (value.kind == Expr::Kind::Unary) {
        if (operation == "-") overflow = __builtin_sub_overflow(std::int64_t{0}, leftValue, &result);
        else result = leftValue;
      } else {
        std::int64_t rightValue = std::stoll(value.operands[1]->text);
        if (operation == "+") overflow = __builtin_add_overflow(leftValue, rightValue, &result);
        else if (operation == "-") overflow = __builtin_sub_overflow(leftValue, rightValue, &result);
        else if (operation == "*") overflow = __builtin_mul_overflow(leftValue, rightValue, &result);
        else if (operation == "/" || operation == "div" || operation == "mod") {
          if (rightValue == 0) error(value.line, "division by zero in constant expression");
          if (leftValue == INT64_MIN && rightValue == -1) overflow = true;
          else result = operation == "mod" ? leftValue % rightValue : leftValue / rightValue;
        } else if (operation == "=") result = leftValue == rightValue;
        else if (operation == "<>") result = leftValue != rightValue;
        else if (operation == "<") result = leftValue < rightValue;
        else if (operation == ">") result = leftValue > rightValue;
        else if (operation == "<=") result = leftValue <= rightValue;
        else if (operation == ">=") result = leftValue >= rightValue;
      }
      if (overflow || (value.type == Type::Int32 && (result < INT32_MIN || result > INT32_MAX)))
        error(value.line, "integer overflow in constant expression");
    }
    value.kind = booleanResult ? Expr::Kind::Boolean : Expr::Kind::Integer;
    value.text = booleanResult ? (result ? "true" : "false") : std::to_string(result);
    value.operands.clear();
  }
  void check(Routine& routine) {
    if (routine.external) return;
    for (size_t index = 0; index < routine.constants.size(); ++index) {
      auto& constant = routine.constants[index];
      checkingInterface = routine.isUnit && index < routine.interfaceConstantCount ? &routine : nullptr;
      auto& value = *constant.value;
      validateConstant(routine, value);
      expression(routine, value);
      foldConstant(value);
      constant.resolved = true;
    }
    checkingInterface = nullptr;
    if (routine.body) statement(routine, *routine.body);
    if (routine.finalization) statement(routine, *routine.finalization);
    for (auto& child : routine.routines) check(*child);
    for (auto& record : routine.records) for (auto& method : record->methods) check(*method);
  }
public:
  void run(Routine& program) {
    for (auto& unit : program.units) prepare(*unit);
    prepare(program);
    for (auto& unit : program.units) check(*unit);
    check(program);
  }
};
}
void analyze(Routine& program) { Analyzer().run(program); }
