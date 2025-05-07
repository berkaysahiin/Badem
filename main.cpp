#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include <llvm-14/llvm/Support/raw_ostream.h>

using namespace clang;

class MethodBodyVisitor : public RecursiveASTVisitor<MethodBodyVisitor> {
public:
  explicit MethodBodyVisitor(ASTContext *Context) : Context(Context) {}

  bool VisitCXXMethodDecl(CXXMethodDecl *MD) {
    if (MD->hasBody()) {
      CXXRecordDecl *ParentClass = MD->getParent();
      std::string ClassName = ParentClass->getNameAsString();
      
      std::string MethodName = MD->getNameAsString();
      
      SourceManager &SM = Context->getSourceManager();
      SourceLocation Loc = MD->getLocation();
      unsigned LineNum = SM.getSpellingLineNumber(Loc);
      
      llvm::outs() << "Class: " << ClassName << ", Method: " << MethodName 
                  << " defined at line " << LineNum << "\n";
      
      Stmt *Body = MD->getBody();
      if (Body) {
        llvm::outs() << "  Body: ";
        Body->printPretty(llvm::outs(), nullptr, PrintingPolicy(Context->getLangOpts()));
        llvm::outs() << "\n\n";

        analyzeMethodBody(Body, ClassName, MethodName);
      }
    }
    return true;
  }
  
  void analyzeMethodBody(Stmt *Body, const std::string &ClassName, const std::string &MethodName) {
    unsigned ifCount = 0, forCount = 0, callCount = 0;
    
    class BodyStmtVisitor : public RecursiveASTVisitor<BodyStmtVisitor> {
    public:
      unsigned &IfCount;
      unsigned &ForCount;
      unsigned &CallCount;
      
      BodyStmtVisitor(unsigned &ifCount, unsigned &forCount, unsigned &callCount)
        : IfCount(ifCount), ForCount(forCount), CallCount(callCount) {}
      
      bool VisitIfStmt(IfStmt *) { IfCount++; return true; }
      bool VisitForStmt(ForStmt *) { ForCount++; return true; }
      bool VisitCallExpr(CallExpr *) { CallCount++; return true; }
    };
    
    BodyStmtVisitor bodyVisitor(ifCount, forCount, callCount);
    bodyVisitor.TraverseStmt(Body);
    
    llvm::outs() << "  Analysis of " << ClassName << "::" << MethodName << ":\n"
                << "    - If statements: " << ifCount << "\n"
                << "    - For loops: " << forCount << "\n"
                << "    - Function calls: " << callCount << "\n";
  }

private:
  ASTContext *Context;
};

// Standard AST consumer setup
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
    
    clang::tooling::CommonOptionsParser& OP = ExpectedParser.get();
    
    clang::tooling::ClangTool Tool(OP.getCompilations(), OP.getSourcePathList());
    
    return Tool.run(clang::tooling::newFrontendActionFactory<MethodBodyAction>().get());
  }
  return 0;
}
