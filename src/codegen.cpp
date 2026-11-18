#include "compiler.h"

#include <cstdio>
#include <filesystem>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
bool integer(Type type) { return type == Type::Int32 || type == Type::Int64; }
std::string llvmType(Type type) {
  switch (type.kind) {
  case Type::Int32: return "i32";
  case Type::Int64: return "i64";
  case Type::Real: return "double";
  case Type::Boolean: return "i1";
  case Type::Pointer: case Type::PChar: case Type::Nil: case Type::Callback: case Type::ManagedString: case Type::Class: return "ptr";
  case Type::Void: return "void";
  case Type::Record: return "%record" + std::to_string(type.record->id);
  case Type::Array: return "[" + std::to_string(type.array->upper - type.array->lower + 1) + " x " + llvmType(type.array->element) + "]";
  case Type::String: break;
  }
  throw std::logic_error("invalid LLVM type");
}
bool needsManagement(Type type) {
  if (type == Type::ManagedString) return true;
  if (type.kind == Type::Array) return needsManagement(type.array->element);
  if (type.kind == Type::Record) {
    if (type.record->base && needsManagement({Type::Record, type.record->base})) return true;
    for (const auto& field : type.record->fields) if (needsManagement(field->type)) return true;
  }
  return false;
}
std::string manageName(Type type, bool retain) {
  return std::string("@p64_") + (retain ? "retain_" : "release_") +
         (type.kind == Type::Record ? "record" + std::to_string(type.record->id) : "array" + std::to_string(type.array->id));
}
struct Value {
  Type type;
  std::string name;
  bool owned = false;
  std::string slot;

  Value() = default;
  Value(Type type, std::string name, bool owned = false, std::string slot = {})
      : type(type), name(std::move(name)), owned(owned), slot(std::move(slot)) {}
};
class Generator {
  std::string types, globals, functions, code;
  std::string temporaryAllocations;
  std::vector<std::pair<Type, std::string>> temporaries;
  std::vector<std::string> tryHandlers;
  std::string unwindHandler;
  std::string metadata;
  std::vector<int> debugGlobals;
  std::vector<std::string> exitLabels;
  std::vector<std::pair<std::string, std::string>> loops;
  const Routine* current = nullptr;
  int reg = 0, label = 0, stringId = 0;
  bool debug = false;
  int nextMetadata = 6, debugScope = 0, debugLine = 1;
  std::unordered_map<std::string, int> debugFiles;
  std::map<Type::Kind, int> debugTypes;
  std::unordered_map<const RecordType*, int> debugRecords;
  std::unordered_map<const ArrayType*, int> debugArrays;
  int debugManagedStringObject = 0;
  std::map<std::pair<int, int>, int> debugLocations;
  std::string metadataString(const std::string& value) {
    std::string result;
    for (unsigned char ch : value) {
      if (ch >= 32 && ch <= 126 && ch != '"' && ch != '\\') result += static_cast<char>(ch);
      else { char encoded[4]; std::snprintf(encoded, sizeof(encoded), "\\%02X", ch); result += encoded; }
    }
    return result;
  }
  std::string fileNode(const std::string& path) {
    std::filesystem::path file(path);
    return "!DIFile(filename: \"" + metadataString(file.filename().string()) + "\", directory: \"" + metadataString(file.parent_path().string()) + "\")";
  }
  int fileMetadata(const Routine& routine) {
    auto* root = &routine;
    while (root->parent) root = root->parent;
    const auto& path = root->sourcePath;
    if (auto found = debugFiles.find(path); found != debugFiles.end()) return found->second;
    int id = nextMetadata++;
    debugFiles.emplace(path, id);
    metadata += "!" + std::to_string(id) + " = " + fileNode(path) + "\n";
    return id;
  }
  std::string beginDebug(const Routine& routine, const std::string& name, const std::string& linkage) {
    if (!debug) return "";
    int file = fileMetadata(routine);
    debugScope = nextMetadata++; debugLine = routine.line;
    metadata += "!" + std::to_string(debugScope) + " = distinct !DISubprogram(name: \"" + metadataString(name) + "\", linkageName: \"" + metadataString(linkage) + "\", scope: !" + std::to_string(file) + ", file: !" + std::to_string(file) + ", line: " + std::to_string(routine.line) + ", type: !2, scopeLine: " + std::to_string(routine.line) + ", spFlags: DISPFlagDefinition, unit: !0)\n";
    return " !dbg !" + std::to_string(debugScope);
  }
  int location() {
    auto key = std::make_pair(debugScope, debugLine);
    if (auto found = debugLocations.find(key); found != debugLocations.end()) return found->second;
    int id = nextMetadata++; debugLocations.emplace(key, id);
    metadata += "!" + std::to_string(id) + " = !DILocation(line: " + std::to_string(debugLine) + ", column: 1, scope: !" + std::to_string(debugScope) + ")\n";
    return id;
  }
  struct Layout { std::uint64_t size; std::uint64_t alignment; };
  Layout layout(Type type) {
    if (type == Type::Boolean) return {1, 1};
    if (type == Type::Int32) return {4, 4};
    if (type == Type::Int64 || type == Type::ManagedString || type == Type::Pointer || type == Type::PChar || type.kind == Type::Class || type.kind == Type::Callback) return {8, 8};
    if (type.kind == Type::Real) return {8, 8};
    if (type.kind == Type::Array) {
      auto element = layout(type.array->element);
      auto count = static_cast<std::uint64_t>(type.array->upper - type.array->lower + 1);
      if (!element.alignment || (element.size && count > std::numeric_limits<std::uint64_t>::max() / element.size)) return {0, 0};
      return {element.size * count, element.alignment};
    }
    if (type.kind == Type::Record) {
      Layout result{0, 1};
      for (const auto& field : type.record->fields) {
        auto member = layout(field->type);
        if (!member.alignment || member.size > std::numeric_limits<std::uint64_t>::max() - member.alignment || result.size > std::numeric_limits<std::uint64_t>::max() - member.alignment - member.size) return {0, 0};
        result.size = (result.size + member.alignment - 1) / member.alignment * member.alignment + member.size;
        if (member.alignment > result.alignment) result.alignment = member.alignment;
      }
      if (result.size > std::numeric_limits<std::uint64_t>::max() - result.alignment) return {0, 0};
      result.size = (result.size + result.alignment - 1) / result.alignment * result.alignment;
      return result;
    }
    return {0, 0};
  }
  int debugType(Type type) {
    if (type.kind == Type::Record || type.kind == Type::Array) {
      auto size = layout(type);
      if (!size.alignment || size.size > std::numeric_limits<std::uint64_t>::max() / 8) return 0;
      if (type.kind == Type::Record) {
        if (auto found = debugRecords.find(type.record); found != debugRecords.end()) return found->second;
        int id = nextMetadata++, elements = nextMetadata++;
        debugRecords.emplace(type.record, id);
        std::string members;
        std::uint64_t offset = 0;
        for (const auto& field : type.record->fields) {
          auto member = layout(field->type);
          offset = (offset + member.alignment - 1) / member.alignment * member.alignment;
          int fieldType = debugType(field->type);
          if (fieldType) {
            int fieldId = nextMetadata++;
            if (!members.empty()) members += ", ";
            members += "!" + std::to_string(fieldId);
            metadata += "!" + std::to_string(fieldId) + " = !DIDerivedType(tag: DW_TAG_member, name: \"" + metadataString(field->name) + "\", scope: !" + std::to_string(id) + ", line: " + std::to_string(field->line) + ", baseType: !" + std::to_string(fieldType) + ", size: " + std::to_string(member.size * 8) + ", offset: " + std::to_string(offset * 8) + ")\n";
          }
          offset += member.size;
        }
        metadata += "!" + std::to_string(elements) + " = !{" + members + "}\n";
        metadata += "!" + std::to_string(id) + " = distinct !DICompositeType(tag: DW_TAG_structure_type, name: \"" + metadataString(type.record->name) + "\", size: " + std::to_string(size.size * 8) + ", elements: !" + std::to_string(elements) + ")\n";
        return id;
      }
      if (auto found = debugArrays.find(type.array); found != debugArrays.end()) return found->second;
      int elementType = debugType(type.array->element);
      if (!elementType) return 0;
      int id = nextMetadata++, elements = nextMetadata++, range = nextMetadata++;
      debugArrays.emplace(type.array, id);
      metadata += "!" + std::to_string(range) + " = !DISubrange(count: " + std::to_string(type.array->upper - type.array->lower + 1) + ", lowerBound: " + std::to_string(type.array->lower) + ")\n";
      metadata += "!" + std::to_string(elements) + " = !{!" + std::to_string(range) + "}\n";
      metadata += "!" + std::to_string(id) + " = !DICompositeType(tag: DW_TAG_array_type, baseType: !" + std::to_string(elementType) + ", size: " + std::to_string(size.size * 8) + ", elements: !" + std::to_string(elements) + ")\n";
      return id;
    }
    std::string definition;
    if (type == Type::Int32) definition = "!DIBasicType(name: \"Integer\", size: 32, encoding: DW_ATE_signed)";
    else if (type == Type::Int64) definition = "!DIBasicType(name: \"Int64\", size: 64, encoding: DW_ATE_signed)";
    else if (type.kind == Type::Real) definition = "!DIBasicType(name: \"Real\", size: 64, encoding: DW_ATE_float)";
    else if (type == Type::Boolean) definition = "!DIBasicType(name: \"Boolean\", size: 8, encoding: DW_ATE_boolean)";
    else if (type == Type::ManagedString) {
      if (!debugManagedStringObject) {
        int objectId = nextMetadata++;
        int membersId = nextMetadata++;
        int refId = nextMetadata++;
        int lenId = nextMetadata++;
        int bytesId = nextMetadata++;
        int usizeId = nextMetadata++;
        int charPtrId = nextMetadata++;
        metadata += "!" + std::to_string(usizeId) + " = !DIBasicType(name: \"NativeUInt\", size: 64, encoding: DW_ATE_unsigned)\n";
        metadata += "!" + std::to_string(charPtrId) + " = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !" + std::to_string(nextMetadata++) + ", size: 64)\n";
        // Add the pointed-to char type.
        metadata += "!" + std::to_string(charPtrId + 1) + " = !DIBasicType(name: \"char\", size: 8, encoding: DW_ATE_unsigned_char)\n";
        metadata += "!" + std::to_string(refId) + " = !DIDerivedType(tag: DW_TAG_member, name: \"references\", scope: !" + std::to_string(objectId) + ", baseType: !" + std::to_string(usizeId) + ", size: 64, offset: 0)\n";
        metadata += "!" + std::to_string(lenId) + " = !DIDerivedType(tag: DW_TAG_member, name: \"length\", scope: !" + std::to_string(objectId) + ", baseType: !" + std::to_string(usizeId) + ", size: 64, offset: 64)\n";
        metadata += "!" + std::to_string(bytesId) + " = !DIDerivedType(tag: DW_TAG_member, name: \"bytes\", scope: !" + std::to_string(objectId) + ", baseType: !" + std::to_string(charPtrId) + ", size: 64, offset: 128)\n";
        metadata += "!" + std::to_string(membersId) + " = !{!" + std::to_string(refId) + ", !" + std::to_string(lenId) + ", !" + std::to_string(bytesId) + "}\n";
        metadata += "!" + std::to_string(objectId) + " = distinct !DICompositeType(tag: DW_TAG_structure_type, name: \"String\", size: 192, elements: !" + std::to_string(membersId) + ")\n";
        debugManagedStringObject = objectId;
      }
      definition = "!DIDerivedType(tag: DW_TAG_pointer_type, name: \"String\", baseType: !" + std::to_string(debugManagedStringObject) + ", size: 64)";
    }
    else if (type == Type::Pointer || type == Type::PChar || type.kind == Type::Callback || type.kind == Type::Class)
      definition = "!DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)";
    else return 0;
    if (auto found = debugTypes.find(type.kind); found != debugTypes.end()) return found->second;
    int id = nextMetadata++; debugTypes.emplace(type.kind, id);
    metadata += "!" + std::to_string(id) + " = " + definition + "\n";
    return id;
  }
  void debugVariable(const Routine& routine, const Variable& variable, size_t index, const std::string& slot) {
    if (!debug) return;
    int type = debugType(variable.type);
    if (!type) return;
    int id = nextMetadata++;
    metadata += "!" + std::to_string(id) + " = !DILocalVariable(name: \"" + metadataString(variable.name) + "\", ";
    if (index < routine.parameterCount) metadata += "arg: " + std::to_string(index + 1) + ", ";
    metadata += "scope: !" + std::to_string(debugScope) + ", file: !" + std::to_string(fileMetadata(routine)) + ", line: " + std::to_string(variable.line) + ", type: !" + std::to_string(type) + ")\n";
    debugLine = variable.line;
    auto address = variable.byReference ? instruction("load ptr, ptr " + slot) : slot;
    emit("call void @llvm.dbg.declare(metadata ptr " + address + ", metadata !" + std::to_string(id) + ", metadata !DIExpression())");
  }
  std::string temp() { return "%r" + std::to_string(++reg); }
  std::string newLabel() { return "L" + std::to_string(++label); }
  void emit(const std::string& text) { code += "  " + text + (debug ? ", !dbg !" + std::to_string(location()) : "") + "\n"; }
  void mark(const std::string& name) { code += name + ":\n"; }
  std::string instruction(const std::string& text) { auto name = temp(); emit(name + " = " + text); return name; }
  std::string frameType(const Routine* routine) { return "%frame" + std::to_string(routine->id); }
  std::string frameFor(const Routine* owner) {
    if (owner->isUnit) return "@p64_unit_frame" + std::to_string(owner->id);
    std::string frame = "%frame";
    for (auto scope = current; scope != owner; scope = scope->parent) {
      if (!scope) throw std::logic_error("invalid lexical scope");
      auto parent = instruction("getelementptr " + frameType(scope) + ", ptr " + frame + ", i32 0, i32 0");
      frame = instruction("load ptr, ptr " + parent);
    }
    return frame;
  }
  std::string address(const Variable* variable) {
    if (variable->owner->isUnit) return "@p64_unit_var" + std::to_string(variable->owner->id) + "_" + std::to_string(variable->index);
    auto frame = frameFor(variable->owner);
    auto slot = instruction("getelementptr " + frameType(variable->owner) + ", ptr " + frame + ", i32 0, i32 " + std::to_string(variable->index));
    return variable->byReference ? instruction("load ptr, ptr " + slot) : slot;
  }
  std::string address(const Variable* variable, const std::vector<Selector>& selectors) {
    auto ptr = address(variable);
    for (const auto& selector : selectors) {
      if (selector.index) {
        auto index = convert(expression(*selector.index), Type::Int64);
        auto array = selector.array;
        auto offset = instruction("call i64 @p64_array_index(i64 " + index.name + ", i64 " + std::to_string(array->lower) + ", i64 " + std::to_string(array->upper) + ", i32 " + std::to_string(selector.line) + ")");
        ptr = instruction("getelementptr " + llvmType({Type::Array, nullptr, array}) + ", ptr " + ptr + ", i32 0, i64 " + offset);
      } else {
        auto field = selector.field;
        if (selector.classField) {
          auto object = instruction("load ptr, ptr " + ptr);
          ptr = instruction("call ptr @p64_class_checked(ptr " + object + ", i32 " + std::to_string(selector.line) + ")");
        }
        ptr = instruction("getelementptr " + llvmType({Type::Record, field->owner}) + ", ptr " + ptr + ", i32 0, i32 " + std::to_string(field->index));
      }
    }
    return ptr;
  }

  std::string address(Type type, const std::string& ptr, const std::vector<Selector>& selectors) {
    std::string result = ptr;
    for (const auto& selector : selectors) {
      if (selector.index) {
        auto index = convert(expression(*selector.index), Type::Int64);
        auto array = selector.array;
        auto offset = instruction("call i64 @p64_array_index(i64 " + index.name + ", i64 " + std::to_string(array->lower) + ", i64 " + std::to_string(array->upper) + ", i32 " + std::to_string(selector.line) + ")");
        result = instruction("getelementptr " + llvmType({Type::Array, nullptr, array}) + ", ptr " + result + ", i32 0, i64 " + offset);
        type = array->element;
      } else {
        auto field = selector.field;
        if (selector.classField) {
          auto object = instruction("load ptr, ptr " + result);
          result = instruction("call ptr @p64_class_checked(ptr " + object + ", i32 " + std::to_string(selector.line) + ")");
          type = {Type::Record, type.record};
        }
        result = instruction("getelementptr " + llvmType({Type::Record, field->owner}) + ", ptr " + result + ", i32 0, i32 " + std::to_string(field->index));
        type = field->type;
      }
    }
    return result;
  }

  Value select(Value base, const std::vector<Selector>& selectors, Type finalType) {
    if (selectors.empty()) return base;
    std::string slot = base.slot;
    if (slot.empty()) {
      slot = instruction("alloca " + llvmType(base.type));
      emit("store " + llvmType(base.type) + " " + base.name + ", ptr " + slot);
    }
    auto selectedPtr = address(base.type, slot, selectors);
    auto loaded = instruction("load " + llvmType(finalType) + ", ptr " + selectedPtr);
    Value result{finalType, loaded, needsManagement(finalType)};
    if (result.owned) manageValue(result, true);
    result = track(result);
    release(base);
    return result;
  }
  Value convert(Value value, Type target) {
    if (value.type == target) return value;
    if (value.type == Type::Int32 && target == Type::Int64) return {target, instruction("sext i32 " + value.name + " to i64")};
    if (integer(value.type) && target.kind == Type::Real) {
      auto source = value.type == Type::Int32 ? convert(value, Type::Int64) : value;
      return {target, instruction("sitofp i64 " + source.name + " to double")};
    }
    if (target.kind == Type::Class && value.type.kind == Type::Class) {
      for (auto* record = value.type.record; record; record = record->base) if (record == target.record) return {target, value.name};
    }
    if (target == Type::ManagedString && value.type == Type::String) return track({target, instruction("call ptr @p64_string_new(ptr " + value.name + ")"), true});
    if (target == Type::ManagedString && value.type == Type::PChar) return track({target, instruction("call ptr @p64_string_new(ptr " + value.name + ")"), true});
    if ((target == Type::Pointer || target == Type::PChar || target.kind == Type::Callback || target.kind == Type::Class) && value.type == Type::Nil) return {target, value.name};
    if ((target == Type::PChar && value.type == Type::String) || (target == Type::Pointer && value.type == Type::PChar) ||
        (target == Type::PChar && value.type == Type::Pointer) ||
        (target == Type::Pointer && value.type.kind == Type::Class) ||
        (target.kind == Type::Class && value.type == Type::Pointer))
      return {target, value.name};
    throw std::logic_error("invalid conversion");
  }
  void manageValue(const Value& value, bool retain) {
    if (value.type == Type::ManagedString)
      emit("call void @p64_string_" + std::string(retain ? "retain" : "release") + "(ptr " + value.name + ")");
    else if (needsManagement(value.type))
      emit("call void " + manageName(value.type, retain) + "_value(" + llvmType(value.type) + " " + value.name + ")");
  }
  void manageObject(Type type, const std::string& ptr, bool retain) {
    if (type == Type::ManagedString) {
      auto value = instruction("load ptr, ptr " + ptr);
      emit("call void @p64_string_" + std::string(retain ? "retain" : "release") + "(ptr " + value + ")");
    } else if (needsManagement(type)) emit("call void " + manageName(type, retain) + "(ptr " + ptr + ")");
  }
  Value track(Value value) {
    if (!value.owned) return value;
    value.slot = "%temporary" + std::to_string(temporaries.size());
    temporaries.emplace_back(value.type, value.slot);
    auto type = llvmType(value.type);
    temporaryAllocations += "  " + value.slot + " = alloca " + type + "\n  store " + type + " zeroinitializer, ptr " + value.slot + "\n";
    emit("store " + type + " " + value.name + ", ptr " + value.slot);
    return value;
  }
  void transfer(const Value& value) {
    if (!value.slot.empty()) emit("store " + llvmType(value.type) + " zeroinitializer, ptr " + value.slot);
  }

  void cleanupTemporaries(size_t base, size_t end) {
    for (size_t index = base; index < end; ++index) {
      Type type = temporaries[index].first;
      const std::string& slot = temporaries[index].second;
      if (!needsManagement(type) && type != Type::ManagedString) continue;
      manageObject(type, slot, false);
      emit("store " + llvmType(type) + " zeroinitializer, ptr " + slot);
    }
  }

  void cleanupManagedLocals(const Routine& routine) {
    for (const auto& variable : routine.variables) {
      if (!needsManagement(variable->type)) continue;
      if (variable->byReference) continue;
      if (variable.get() == routine.resultVariable) continue;
      manageObject(variable->type, address(variable.get()), false);
    }
  }
  void release(const Value& value) {
    if (value.owned) { manageValue(value, false); transfer(value); }
  }

  struct BuiltCall {
    std::string symbol;
    std::string args;
    std::vector<Value> transferred;
    std::vector<Value> postRelease;
  };

  BuiltCall buildCallbackCall(const std::string& symbol, const CallbackType& signature,
                             const std::vector<std::unique_ptr<Expr>>& arguments) {
    BuiltCall result;
    for (size_t index = 0; index < arguments.size(); ++index) {
      const auto& parameter = *signature.parameters[index];
      if (!result.args.empty()) result.args += ", ";
      if (parameter.byReference) result.args += "ptr " + address(arguments[index]->variable, arguments[index]->selectors);
      else {
        if (parameter.type == Type::PChar) {
          auto value = expression(*arguments[index]);
          if (value.type == Type::ManagedString) {
            if (!value.owned) { manageValue(value, true); value.owned = true; value = track(value); }
            auto cstr = instruction("call ptr @p64_string_cstr(ptr " + value.name + ")");
            result.args += "ptr " + cstr;
            result.postRelease.push_back(value);
          } else {
            value = convert(value, Type::PChar);
            result.args += "ptr " + value.name;
          }
        } else {
          auto value = convert(expression(*arguments[index]), parameter.type);
          if (needsManagement(value.type) && !value.owned) { manageValue(value, true); value.owned = true; value = track(value); }
          result.args += llvmType(value.type) + " " + value.name;
          if (value.owned) result.transferred.push_back(value);
        }
      }
    }
    result.symbol = symbol;
    return result;
  }

  BuiltCall buildCall(const Routine* routine, const std::vector<std::unique_ptr<Expr>>& arguments, const std::string& receiver = {}, const std::string& dispatch = {}) {
    BuiltCall result;
    result.args = routine->external || !dispatch.empty() ? "" : "ptr " + frameFor(routine->parent);
    if (routine->methodOwner) { if (!result.args.empty()) result.args += ", "; result.args += "ptr " + receiver; }
    for (size_t index = 0; index < arguments.size(); ++index) {
      const auto& parameter = *routine->variables[index + (routine->methodOwner ? 1 : 0)];
      if (!result.args.empty()) result.args += ", ";
      if (parameter.byReference) result.args += "ptr " + address(arguments[index]->variable, arguments[index]->selectors);
      else {
        if (parameter.type == Type::PChar) {
          auto value = expression(*arguments[index]);
          if (value.type == Type::ManagedString) {
            if (!value.owned) { manageValue(value, true); value.owned = true; value = track(value); }
            auto cstr = instruction("call ptr @p64_string_cstr(ptr " + value.name + ")");
            result.args += "ptr " + cstr;
            result.postRelease.push_back(value);
          } else {
            value = convert(value, Type::PChar);
            result.args += "ptr " + value.name;
          }
        } else {
          auto value = convert(expression(*arguments[index]), parameter.type);
          if (needsManagement(value.type) && !value.owned) { manageValue(value, true); value.owned = true; value = track(value); }
          result.args += llvmType(value.type) + " " + value.name;
          if (value.owned) result.transferred.push_back(value);
        }
      }
    }
    result.symbol = dispatch.empty() ? "@" + (routine->external ? routine->externalName : "p64fn" + std::to_string(routine->id)) : dispatch;
    return result;
  }

  void callStatement(const Routine* routine, const std::vector<std::unique_ptr<Expr>>& arguments, const std::string& receiver = {}, const std::string& dispatch = {}) {
    auto built = buildCall(routine, arguments, receiver, dispatch);
    if (tryHandlers.empty()) {
      if (routine->result == Type::Void)
        emit("call void " + built.symbol + "(" + built.args + ")");
      else {
        auto ignored = "%r" + std::to_string(++reg);
        emit(ignored + " = call " + llvmType(routine->result) + " " + built.symbol + "(" + built.args + ")");
      }
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
      return;
    }
    auto normal = newLabel();
    if (routine->result == Type::Void)
      emit("invoke void " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
    else {
      auto ignored = "%r" + std::to_string(++reg);
      emit(ignored + " = invoke " + llvmType(routine->result) + " " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
    }
    mark(normal);
    for (const auto& value : built.transferred) transfer(value);
    for (const auto& value : built.postRelease) release(value);
  }

  void callCallbackStatementSymbol(const std::string& symbol, const CallbackType& signature,
                                   const std::vector<std::unique_ptr<Expr>>& arguments) {
    auto built = buildCallbackCall(symbol, signature, arguments);
    if (tryHandlers.empty()) {
      if (signature.result == Type::Void)
        emit("call void " + built.symbol + "(" + built.args + ")");
      else {
        auto ignored = "%r" + std::to_string(++reg);
        emit(ignored + " = call " + llvmType(signature.result) + " " + built.symbol + "(" + built.args + ")");
      }
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
      return;
    }
    auto normal = newLabel();
    if (signature.result == Type::Void)
      emit("invoke void " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
    else {
      auto ignored = "%r" + std::to_string(++reg);
      emit(ignored + " = invoke " + llvmType(signature.result) + " " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
    }
    mark(normal);
    for (const auto& value : built.transferred) transfer(value);
    for (const auto& value : built.postRelease) release(value);
  }

  void callCallbackStatement(const Variable* callback, const CallbackType& signature,
                             const std::vector<std::unique_ptr<Expr>>& arguments) {
    auto fn = instruction("load ptr, ptr " + address(callback, {}));
    callCallbackStatementSymbol(fn, signature, arguments);
  }

  Value callValue(const Routine* routine, const std::vector<std::unique_ptr<Expr>>& arguments, const std::string& receiver = {}, const std::string& dispatch = {}) {
    auto built = buildCall(routine, arguments, receiver, dispatch);

    std::string result;
    if (tryHandlers.empty()) {
      result = instruction("call " + llvmType(routine->result) + " " + built.symbol + "(" + built.args + ")");
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
    } else {
      auto normal = newLabel();
      result = "%r" + std::to_string(++reg);
      emit(result + " = invoke " + llvmType(routine->result) + " " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
      mark(normal);
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
    }

    Value value{routine->result, result, needsManagement(routine->result)};
    return track(value);
  }

  Value callCallbackValueSymbol(const std::string& symbol, const CallbackType& signature,
                                const std::vector<std::unique_ptr<Expr>>& arguments) {
    auto built = buildCallbackCall(symbol, signature, arguments);

    std::string result;
    if (tryHandlers.empty()) {
      result = instruction("call " + llvmType(signature.result) + " " + built.symbol + "(" + built.args + ")");
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
    } else {
      auto normal = newLabel();
      result = "%r" + std::to_string(++reg);
      emit(result + " = invoke " + llvmType(signature.result) + " " + built.symbol + "(" + built.args + ") to label %" + normal + " unwind label %" + tryHandlers.back());
      mark(normal);
      for (const auto& value : built.transferred) transfer(value);
      for (const auto& value : built.postRelease) release(value);
    }

    Value value{signature.result, result, needsManagement(signature.result)};
    return track(value);
  }

  Value callCallbackValue(const Variable* callback, const CallbackType& signature,
                          const std::vector<std::unique_ptr<Expr>>& arguments) {
    auto fn = instruction("load ptr, ptr " + address(callback, {}));
    return callCallbackValueSymbol(fn, signature, arguments);
  }

  void methodCallStatement(const Routine* method, const Variable* variable, const std::vector<Selector>& selectors,
                           const std::vector<std::unique_ptr<Expr>>& arguments, int line) {
    auto object = instruction("load ptr, ptr " + address(variable, selectors));
    auto checked = instruction("call ptr @p64_class_checked(ptr " + object + ", i32 " + std::to_string(line) + ")");
    std::string dispatch;
    if (method->virtualMethod) {
      auto table = instruction("load ptr, ptr " + checked);
      auto slot = instruction("getelementptr ptr, ptr " + table + ", i64 " + std::to_string(method->virtualIndex + 1));
      dispatch = instruction("load ptr, ptr " + slot);
    }
    callStatement(method, arguments, checked, dispatch);
  }

  Value methodCallValue(const Routine* method, const Variable* variable, const std::vector<Selector>& selectors,
                        const std::vector<std::unique_ptr<Expr>>& arguments, int line) {
    auto object = instruction("load ptr, ptr " + address(variable, selectors));
    auto checked = instruction("call ptr @p64_class_checked(ptr " + object + ", i32 " + std::to_string(line) + ")");
    std::string dispatch;
    if (method->virtualMethod) {
      auto table = instruction("load ptr, ptr " + checked);
      auto slot = instruction("getelementptr ptr, ptr " + table + ", i64 " + std::to_string(method->virtualIndex + 1));
      dispatch = instruction("load ptr, ptr " + slot);
    }
    return callValue(method, arguments, checked, dispatch);
  }
  Value expression(const Expr& expr) {
    debugLine = expr.line;
    switch (expr.kind) {
    case Expr::Kind::Integer: return {expr.type, expr.text};
    case Expr::Kind::Real: return {expr.type, expr.text};
    case Expr::Kind::Boolean: return {expr.type, expr.text == "true" ? "1" : "0"};
    case Expr::Kind::Nil: return {expr.type, "null"};
    case Expr::Kind::NewObject: {
      auto type = llvmType({Type::Record, expr.type.record});
      auto size = "ptrtoint (ptr getelementptr (" + type + ", ptr null, i32 1) to i64)";
      auto object = instruction("call ptr @p64_class_new(i64 " + size + ")");
      auto vtable = instruction("getelementptr " + type + ", ptr " + object + ", i32 0, i32 0");
      emit("store ptr @p64_vtable" + std::to_string(expr.type.record->id) + ", ptr " + vtable);
      if (expr.routine) callStatement(expr.routine, expr.operands, object);
      return {expr.type, object};
    }
    case Expr::Kind::MemberCall: return methodCallValue(expr.routine, expr.variable, expr.selectors, expr.operands, expr.line);
    case Expr::Kind::CallbackCall: {
      auto fn = instruction("load ptr, ptr " + address(expr.variable, expr.selectors));
      const auto* signature = expr.selectors.back().field->type.callback;
      return callCallbackValueSymbol(fn, *signature, expr.operands);
    }
    case Expr::Kind::RoutineAddress: return {expr.type, "@p64cb" + std::to_string(expr.routine->id)};
    case Expr::Kind::Name: {
      auto value = instruction("load " + llvmType(expr.type) + ", ptr " + address(expr.variable, expr.selectors));
      Value result{expr.type, value, needsManagement(expr.type)};
      if (result.owned) manageValue(result, true);
      return track(result);
    }
    case Expr::Kind::Call:
      if (expr.routine) return select(callValue(expr.routine, expr.operands), expr.selectors, expr.type);
      return select(callCallbackValue(expr.variable, *expr.variable->type.callback, expr.operands), expr.selectors, expr.type);
    case Expr::Kind::Unary: {
      auto operand = expression(*expr.operands[0]);
      if (expr.text == "+") return operand;
      if (expr.text == "not") return {expr.type, instruction("xor i1 " + operand.name + ", true")};
      if (expr.type.kind == Type::Real) return {expr.type, instruction("fsub double 0.0, " + operand.name)};
      return {expr.type, instruction("sub " + llvmType(expr.type) + " 0, " + operand.name)};
    }
    case Expr::Kind::Binary: {
      if ((expr.text == "and" || expr.text == "or") && expr.type == Type::Boolean) {
        auto pre = newLabel();
        emit("br label %" + pre);
        mark(pre);

        auto left = expression(*expr.operands[0]);
        auto rhs = newLabel();
        auto end = newLabel();
        bool isAnd = expr.text == "and";
        emit(std::string("br i1 ") + left.name + ", label %" + (isAnd ? rhs : end) + ", label %" + (isAnd ? end : rhs));

        mark(rhs);
        auto right = expression(*expr.operands[1]);
        emit("br label %" + end);

        mark(end);
        std::string shortValue = isAnd ? "false" : "true";
        return {Type::Boolean, instruction("phi i1 [ " + shortValue + ", %" + pre + " ], [ " + right.name + ", %" + rhs + "]")};
      }

      auto left = expression(*expr.operands[0]);
      auto right = expression(*expr.operands[1]);
      if (left.type == Type::String || left.type == Type::ManagedString) {
        left = convert(left, Type::ManagedString); right = convert(right, Type::ManagedString);
        std::string result;
        if (expr.text == "+") result = instruction("call ptr @p64_string_concat(ptr " + left.name + ", ptr " + right.name + ")");
        else {
          auto comparison = instruction("call i32 @p64_string_compare(ptr " + left.name + ", ptr " + right.name + ")");
          std::string predicate = expr.text == "=" ? "eq" : expr.text == "<>" ? "ne" : expr.text == "<" ? "slt" : expr.text == ">" ? "sgt" : expr.text == "<=" ? "sle" : "sge";
          result = instruction("icmp " + predicate + " i32 " + comparison + ", 0");
        }
        release(left); release(right);
        return track({expr.type, result, expr.type == Type::ManagedString});
      }
      if (left.type.kind == Type::Real || right.type.kind == Type::Real || expr.type.kind == Type::Real) {
        left = convert(left, {Type::Real});
        right = convert(right, {Type::Real});
        const auto& operation = expr.text;
        if (operation == "=" || operation == "<>" || operation == "<" || operation == ">" || operation == "<=" || operation == ">=") {
          std::string predicate = operation == "=" ? "oeq" : operation == "<>" ? "one" : operation == "<" ? "olt" : operation == ">" ? "ogt" : operation == "<=" ? "ole" : "oge";
          return {Type::Boolean, instruction("fcmp " + predicate + " double " + left.name + ", " + right.name)};
        }
        std::string opcode = operation == "+" ? "fadd" : operation == "-" ? "fsub" : operation == "*" ? "fmul" : "fdiv";
        return {expr.type, instruction(opcode + " double " + left.name + ", " + right.name)};
      }
      if (left.type == Type::Int64 || right.type == Type::Int64) { left = convert(left, Type::Int64); right = convert(right, Type::Int64); }
      const auto& operation = expr.text;
      if (operation == "=" || operation == "<>" || operation == "<" || operation == ">" || operation == "<=" || operation == ">=") {
        std::string predicate = operation == "=" ? "eq" : operation == "<>" ? "ne" : operation == "<" ? "slt" : operation == ">" ? "sgt" : operation == "<=" ? "sle" : "sge";
        return {Type::Boolean, instruction("icmp " + predicate + " " + llvmType(left.type) + " " + left.name + ", " + right.name)};
      }
      std::string opcode = operation == "+" ? "add" : operation == "-" ? "sub" : operation == "*" ? "mul" : operation == "mod" ? "srem" : operation == "and" ? "and" : operation == "or" ? "or" : "sdiv";
      return {expr.type, instruction(opcode + " " + llvmType(left.type) + " " + left.name + ", " + right.name)};
    }
    case Expr::Kind::String: return {expr.type, stringLiteral(expr.text)};
    }
    throw std::logic_error("unexpected string expression");
  }
  std::string stringLiteral(const std::string& literal) {
    std::string value;
    for (size_t index = 1; index + 1 < literal.size(); ++index) {
      if (literal[index] == '\'' && index + 1 < literal.size() - 1 && literal[index + 1] == '\'') ++index;
      value += literal[index];
    }
    std::string bytes;
    for (unsigned char ch : value) { char encoded[4]; std::snprintf(encoded, sizeof(encoded), "\\%02X", ch); bytes += encoded; }
    std::string name = "@.str" + std::to_string(++stringId);
    globals += name + " = private unnamed_addr constant [" + std::to_string(value.size() + 1) + " x i8] c\"" + bytes + "\\00\"\n";
    return name;
  }
  void statement(const Stmt& stmt) {
    debugLine = stmt.line;
    switch (stmt.kind) {
    case Stmt::Kind::Block: for (auto& child : stmt.statements) statement(*child); break;
    case Stmt::Kind::Assign: {
      auto value = convert(expression(*stmt.expressions[0]), stmt.targetType);
      auto target = address(stmt.variable, stmt.selectors);
      if (needsManagement(stmt.targetType)) {
        if (!value.owned) manageValue(value, true);
        manageObject(stmt.targetType, target, false);
      }
      emit("store " + llvmType(value.type) + " " + value.name + ", ptr " + target);
      transfer(value); break;
    }
    case Stmt::Kind::Call:
	      if (stmt.routine) callStatement(stmt.routine, stmt.expressions);
	      else callCallbackStatement(stmt.variable, *stmt.variable->type.callback, stmt.expressions);
	      break;
    case Stmt::Kind::CallbackCall: {
      auto fn = instruction("load ptr, ptr " + address(stmt.variable, stmt.selectors));
      callCallbackStatementSymbol(fn, *stmt.targetType.callback, stmt.expressions);
      break;
    }
    case Stmt::Kind::MethodCall: methodCallStatement(stmt.routine, stmt.variable, stmt.selectors, stmt.expressions, stmt.line); break;
    case Stmt::Kind::Destroy: {
      auto object = instruction("load ptr, ptr " + address(stmt.variable, stmt.selectors));
      auto present = instruction("icmp ne ptr " + object + ", null");
      auto destroy = newLabel(), end = newLabel();
      emit("br i1 " + present + ", label %" + destroy + ", label %" + end);
      mark(destroy);
      auto vtable = instruction("load ptr, ptr " + object);
      auto destructor = instruction("load ptr, ptr " + vtable);
      emit("call void " + destructor + "(ptr " + object + ")");
      emit("br label %" + end); mark(end); break;
    }
    case Stmt::Kind::Write:
      for (auto& expr : stmt.expressions) {
        if (expr->type == Type::String) emit("call void @p64_write_str(ptr " + stringLiteral(expr->text) + ")");
        else if (expr->type == Type::ManagedString) {
          auto value = expression(*expr); emit("call void @p64_string_write(ptr " + value.name + ")"); release(value);
        } else if (expr->type.kind == Type::Real) {
          auto value = convert(expression(*expr), {Type::Real});
          emit("call void @p64_write_real(double " + value.name + ")");
        } else { auto value = convert(expression(*expr), Type::Int64); emit("call void @p64_write_int(i64 " + value.name + ")"); }
      }
      if (stmt.text == "writeln") emit("call void @p64_newline()");
      break;
    case Stmt::Kind::ReadLn: {
      auto line = track({Type::ManagedString, instruction("call ptr @p64_read_line()"), true});
      if (stmt.expressions.empty()) {
        release(line);
        break;
      }

      const auto& target = *stmt.expressions[0];
      auto ptr = address(target.variable, target.selectors);

      if (target.type == Type::ManagedString) {
        manageObject(Type::ManagedString, ptr, false);
        emit("store ptr " + line.name + ", ptr " + ptr);
        transfer(line);
        break;
      }

      if (target.type == Type::Int32) {
        auto parsed = instruction("call i32 @p64_parse_int32(ptr " + line.name + ", i32 " + std::to_string(target.line) + ")");
        release(line);
        emit("store i32 " + parsed + ", ptr " + ptr);
        break;
      }
      if (target.type == Type::Int64) {
        auto parsed = instruction("call i64 @p64_parse_int64(ptr " + line.name + ", i32 " + std::to_string(target.line) + ")");
        release(line);
        emit("store i64 " + parsed + ", ptr " + ptr);
        break;
      }
      if (target.type.kind == Type::Real) {
        auto parsed = instruction("call double @p64_parse_real(ptr " + line.name + ", i32 " + std::to_string(target.line) + ")");
        release(line);
        emit("store double " + parsed + ", ptr " + ptr);
        break;
      }
      throw std::logic_error("unexpected ReadLn type");
    }
    case Stmt::Kind::If: {
      auto condition = expression(*stmt.expressions[0]); auto yes = newLabel(), no = newLabel(), end = newLabel();
      emit("br i1 " + condition.name + ", label %" + yes + ", label %" + no);
      mark(yes); statement(*stmt.statements[0]); emit("br label %" + end);
      mark(no); if (stmt.statements.size() > 1) statement(*stmt.statements[1]); emit("br label %" + end); mark(end); break;
    }
    case Stmt::Kind::While: {
      auto top = newLabel(), body = newLabel(), end = newLabel(); emit("br label %" + top); mark(top);
      auto condition = expression(*stmt.expressions[0]); emit("br i1 " + condition.name + ", label %" + body + ", label %" + end);
      mark(body); loops.emplace_back(end, top); statement(*stmt.statements[0]); loops.pop_back(); emit("br label %" + top); mark(end); break;
    }
    case Stmt::Kind::Repeat: {
      auto body = newLabel(), conditionLabel = newLabel(), end = newLabel();
      emit("br label %" + body); mark(body);
      loops.emplace_back(end, conditionLabel);
      for (auto& child : stmt.statements) statement(*child);
      loops.pop_back();
      emit("br label %" + conditionLabel); mark(conditionLabel);
      auto condition = expression(*stmt.expressions[0]);
      emit("br i1 " + condition.name + ", label %" + end + ", label %" + body); mark(end); break;
    }
    case Stmt::Kind::For: {
      auto initial = convert(expression(*stmt.expressions[0]), stmt.targetType);
      auto ptr = address(stmt.variable, stmt.selectors); auto type = llvmType(stmt.targetType);
      emit("store " + type + " " + initial.name + ", ptr " + ptr);
      auto bound = convert(expression(*stmt.expressions[2]), stmt.targetType);
      auto top = newLabel(), body = newLabel(), step = newLabel(), end = newLabel(); bool down = stmt.expressions[1]->text == "true";
      emit("br label %" + top); mark(top);
      auto value = instruction("load " + type + ", ptr " + ptr);
      auto test = instruction("icmp " + std::string(down ? "sge " : "sle ") + type + " " + value + ", " + bound.name);
      auto advance = newLabel();
      emit("br i1 " + test + ", label %" + body + ", label %" + end); mark(body);
      loops.emplace_back(end, advance); statement(*stmt.statements[0]); loops.pop_back();
      emit("br label %" + advance); mark(advance);
      auto old = instruction("load " + type + ", ptr " + ptr);
      auto finished = instruction("icmp eq " + type + " " + old + ", " + bound.name);
      emit("br i1 " + finished + ", label %" + end + ", label %" + step); mark(step);
      auto next = instruction(std::string(down ? "sub " : "add ") + type + " " + old + ", 1");
      emit("store " + type + " " + next + ", ptr " + ptr); emit("br label %" + top); mark(end); break;
    }
    case Stmt::Kind::Break: case Stmt::Kind::Continue:
      emit("br label %" + (stmt.kind == Stmt::Kind::Break ? loops.back().first : loops.back().second));
      mark(newLabel());
      break;
    case Stmt::Kind::Exit: {
      if (!stmt.expressions.empty()) {
        auto value = convert(expression(*stmt.expressions[0]), current->result);
        auto target = address(current->resultVariable);
        if (needsManagement(current->result)) {
          if (!value.owned) manageValue(value, true);
          manageObject(current->result, target, false);
        }
        emit("store " + llvmType(value.type) + " " + value.name + ", ptr " + target);
        transfer(value);
      }
      emit("br label %" + exitLabels.back());
      mark(newLabel());
      break;
    }
    case Stmt::Kind::Raise: {
      auto unwind = tryHandlers.empty() ? unwindHandler : tryHandlers.back();
      auto normal = newLabel();
      emit("invoke void @p64_raise() to label %" + normal + " unwind label %" + unwind);
      mark(normal);
      emit("unreachable");
      mark(newLabel());
      break;
    }
    case Stmt::Kind::Try: {
      auto handler = newLabel();
      auto after = newLabel();
      size_t temporariesBase = temporaries.size();
      tryHandlers.push_back(handler);
      statement(*stmt.statements.at(0));
      tryHandlers.pop_back();
      size_t temporariesEnd = temporaries.size();
      emit("br label %" + after);
      mark(handler);
      auto lp = instruction("landingpad { ptr, i32 } catch ptr null");
      auto exn = instruction("extractvalue { ptr, i32 } " + lp + ", 0");
      emit("call ptr @__cxa_begin_catch(ptr " + exn + ")");

      cleanupTemporaries(temporariesBase, temporariesEnd);

      statement(*stmt.statements.at(1));

      emit("call void @__cxa_end_catch()");
      emit("br label %" + after);
      mark(after);
      break;
    }
    case Stmt::Kind::TryFinally: {
      auto handler = newLabel();
      auto after = newLabel();
      size_t temporariesBase = temporaries.size();
      tryHandlers.push_back(handler);
      statement(*stmt.statements.at(0));
      tryHandlers.pop_back();
      size_t temporariesEnd = temporaries.size();

      statement(*stmt.statements.at(1));
      emit("br label %" + after);

      mark(handler);
      auto lp = instruction("landingpad { ptr, i32 } catch ptr null");
      auto exn = instruction("extractvalue { ptr, i32 } " + lp + ", 0");
      emit("call ptr @__cxa_begin_catch(ptr " + exn + ")");

      cleanupTemporaries(temporariesBase, temporariesEnd);
      statement(*stmt.statements.at(1));

      auto unwind = tryHandlers.empty() ? unwindHandler : tryHandlers.back();
      auto normal = newLabel();
      emit("invoke void @__cxa_rethrow() to label %" + normal + " unwind label %" + unwind);
      mark(normal);
      emit("unreachable");

      mark(after);
      break;
    }
    case Stmt::Kind::Empty: break;
    }
  }

  bool usesExceptions(const Stmt* stmt) const {
    if (!stmt) return false;
    if (stmt->kind == Stmt::Kind::Try || stmt->kind == Stmt::Kind::TryFinally || stmt->kind == Stmt::Kind::Raise) return true;
    for (const auto& child : stmt->statements) if (usesExceptions(child.get())) return true;
    for (const auto& expr : stmt->expressions) (void)expr;
    return false;
  }
  void managementFunctions(Type type) {
    if (!needsManagement(type)) return;
    std::string aggregateType = llvmType(type);
    for (bool retain : {true, false}) {
      std::string name = manageName(type, retain);
      functions += "define internal void " + name + "(ptr %object) {\nentry:\n";
      auto appendAction = [&](Type child, const std::string& ptr, const std::string& suffix) {
        if (child == Type::ManagedString) {
          functions += "  %value" + suffix + " = load ptr, ptr " + ptr + "\n";
          functions += "  call void @p64_string_" + std::string(retain ? "retain" : "release") + "(ptr %value" + suffix + ")\n";
        } else if (needsManagement(child)) functions += "  call void " + manageName(child, retain) + "(ptr " + ptr + ")\n";
      };
      if (type.kind == Type::Record) {
        if (type.record->base && needsManagement({Type::Record, type.record->base}))
          functions += "  call void " + manageName({Type::Record, type.record->base}, retain) + "(ptr %object)\n";
        for (size_t index = 0; index < type.record->fields.size(); ++index) {
          Type fieldType = type.record->fields[index]->type;
          if (!needsManagement(fieldType)) continue;
          std::string suffix = std::to_string(index);
          functions += "  %field" + suffix + " = getelementptr " + aggregateType + ", ptr %object, i32 0, i32 " + std::to_string(type.record->fields[index]->index) + "\n";
          appendAction(fieldType, "%field" + suffix, suffix);
        }
        functions += "  ret void\n}\n";
      } else {
        functions += "  br label %loop\nloop:\n  %index = phi i64 [ 0, %entry ], [ %next, %loop ]\n";
        functions += "  %item = getelementptr " + aggregateType + ", ptr %object, i32 0, i64 %index\n";
        appendAction(type.array->element, "%item", "item");
        functions += "  %next = add i64 %index, 1\n  %done = icmp eq i64 %next, " + std::to_string(type.array->upper - type.array->lower + 1) + "\n";
        functions += "  br i1 %done, label %exit, label %loop\nexit:\n  ret void\n}\n";
      }
      functions += "define internal void " + name + "_value(" + aggregateType + " %value) {\nentry:\n";
      functions += "  %slot = alloca " + aggregateType + "\n  store " + aggregateType + " %value, ptr %slot\n";
      functions += "  call void " + name + "(ptr %slot)\n  ret void\n}\n";
    }
  }
  void routine(const Routine& routine) {
    if (routine.external) {
      std::string declaration = "declare " + llvmType(routine.result) + " @" + routine.externalName + "(";
      for (size_t index = 0; index < routine.parameterCount; ++index) {
        if (index) declaration += ", ";
        const auto& parameter = *routine.variables[index];
        declaration += parameter.byReference ? "ptr" : llvmType(parameter.type);
      }
      functions += declaration + ")\n";
      return;
    }
    for (const auto& record : routine.records) {
      types += llvmType({Type::Record, record.get()}) + " = type { ";
      bool first = true;
      if (record->isClass) { types += "ptr"; first = false; }
      std::vector<const RecordType*> ancestry;
      for (auto* ancestor = record.get(); ancestor; ancestor = ancestor->base) ancestry.push_back(ancestor);
      for (auto ancestor = ancestry.rbegin(); ancestor != ancestry.rend(); ++ancestor)
        for (auto& field : (*ancestor)->fields) {
          if (!first) types += ", ";
          types += llvmType(field->type); first = false;
        }
      types += " }\n";
    }
    for (const auto& record : routine.records) managementFunctions({Type::Record, record.get()});
    for (const auto& array : routine.arrays) managementFunctions({Type::Array, nullptr, array.get()});
    for (const auto& record : routine.records) if (record->isClass) {
      auto id = std::to_string(record->id);
      auto count = std::to_string(record->virtualMethods.size() + 1);
      globals += "@p64_vtable" + id + " = private constant [" + count + " x ptr] [ptr @p64_destroy" + id;
      for (auto* method : record->virtualMethods) globals += ", ptr @p64_vm" + std::to_string(method->id);
      globals += "]\n";
      functions += "define internal void @p64_destroy" + id + "(ptr %object) {\nentry:\n";
      for (auto* ancestor = record.get(); ancestor; ancestor = ancestor->base)
        for (auto& method : ancestor->methods) if (method->destructor)
          functions += "  call void @p64_vm" + std::to_string(method->id) + "(ptr %object)\n";
      if (needsManagement({Type::Record, record.get()})) functions += "  call void " + manageName({Type::Record, record.get()}, false) + "(ptr %object)\n";
      functions += "  call void @p64_class_free(ptr %object)\n  ret void\n}\n";
    }
    if (routine.isUnit) {
      types += frameType(&routine) + " = type { ptr }\n";
      globals += "@p64_unit_frame" + std::to_string(routine.id) + " = internal global " + frameType(&routine) + " zeroinitializer\n";
      for (auto& variable : routine.variables) {
        globals += "@p64_unit_var" + std::to_string(routine.id) + "_" + std::to_string(variable->index) + " = internal global " + llvmType(variable->type) + " zeroinitializer";
        int type = debug ? debugType(variable->type) : 0;
        if (type) {
          int declaration = nextMetadata++, expression = nextMetadata++;
          int file = fileMetadata(routine);
          metadata += "!" + std::to_string(declaration) + " = distinct !DIGlobalVariable(name: \"" + metadataString(variable->name) + "\", scope: !" + std::to_string(file) + ", file: !" + std::to_string(file) + ", line: " + std::to_string(variable->line) + ", type: !" + std::to_string(type) + ", isLocal: true, isDefinition: true)\n";
          metadata += "!" + std::to_string(expression) + " = !DIGlobalVariableExpression(var: !" + std::to_string(declaration) + ", expr: !DIExpression())\n";
          globals += ", !dbg !" + std::to_string(expression);
          debugGlobals.push_back(expression);
        }
        globals += "\n";
      }
      current = &routine; reg = 0; label = 0; code.clear(); temporaries.clear(); temporaryAllocations.clear(); unwindHandler.clear();
      std::string initName = "p64_unit_init" + std::to_string(routine.id);
      auto initDebug = beginDebug(routine, routine.name + ".initialization", initName);
      auto initExit = newLabel();
      exitLabels.push_back(initExit);
      if (routine.body) statement(*routine.body);
      emit("br label %" + initExit);
      mark(initExit);
      exitLabels.pop_back();
      emit("ret void");
      functions += "define internal void @" + initName + "() uwtable" + initDebug + " {\nentry:\n" + temporaryAllocations + code + "}\n";
      reg = 0; label = 0; code.clear(); temporaries.clear(); temporaryAllocations.clear(); unwindHandler.clear();
      std::string finiName = "p64_unit_fini" + std::to_string(routine.id);
      auto finiDebug = beginDebug(routine, routine.name + ".finalization", finiName);
      auto finiExit = newLabel();
      exitLabels.push_back(finiExit);
      if (routine.finalization) statement(*routine.finalization);
      emit("br label %" + finiExit);
      mark(finiExit);
      exitLabels.pop_back();
      for (auto& variable : routine.variables)
        if (needsManagement(variable->type)) manageObject(variable->type, address(variable.get()), false);
      emit("ret void");
      functions += "define internal void @" + finiName + "() uwtable" + finiDebug + " {\nentry:\n" + temporaryAllocations + code + "}\n";
      for (auto& child : routine.routines) this->routine(*child);
      for (auto& record : routine.records) for (auto& method : record->methods) this->routine(*method);
      return;
    }
    types += frameType(&routine) + " = type { ptr";
    for (auto& variable : routine.variables) types += ", " + (variable->byReference ? "ptr" : llvmType(variable->type));
    types += " }\n";
    current = &routine; reg = 0; label = 0; code.clear(); temporaries.clear(); temporaryAllocations.clear(); unwindHandler.clear();
    auto debugAttachment = beginDebug(routine, routine.parent ? routine.name : "main", routine.parent ? "p64fn" + std::to_string(routine.id) : "main");
    bool exceptions = usesExceptions(routine.body.get()) || usesExceptions(routine.finalization.get());
    if (exceptions) unwindHandler = newLabel();
    std::string header = routine.parent ? "define " + llvmType(routine.result) + " @p64fn" + std::to_string(routine.id) + "(ptr %parent" : "define i32 @main(i32 %argc, ptr %argv";
    if (routine.parent) for (size_t index = 0; index < routine.parameterCount; ++index) header += ", " + (routine.variables[index]->byReference ? "ptr" : llvmType(routine.variables[index]->type)) + " %arg" + std::to_string(index);
    header += ") uwtable";
    if (exceptions) header += " personality ptr @__gxx_personality_v0";
    header += debugAttachment + " {\nentry:\n";
    emit("%frame = alloca " + frameType(&routine));
    if (!routine.parent) emit("store ptr %frame, ptr @p64_program_frame");
    if (!routine.parent) emit("call void @p64rt_set_args(i32 %argc, ptr %argv)");
    auto parent = instruction("getelementptr " + frameType(&routine) + ", ptr %frame, i32 0, i32 0");
    emit("store ptr " + std::string(routine.parent ? "%parent" : "null") + ", ptr " + parent);
    for (size_t index = 0; index < routine.variables.size(); ++index) {
      auto& variable = *routine.variables[index];
      auto ptr = instruction("getelementptr " + frameType(&routine) + ", ptr %frame, i32 0, i32 " + std::to_string(variable.index));
      std::string initial =
        index < routine.parameterCount ? "%arg" + std::to_string(index) :
        variable.type.kind == Type::Record || variable.type.kind == Type::Array ? "zeroinitializer" :
        variable.type.kind == Type::Pointer || variable.type.kind == Type::PChar || variable.type.kind == Type::Callback || variable.type.kind == Type::ManagedString || variable.type.kind == Type::Class ? "null" :
        variable.type.kind == Type::Real ? "0.0" :
        "0";
      emit("store " + (variable.byReference ? "ptr" : llvmType(variable.type)) + " " + initial + ", ptr " + ptr);
      debugVariable(routine, variable, index, ptr);
    }
    if (!routine.parent) for (auto& unit : routine.units) emit("call void @p64_unit_init" + std::to_string(unit->id) + "()");

    auto exitLabel = newLabel();
    exitLabels.push_back(exitLabel);

    statement(*routine.body);
    emit("br label %" + exitLabel);
    mark(exitLabel);

    exitLabels.pop_back();

    if (!routine.parent) for (auto unit = routine.units.rbegin(); unit != routine.units.rend(); ++unit) emit("call void @p64_unit_fini" + std::to_string((*unit)->id) + "()");
    for (auto& variable : routine.variables) {
      if (needsManagement(variable->type) && !variable->byReference && variable.get() != routine.resultVariable)
        manageObject(variable->type, address(variable.get()), false);
    }
    if (!routine.parent) emit("ret i32 0");
    else if (routine.result == Type::Void) emit("ret void");
    else { auto ptr = address(routine.resultVariable); auto value = instruction("load " + llvmType(routine.result) + ", ptr " + ptr); emit("ret " + llvmType(routine.result) + " " + value); }

    if (exceptions) {
      mark(unwindHandler);
      auto lp = instruction("landingpad { ptr, i32 } cleanup");
      cleanupTemporaries(0, temporaries.size());
      cleanupManagedLocals(routine);
      emit("resume { ptr, i32 } " + lp);
    }
    functions += header + temporaryAllocations + code + "}\n";
    if (routine.addressTaken || (routine.methodOwner && (routine.virtualMethod || routine.destructor))) {
      std::string parameters, arguments = "ptr %parent";
      for (size_t index = 0; index < routine.parameterCount; ++index) {
        const auto& parameter = *routine.variables[index];
        std::string type = parameter.byReference ? "ptr" : llvmType(parameter.type);
        if (index) parameters += ", ";
        parameters += type + " %arg" + std::to_string(index);
        arguments += ", " + type + " %arg" + std::to_string(index);
      }
      std::string resultType = llvmType(routine.result);
      std::string thunk = routine.addressTaken ? "p64cb" : "p64_vm";
      functions += "define internal " + resultType + " @" + thunk + std::to_string(routine.id) + "(" + parameters + ") {\nentry:\n";
      if (routine.parent->isUnit)
        functions += "  %parent = getelementptr " + frameType(routine.parent) + ", ptr @p64_unit_frame" + std::to_string(routine.parent->id) + ", i32 0\n";
      else functions += "  %parent = load ptr, ptr @p64_program_frame\n";
      functions += routine.result == Type::Void ? "  call void " : "  %value = call " + resultType + " ";
      functions += "@p64fn" + std::to_string(routine.id) + "(" + arguments + ")\n";
      functions += routine.result == Type::Void ? "  ret void\n}\n" : "  ret " + resultType + " %value\n}\n";
    }
    for (auto& child : routine.routines) this->routine(*child);
    for (auto& record : routine.records) for (auto& method : record->methods) this->routine(*method);
  }
public:
  explicit Generator(bool includeDebug) : debug(includeDebug) {}
  std::string run(const Routine& program) {
    if (debug) {
      ++nextMetadata;
      debugFiles.emplace(program.sourcePath, 1);
      metadata = "!llvm.dbg.cu = !{!0}\n!llvm.module.flags = !{!3, !4}\n";
      metadata += "!0 = distinct !DICompileUnit(language: DW_LANG_Pascal83, file: !1, producer: \"Pascal64\", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !6)\n";
      metadata += "!1 = " + fileNode(program.sourcePath) + "\n!2 = !DISubroutineType(types: !5)\n!3 = !{i32 2, !\"Dwarf Version\", i32 4}\n!4 = !{i32 2, !\"Debug Info Version\", i32 3}\n!5 = !{}\n";
    }
    for (const auto& unit : program.units) routine(*unit);
    routine(program);
    if (debug) {
      metadata += "!6 = !{";
      for (size_t index = 0; index < debugGlobals.size(); ++index) metadata += (index ? ", !" : "!") + std::to_string(debugGlobals[index]);
      metadata += "}\n";
    }
    return "; Pascal64\n" + types + "@p64_program_frame = internal global ptr null\n" + globals +
           "declare void @p64_write_int(i64)\n" \
           "declare void @p64_write_real(double)\n" \
           "declare void @p64_write_str(ptr)\n" \
           "declare void @p64_newline()\n" \
           "declare i64 @p64_array_index(i64, i64, i64, i32)\n" \
           "declare ptr @p64_string_new(ptr)\n" \
           "declare ptr @p64_string_cstr(ptr)\n" \
           "declare void @p64_string_retain(ptr)\n" \
           "declare void @p64_string_release(ptr)\n" \
           "declare ptr @p64_string_concat(ptr, ptr)\n" \
           "declare i32 @p64_string_compare(ptr, ptr)\n" \
           "declare void @p64_string_write(ptr)\n" \
           "declare ptr @p64_read_line()\n" \
           "declare i32 @p64_parse_int32(ptr, i32)\n" \
           "declare i64 @p64_parse_int64(ptr, i32)\n" \
           "declare double @p64_parse_real(ptr, i32)\n" \
           "declare ptr @p64_class_new(i64)\n" \
           "declare ptr @p64_class_checked(ptr, i32)\n" \
           "declare void @p64_class_free(ptr)\n" \
           "declare void @p64rt_set_args(i32, ptr)\n" \
           "declare void @p64_raise()\n" \
           "declare i32 @__gxx_personality_v0(...)\n" \
           "declare ptr @__cxa_begin_catch(ptr)\n" \
           "declare void @__cxa_rethrow()\n" \
           "declare void @__cxa_end_catch()\n" +
           (debug ? "declare void @llvm.dbg.declare(metadata, metadata, metadata)\n" : "") + functions + metadata;
  }
};
}
std::string generate(const Routine& program, bool debug) { return Generator(debug).run(program); }
