#pragma once

#include <set>
#include <fstream>
#include <unordered_map>
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;

enum class QueryType {
  None,
  Get,
  Read,
  Length
};

class QueryVisitor : public RecursiveASTVisitor<QueryVisitor> {
public:
  explicit QueryVisitor(ASTContext *context) 
    : Context(context) {}


  bool VisitCXXMethodDecl(CXXMethodDecl *methodDecl);
  bool VisitCallExpr(CallExpr *call);
  void writeDependenciesToCSV();

private:
  void analyzeMethod(CXXMethodDecl* md);
  bool processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl);
  void visitCalledFunctionBody(const FunctionDecl *funcDecl);
  bool skipFunction(const FunctionDecl* fd);
  void detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const std::string &qualifiedName);
  bool isVariantBasePointer(QualType type);
  void processTemplateArgument(const TemplateArgument &arg, bool isReadQuery);

  ASTContext *Context;
  std::set<const FunctionDecl *> Visited;

  std::string currentVariantClass;
  std::unordered_map<std::string, std::set<std::string>> variantWriteDependency;
  std::unordered_map<std::string, std::set<std::string>> variantReadDependency; 
};
