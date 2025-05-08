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

  ~QueryVisitor() {
    if(currentVariantClass.empty()) {
      return;
    }

    if(variantWriteDependency[currentVariantClass].empty()) {
      return;
    }
      std::string filename = currentVariantClass + ".astrequires";
      std::ofstream outFile(filename);
      
      if (!outFile.is_open()) {
        llvm::errs() << "Error: Could not open file " << filename << " for writing\n";
        return;
      }
      
      for (const auto& [variant, dependencies] : variantWriteDependency) {
        outFile << "Variant: " << variant << "\n";
        outFile << "Dependencies:\n";
        for (const auto& dep : dependencies) {
          outFile << "  - " << dep << "\n";
        }
        outFile << "\n";
      }
      
      outFile.close();
      llvm::outs() << "Variant dependencies written to " << filename << "\n";
  }

  bool VisitCXXMethodDecl(CXXMethodDecl *methodDecl);
  bool VisitCallExpr(CallExpr *call);

private:
  void analyzeMethod(CXXMethodDecl* md);
  bool processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl);
  void visitCalledFunctionBody(const FunctionDecl *funcDecl);
  bool skipFunction(const FunctionDecl* fd);
  void detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const std::string &qualifiedName);
  bool isVariantBasePointer(QualType type);
  void printTemplateArguments(const FunctionDecl *funcDecl);
  void printTemplateArgument(const TemplateArgument &arg);

  ASTContext *Context;
  std::set<const FunctionDecl *> Visited;

  std::string currentVariantClass;
  std::unordered_map<std::string, std::set<std::string>> variantWriteDependency;
};
