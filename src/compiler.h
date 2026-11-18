#pragma once

#include <memory>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

struct RecordType;
struct ArrayType;
struct CallbackType;
struct Type {
  enum Kind { Void, Int32, Int64, Real, Boolean, String, ManagedString, Pointer, PChar, Nil, Record, Array, Callback, Class } kind;
  RecordType* record;
  ArrayType* array;
  CallbackType* callback;
  Type(Kind value = Void, RecordType* recordType = nullptr, ArrayType* arrayType = nullptr, CallbackType* callbackType = nullptr) : kind(value), record(recordType), array(arrayType), callback(callbackType) {}
  bool operator==(const Type&) const = default;
};
struct Routine;
struct Field {
  std::string name;
  Type type;
  int line;
  RecordType* owner = nullptr;
  int index = 0;
};
struct RecordType {
  std::string name;
  int line;
  int id = 0;
  bool isClass = false;
  RecordType* base = nullptr;
  std::vector<std::unique_ptr<Field>> fields;
  std::vector<std::unique_ptr<Routine>> methods;
  std::vector<Routine*> virtualMethods;
};
struct TypeAlias {
  std::string name;
  Type type;
  int line;
};
struct ArrayType {
  std::string name;
  int line;
  std::int64_t lower;
  std::int64_t upper;
  Type element;
  int id = 0;
};
struct Variable {
  std::string name;
  Type type;
  int line;
  Routine* owner = nullptr;
  int index = 0;
  bool byReference = false;
};
struct CallbackType {
  std::string name;
  int line;
  Type result = Type::Void;
  std::vector<std::unique_ptr<Variable>> parameters;
};
struct Expr;
struct Selector {
  std::string name;
  int line;
  std::unique_ptr<Expr> index;
  Field* field = nullptr;
  ArrayType* array = nullptr;
  bool classField = false;
};
struct Expr {
  enum class Kind { Integer, Real, Boolean, String, Nil, Name, MemberCall, NewObject, RoutineAddress, Call, Unary, Binary } kind;
  std::string text;
  int line;
  Type type = Type::Void;
  Variable* variable = nullptr;
  Routine* routine = nullptr;
  std::vector<Selector> selectors;
  std::vector<std::unique_ptr<Expr>> operands;
  Expr(Kind nodeKind, std::string nodeText, int nodeLine) : kind(nodeKind), text(std::move(nodeText)), line(nodeLine) {}
};
struct Constant {
  std::string name;
  int line;
  std::unique_ptr<Expr> value;
  bool resolved = false;
};
struct Stmt {
  enum class Kind { Block, Assign, Call, MethodCall, Destroy, Write, If, While, Repeat, For, Break, Continue, Try, TryFinally, Raise, Empty } kind;
  std::string text;
  int line;
  Variable* variable = nullptr;
  Routine* routine = nullptr;
  Type targetType = Type::Void;
  std::vector<Selector> selectors;
  std::vector<std::unique_ptr<Expr>> expressions;
  std::vector<std::unique_ptr<Stmt>> statements;
  Stmt(Kind nodeKind, std::string nodeText, int nodeLine) : kind(nodeKind), text(std::move(nodeText)), line(nodeLine) {}
};
struct Routine {
  std::string name;
  std::string sourcePath;
  int line = 1;
  int id = 0;
  bool external = false;
  bool addressTaken = false;
  bool isUnit = false;
  bool prototype = false;
  RecordType* methodOwner = nullptr;
  bool constructor = false;
  bool destructor = false;
  bool virtualMethod = false;
  bool overrideMethod = false;
  int virtualIndex = -1;
  std::string externalName;
  Type result = Type::Void;
  Routine* parent = nullptr;
  std::vector<std::unique_ptr<Variable>> variables;
  std::size_t parameterCount = 0;
  Variable* resultVariable = nullptr;
  std::vector<std::unique_ptr<RecordType>> records;
  std::vector<std::unique_ptr<ArrayType>> arrays;
  std::vector<std::unique_ptr<CallbackType>> callbacks;
  std::vector<TypeAlias> aliases;
  std::vector<Constant> constants;
  std::vector<std::unique_ptr<Routine>> routines;
  std::vector<Routine*> imports;
  std::vector<std::unique_ptr<Routine>> units;
  std::unordered_set<std::string> exports;
  std::size_t interfaceConstantCount = 0;
  std::size_t interfaceImportCount = 0;
  std::unique_ptr<Stmt> body;
  std::unique_ptr<Stmt> finalization;
};

std::unique_ptr<Routine> parse(const std::string& source, const std::string& sourcePath, const std::vector<std::string>& unitPaths);
void analyze(Routine& program);
std::string generate(const Routine& program, bool debug);
