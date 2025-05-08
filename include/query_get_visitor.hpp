#pragma once

#include <set>
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;

class QueryGetVisitor : public RecursiveASTVisitor<QueryGetVisitor> {
public:
  QueryGetVisitor(ASTContext *context, std::set<const FunctionDecl *> &visited)
      : Context(context), Visited(visited) {}

  bool VisitCallExpr(CallExpr *call);

private:

  void processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl);
  void visitCalledFunctionBody(const FunctionDecl *funcDecl);
  void detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const std::string &qualifiedName);
  bool isVariantBasePointer(QualType type);
  void printTemplateArguments(const FunctionDecl *funcDecl);
  void printTemplateArgument(const TemplateArgument &arg);

  ASTContext *Context;
  std::set<const FunctionDecl *> &Visited;
};
