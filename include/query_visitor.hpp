#pragma once

#include <set>
#include <fstream>
#include <unordered_map>
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;

class QueryVisitor : public RecursiveASTVisitor<QueryVisitor> {
public:
  explicit QueryVisitor(ASTContext *context) 
    : Context(context) {}


  bool VisitCXXMethodDecl(CXXMethodDecl *methodDecl);
  bool VisitCallExpr(CallExpr *call);
  void writeDependenciesToCSV();

private:
  void analyzeMethod(CXXMethodDecl* md);
  bool isQuery(const FunctionDecl *funcDecl);
  bool processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl);
  void visitCalledFunctionBody(const FunctionDecl *funcDecl);
  bool skipFunction(const FunctionDecl* fd);
  void detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const bool isReadQuery);
  bool isVariantBasePointer(QualType type);
  void processTemplateArgument(const TemplateArgument &arg, std::set<std::string>& dependencySet);

  ASTContext *Context;
  std::set<const FunctionDecl *> Visited;

  std::string currentVariantClass;
  std::unordered_map<std::string, std::set<std::string>> variantWriteDependency;
  std::unordered_map<std::string, std::set<std::string>> variantReadDependency; 

  static constexpr std::string_view kVariantBase = "VariantBase";
  static constexpr std::string_view kQueryGet = "Query::get";
  static constexpr std::string_view kQueryRead = "Query::read";
  static constexpr std::string_view kQuery = "Query::";

  static inline const std::set<std::string_view> kTrackedMethods = {
    "on_init", "on_post_init", "on_update", 
    "on_play_update", "on_play_start", "on_play_late_start"
  };

};
