#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <vector>
#include <string>

namespace {
class StaticReflectionASTVisitor;
class StaticReflectionASTConsumer;
class StaticReflectionAction;

struct StaticReflectionCommandLineConfig {
};

class StaticReflectionASTVisitor: public clang::RecursiveASTVisitor<StaticReflectionASTVisitor> {
  StaticReflectionASTConsumer* Consumer;
  clang::ASTContext& Ctx;
public:
  StaticReflectionASTVisitor(StaticReflectionASTConsumer* Consumer_, clang::ASTContext& Ctx_): Consumer(Consumer_), Ctx(Ctx_) {
  }
  void run() {
    TraverseDecl(Ctx.getTranslationUnitDecl());
  }
};

class StaticReflectionASTConsumer: public clang::ASTConsumer {
  clang::CompilerInstance& CI;
  llvm::StringRef InFile;
  StaticReflectionCommandLineConfig config;
  clang::ASTContext* Context;
  void reset() {
  }
public:
  StaticReflectionASTConsumer(clang::CompilerInstance& CI_, llvm::StringRef InFile_, StaticReflectionCommandLineConfig config_): CI(CI_), InFile(InFile_), config(config_) {
  }
  virtual ~StaticReflectionASTConsumer() {
    llvm::outs() << "~StaticReflectionASTConsumer()\n";
  }
  virtual void Initialize(clang::ASTContext& Context) override {
    reset();
    this->Context = &Context;
  }
  virtual void HandleTranslationUnit(clang::ASTContext& Ctx) override {
    StaticReflectionASTVisitor(this, Ctx).run();
  }
};

class StaticReflectionAction : public clang::PluginASTAction {
  StaticReflectionCommandLineConfig config;
  bool ParseCommandLineConfig(const clang::CompilerInstance& CI, const std::vector<std::string>& args) {
    (void) CI;
    (void) args;
    return true;
  }
public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, llvm::StringRef InFile) override {
    return std::make_unique<StaticReflectionASTConsumer>(CI, InFile, config);
  }
  bool ParseArgs(const clang::CompilerInstance &CI, const std::vector<std::string>& args) override {
    return ParseCommandLineConfig(CI, args);
  }
};

static clang::FrontendPluginRegistry::Add<StaticReflectionAction> X("static-reflection", "a tool to do compile time reflection");
}
