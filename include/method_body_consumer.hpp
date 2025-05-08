#pragma once

#include "clang/AST/ASTConsumer.h"

#include "method_body_visitor.hpp"

using namespace clang;

class MethodBodyConsumer : public ASTConsumer {
public:
  explicit MethodBodyConsumer(ASTContext *Context) : Visitor(Context) {}

  virtual void HandleTranslationUnit(ASTContext &context) override;

private:
  MethodBodyVisitor Visitor;
};
