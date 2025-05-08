#include "query_visitor.hpp"
#include "clang/Basic/SourceManager.h"
#include <llvm-14/llvm/Support/raw_ostream.h>
#include <string_view>

bool QueryVisitor::VisitCXXMethodDecl(CXXMethodDecl *methodDecl) {
    if (!methodDecl->hasBody() || !methodDecl->isUserProvided()) {
        return true;
    }

    std::string methodName = methodDecl->getNameInfo().getName().getAsString();

    llvm::outs() << "Will Analyze method: " << methodName << "\n";

    if (kTrackedMethods.find(methodName) == kTrackedMethods.end()) {
        return true;
    }

    CXXRecordDecl *parentClass = methodDecl->getParent();
    currentVariantClass = parentClass->getNameAsString();


    analyzeMethod(methodDecl);

    return true;
}

void QueryVisitor::analyzeMethod(CXXMethodDecl *md) {
    TraverseStmt(md->getBody());
}

bool QueryVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();

  if (!funcDecl) {
    return true;
  }

  if(skipFunction(funcDecl)) {
    return true;
  }

  bool foundQuery = processCurrentCall(call, funcDecl);

  if(!foundQuery && !skipFunction(funcDecl)) {
      visitCalledFunctionBody(funcDecl);
  }

  return true;
}

bool QueryVisitor::processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl) {
  const std::string_view qualifiedName = funcDecl->getQualifiedNameAsString();
  bool isGetQuery = qualifiedName.find(kQueryGet) != std::string_view::npos;
  bool isReadQuery = qualifiedName.find(kQueryRead) != std::string_view::npos;

  if(isGetQuery || isReadQuery) {
    detectVariantBaseQueryCalls(call, funcDecl, isReadQuery);
  }

  return false;
}

bool QueryVisitor::skipFunction(const FunctionDecl* fd) {
  const std::string_view qualifiedName = fd->getQualifiedNameAsString();
  bool isGetQuery = qualifiedName.find(kQueryGet) != std::string_view::npos;
  bool isReadQuery = qualifiedName.find(kQueryRead) != std::string_view::npos;

  if(isGetQuery || isReadQuery) {
    return false;
  }

  SourceLocation loc = fd->getLocation();
  
  if (loc.isInvalid()) {
      return true;
  }
  
  if (Context->getSourceManager().isInSystemHeader(loc)) {
      return true;
  }
  
  FileID fileID = Context->getSourceManager().getFileID(loc);
  const FileEntry* fileEntry = Context->getSourceManager().getFileEntryForID(fileID);
  
  if (!fileEntry) {
      return true;
  }
  
  llvm::StringRef filePath = fileEntry->getName();

  if (filePath.find("/game/") != llvm::StringRef::npos) {
      return false;  
  }
  
  return true;
}

void QueryVisitor::visitCalledFunctionBody(const FunctionDecl *funcDecl) {
  if(skipFunction(funcDecl)) {
    return;
  }

  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (!funcDecl->hasBody() || !funcDecl->isUserProvided()) {
      return;
  }

  if (Visited.insert(funcDecl).second) {
      TraverseStmt(funcDecl->getBody());
  }
}

void QueryVisitor::detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const bool isReadQuery) {
  if (call->getNumArgs() < 1) { 
    return;
  }

  const Expr* arg = call->getArg(0);

  if (!isVariantBasePointer(arg->getType())) { 
      return;
  }

  const auto *specInfo = funcDecl->getTemplateSpecializationInfo();
  if (!specInfo)
    return;

  const TemplateArgumentList *templateArgs = specInfo->TemplateArguments;
  if (!templateArgs || templateArgs->size() == 0) {
      return;
  }

  auto& dependencyMap = isReadQuery ? variantReadDependency : variantWriteDependency;
  auto& currentSet = dependencyMap[currentVariantClass];

  for (unsigned i = 0; i < templateArgs->size(); ++i) {
    processTemplateArgument(templateArgs->get(i), currentSet);
  }

}

void QueryVisitor::processTemplateArgument(const TemplateArgument &arg, std::set<std::string>& dependencySet) {
  if (arg.getKind() == TemplateArgument::Type) {
      dependencySet.insert(arg.getAsType().getAsString());
  }
  else if (arg.getKind() == TemplateArgument::Pack) {
    for (const auto &packArg : arg.pack_elements()) {
      if (packArg.getKind() == TemplateArgument::Type) {
          dependencySet.insert(packArg.getAsType().getAsString());
      }
    }
  }
}

bool QueryVisitor::isVariantBasePointer(QualType type) {
  if (!type->isPointerType()) {
      return false;
  }

  const QualType pointee = type->getPointeeType();
  const CXXRecordDecl* record = pointee->getAsCXXRecordDecl();

  return record && record->getNameAsString() == kVariantBase;
}

void QueryVisitor::writeDependenciesToCSV() {
  if(currentVariantClass.empty()) {
        return;
  }

    std::string filename = currentVariantClass + ".astrequires";
    std::ofstream csvFile(filename);
    
    if (!csvFile.is_open()) {
        llvm::errs() << "Error: Could not open file " << filename << " for writing\n";
        return;
    }
    
    csvFile << "Write,";
    
    for (const auto &pair : variantWriteDependency) {
        const std::string &variant = pair.first;
        for (const std::string &dependency : pair.second) {
            csvFile << dependency << ",";
        }
    }
    
    csvFile << "\n";
    csvFile << "Read,";

    for (const auto &pair : variantReadDependency) {
        const std::string &variant = pair.first;
        for (const std::string &dependency : pair.second) {
            csvFile << dependency << ",";
        }
    }

    csvFile << "\n";
    
    csvFile.close();
    llvm::outs() << "Dependencies written to " << filename << "\n";
}
