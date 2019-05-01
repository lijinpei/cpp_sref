#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/raw_ostream.h"
#include "fmt/format.h"

#include <iostream>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <string_view>

namespace {
class StaticReflectionASTVisitor;
class StaticReflectionASTConsumer;
class StaticReflectionAction;

struct StaticReflectionCommandLineConfig {
  int indent = 2;
};

class StaticReflectionASTVisitor {
  StaticReflectionASTConsumer* consumer;
  clang::ASTContext& ctx;
  StaticReflectionCommandLineConfig config;
  fmt::memory_buffer out;
  std::vector<clang::DeclContext*> current_namespace;
  int current_indent;
  bool visitDecl(clang::Decl* D) {
    if (!ctx.getSourceManager().isInMainFile(D->getLocation())) {
      return true;
    }
    //fmt::format_to(out, "VisitDecl({} {})\n", D->getKind(), D->getDeclKindName());
    if (auto* TD = llvm::dyn_cast<clang::TypeDecl>(D)) {
      fmt::format_to(out, "TypeDecl({})\n", TD->getNameAsString());
      auto* decl_cont = TD->getDeclContext();
      for (auto* DC = TD->getDeclContext(); DC; DC = DC->getParent()) {
        if (auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(DC)) {
          fmt::format_to(out, "nested {}\n", ND->getNameAsString());
        } else {
          fmt::format_to(out, "nested not namespace\n");
        }
      }
    }
    return true;
  }
  void inc_indent() {
    current_indent += config.indent;
  }
  void dec_indent() {
    current_indent -= config.indent;
  }
  void indent() {
    // TODO: more efficient
    for (int i = 0; i < current_indent; ++i) {
      fmt::format_to(out, " ");
    }
  }
  bool is_namespace_like(clang::DeclContext* DC) {
    auto kind = DC->getDeclKind();
    auto ret = kind == clang::Decl::Kind::LinkageSpec || kind == clang::Decl::Kind::Namespace || kind == clang::Decl::Kind::TranslationUnit;
    if (!ret) {
      llvm::outs() << "not namespace like " << DC->getDeclKindName() << '\n';
      //DC->dumpDeclContext();
    }
    return ret;
  }
  void open_namespace(clang::DeclContext* DC) {
    assert(is_namespace_like(DC));
    auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(DC);
    indent();
    if (ND->isInline()) {
      fmt::format_to(out, "inline ");
    }
    if (ND->isAnonymousNamespace()) {
      fmt::format_to(out, "namespace {{\n");
    } else {
      auto sr = ND->getName();
      fmt::format_to(out, "namespace {} {{\n", std::string_view(sr.data(), sr.size()));
    }
    inc_indent();
  }
  void close_namespace(clang::DeclContext* DC) {
    assert(is_namespace_like(DC));
    auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(DC);
    dec_indent();
    indent();
    fmt::format_to(out, "}} // ");
    if (ND->isInline()) {
      fmt::format_to(out, "inline ");
    }
    if (ND->isAnonymousNamespace()) {
      fmt::format_to(out, "anonymous namespace\n");
    } else {
      auto sr = ND->getName();
      fmt::format_to(out, "namespace {}\n", std::string_view(sr.data(), sr.size()));
    }
  }

  bool adjust_to_namespace(clang::DeclContext* DC, clang::TypeDecl* TD) {
    llvm::outs() << "adjust_to_namespace\n";
    std::deque<clang::DeclContext*> to_namespaces;
    for(; DC; DC = DC->getParent()) {
      auto b = is_namespace_like(DC);
      if (!b) {
        TD->dump();
      }
      assert(b);
      if (DC->getDeclKind() == clang::Decl::Kind::LinkageSpec) {
        return false;
      }
      to_namespaces.push_back(DC);
      llvm::outs() << "push\n";
    }
    auto iter1 = current_namespace.begin();
    auto iter2 = std::prev(to_namespaces.end());
    while (true) {
      if (iter2 != to_namespaces.begin()) {
        auto iter1_ = std::next(iter1);
        auto iter2_ = std::prev(iter2);
        if (iter1 != current_namespace.end() && (*iter1_ == *iter2_)) {
          iter1 = iter1_;
          iter2 = iter2_;
          continue;
        }
      }
      for (auto iter = std::prev(current_namespace.end()); iter != iter1; iter = std::prev(iter)) {
        close_namespace(*iter);
      }
      for (auto iter = iter2; iter != to_namespaces.begin();) {
        iter = std::prev(iter);
        open_namespace(*iter);
      }
      break;
    }
  }
  void traverseDeclContext(clang::DeclContext* DC) {
    open_namespace(DC);
    traverseDeclContextInner(DC);
    close_namespace(DC);
  }
  void traverseDeclContextInner(clang::DeclContext* DC) {
    assert(is_namespace_like(DC));
    for (auto* decl: llvm::make_range(DC->decls_begin(), DC->decls_end())) {
      if (!ctx.getSourceManager().isInMainFile(decl->getLocation())) {
        //continue;
      }
      if (auto* TD = llvm::dyn_cast<clang::TypeDecl>(decl)) {
        if (adjust_to_namespace(TD->getDeclContext(), TD)) {
        }
      } else if (auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
        traverseDeclContext(ND);
      }
    }
  }
  void traverseTranslationUnit(clang::TranslationUnitDecl* TU) {
    auto* DC = clang::TranslationUnitDecl::castToDeclContext(TU);
    current_namespace.push_back(DC);
    traverseDeclContextInner(DC);
  }
public:
  StaticReflectionASTVisitor(StaticReflectionASTConsumer* consumer_, clang::ASTContext& ctx_, StaticReflectionCommandLineConfig config_): consumer(consumer_), ctx(ctx_), config(config_) {
  }
  void run() {
    fmt::format_to(out, "namespace static_reflection {{\n");
    traverseTranslationUnit(ctx.getTranslationUnitDecl());
    fmt::format_to(out, "}} // namespace static_reflection \n");
    llvm::outs() << llvm::StringRef(out.data(), out.size());
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
  }
  virtual void Initialize(clang::ASTContext& Context) override {
    reset();
    this->Context = &Context;
  }
  virtual void HandleTranslationUnit(clang::ASTContext& Ctx) override {
    StaticReflectionASTVisitor(this, Ctx, config).run();
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
