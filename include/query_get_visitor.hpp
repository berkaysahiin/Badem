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
  ASTContext *Context;
  std::set<const FunctionDecl *> &Visited;
};

