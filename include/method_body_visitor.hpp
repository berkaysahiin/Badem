#pragma once

#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;

class MethodBodyVisitor : public RecursiveASTVisitor<MethodBodyVisitor> {
public:
  explicit MethodBodyVisitor(ASTContext *context) : Context(context) {}

  bool VisitCXXMethodDecl(CXXMethodDecl*);
  void analyzeMethodBody(Stmt*);

private:
  ASTContext *Context;
};

