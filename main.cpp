#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include <llvm-14/llvm/Support/raw_ostream.h>
#include <set>

using namespace clang;

class QueryGetVisitor;

class MethodBodyVisitor : public RecursiveASTVisitor<MethodBodyVisitor> {
public:
  explicit MethodBodyVisitor(ASTContext *context) : Context(context) {}

  bool VisitCXXMethodDecl(CXXMethodDecl *methodDecl);
  void analyzeMethodBody(Stmt *Body, const std::string &ClassName, const std::string &MethodName);

private:
  ASTContext *Context;
};

class QueryGetVisitor : public RecursiveASTVisitor<QueryGetVisitor> {
public:
  QueryGetVisitor(ASTContext *context, std::set<const FunctionDecl *> &visited)
      : Context(context), Visited(visited) {}

  bool VisitCallExpr(CallExpr *call);

private:
  ASTContext *Context;
  std::set<const FunctionDecl *> &Visited;
};

bool MethodBodyVisitor::VisitCXXMethodDecl(CXXMethodDecl *methodDecl) {
  if (!methodDecl->hasBody() || !methodDecl->isUserProvided())
    return true;

  std::string methodName = methodDecl->getNameInfo().getName().getAsString();

  if (methodName != "on_init" &&
      methodName != "on_post_init" &&
      methodName != "on_update" &&
      methodName != "on_play_update" &&
      methodName != "on_play_start" &&
      methodName != "on_play_late_start") {
    return true;
  }

  CXXRecordDecl *parentClass = methodDecl->getParent();

  bool isVariant = false;
  for (const auto &base : parentClass->bases()) {
    const Type *baseType = base.getType().getTypePtrOrNull();
    if (!baseType)
      continue;
    const CXXRecordDecl *baseDecl = baseType->getAsCXXRecordDecl();
    if (!baseDecl)
      continue;

    if (baseDecl->getNameAsString() == "VariantBase") {
      isVariant = true;
      break;
    }
  }

  if (!isVariant)
    return true;

  std::string className = parentClass->getNameAsString();
  Stmt *body = methodDecl->getBody();
  if (body)
    analyzeMethodBody(body, className, methodName);

  return true;
}

void MethodBodyVisitor::analyzeMethodBody(Stmt *Body, const std::string &ClassName, const std::string &MethodName) {
  std::set<const FunctionDecl *> visited;
  QueryGetVisitor visitor(Context, visited);
  visitor.TraverseStmt(Body);
}

bool QueryGetVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();
  if (!funcDecl)
    return true;

  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (qualifiedName.compare(0, 5, "std::") == 0 || qualifiedName.compare(0, 6, "rttr::") == 0)
    return true;

  if (qualifiedName.find("Query::get") != std::string::npos) {
    if (call->getNumArgs() == 1) {
      Expr *arg = call->getArg(0);
      QualType argType = arg->getType();

      if (argType->isPointerType()) {
        QualType pointee = argType->getPointeeType();
        if (const CXXRecordDecl *record = pointee->getAsCXXRecordDecl()) {
          if (record->getNameAsString() == "VariantBase") {
            llvm::outs() << "    - Detected Query::get with VariantBase* at: "
                       << qualifiedName << "\n";

            if (const auto *specInfo = funcDecl->getTemplateSpecializationInfo()) {
              const TemplateArgumentList *templateArgs = specInfo->TemplateArguments;
              if (templateArgs && templateArgs->size() > 0) {
                llvm::outs() << "      - Template argument(s): ";
                for (unsigned i = 0; i < templateArgs->size(); ++i) {
                  const TemplateArgument &arg = templateArgs->get(i);
                  if (arg.getKind() == TemplateArgument::Type) {
                    llvm::outs() << arg.getAsType().getAsString() << " ";
                  } else if (arg.getKind() == TemplateArgument::Pack) {
                    for (const auto &packArg : arg.pack_elements()) {
                      if (packArg.getKind() == TemplateArgument::Type) {
                        llvm::outs() << packArg.getAsType().getAsString() << " ";
                      }
                    }
                  }
                }
                llvm::outs() << "\n";
              }
            }
          }
        }
      }
    }
  }

  if (funcDecl->hasBody() && funcDecl->isUserProvided()) {
    if (Visited.insert(funcDecl).second) {
      Stmt *calleeBody = funcDecl->getBody();
      QueryGetVisitor subVisitor(Context, Visited);
      subVisitor.TraverseStmt(calleeBody);
    }
  }

  return true;
}

class MethodBodyConsumer : public ASTConsumer {
public:
  explicit MethodBodyConsumer(ASTContext *Context) : Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  MethodBodyVisitor Visitor;
};

class MethodBodyAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &CI, StringRef file) override {
    return std::make_unique<MethodBodyConsumer>(&CI.getASTContext());
  }
};

int main(int argc, const char **argv) {
  if (argc > 1) {
    static llvm::cl::OptionCategory MethodAnalyzerCategory("method-analyzer options");

    auto ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MethodAnalyzerCategory);
    if (!ExpectedParser) {
      llvm::errs() << ExpectedParser.takeError();
      return 1;
    }

    clang::tooling::CommonOptionsParser &OP = ExpectedParser.get();
    clang::tooling::ClangTool Tool(OP.getCompilations(), OP.getSourcePathList());

    return Tool.run(clang::tooling::newFrontendActionFactory<MethodBodyAction>().get());
  }

  return 0;
}
