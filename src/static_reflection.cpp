#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclCXX.h"
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
  bool main_only = false;
};

/*
class DeclContextGuardBase {
  DeclContextGuardBase* parent;
public:
  virtual void open_decl_context() = 0;
  virtual void close_decl_context() = 0;
  virtual bool decl_context_opened() = 0;
  virtual void may_open_decl_context() {
    if (!decl_context_opened()) {
      open_decl_context();
    }
  }
  virtual void may_close_decl_context() {
    if (decl_context_opened()) {
      close_decl_context();
    }
  }
  virtual ~DeclContextGuardBase() {}
};

struct DeclContextGuard: DeclContextGuardBase {
private:
  bool opened;
  clang::DeclContext* decl_context;
public:
  DeclContextGuard(clang::DeclContext* decl_context_): opened(false), decl_context(decl_context_) {
  }
  void open_decl_context() override {
  }
  void close_decl_context() override {
  }
  bool decl_context_opened() override {
    return opened;
  }
};

struct NopDeclContextGuard: DeclContextGuardBase {
private:
  bool opened = false;
public:
  void open_decl_context() override {
    opened = true;
  }
  void close_decl_context() override {
    opened = false;
  }
  bool decl_context_opened() override {
    return opened;
  }
};
*/

class StaticReflectionASTVisitor {
  StaticReflectionASTConsumer* consumer;
  clang::ASTContext& ctx;
  StaticReflectionCommandLineConfig config;
  fmt::memory_buffer out;
  std::vector<clang::DeclContext*> current_decl_context;
  int current_indent;
  bool visitDecl(clang::Decl* D) {
    if (!ctx.getSourceManager().isInMainFile(D->getLocation())) {
      return true;
    }
    //fmt::format_to(out, "VisitDecl({} {})\n", D->getKind(), D->getDeclKindName());
    if (auto* TD = llvm::dyn_cast<clang::TypeDecl>(D)) {
      fmt::format_to(out, "TypeDecl({})\n", TD->getNameAsString());
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
    //llvm::outs() << "current_indent: " << current_indent << "\n";
    // TODO: more efficient
    for (int i = 0; i < current_indent; ++i) {
      fmt::format_to(out, " ");
    }
  }
  /*
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
  */
  static std::string_view get_linkage_name(clang::LinkageSpecDecl::LanguageIDs l) {
    if (l == clang::LinkageSpecDecl::lang_c) {
      return "\"C\"";
    } else {
      return "\"C++\"";
    }
  }
  void open_decl_context(clang::DeclContext* dc) {
    current_decl_context.push_back(dc);
    if (auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(dc)) {
      indent();
      if (ND->isInline()) {
        fmt::format_to(out, "inline ");
      }
      fmt::format_to(out, "namespace ");
      if (!ND->isAnonymousNamespace()) {
        auto sr = ND->getName();
        fmt::format_to(out, "{}", std::string_view(sr.data(), sr.size()));
      }
      fmt::format_to(out, " {{\n");
      inc_indent();
      return;
    } else if (auto* LS = llvm::dyn_cast<clang::LinkageSpecDecl>(dc)) {
      indent();
      fmt::format_to(out, "extern {} {{\n", get_linkage_name(LS->getLanguage()));
      inc_indent();
      return;
    }
    assert(false && "unknown decl context");
  }
  void close_decl_context() {
    assert(!current_decl_context.empty());
    auto dc = current_decl_context.back();
    current_decl_context.pop_back();
    dec_indent();
    indent();
    fmt::format_to(out, "}} //");
    if (auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(dc)) {
      if (ND->isInline()) {
        fmt::format_to(out, "inline ");
      }
      fmt::format_to(out, "namespace ");
      if (!ND->isAnonymousNamespace()) {
        auto sr = ND->getName();
        fmt::format_to(out, "{}", std::string_view(sr.data(), sr.size()));
      }
      fmt::format_to(out, "\n");
      return;
    } else if (auto* LS = llvm::dyn_cast<clang::LinkageSpecDecl>(dc)) {
      fmt::format_to(out, "extern {} \n", get_linkage_name(LS->getLanguage()));
      return;
    }
    assert(false && "unknown decl context");
  }
  template <typename IterTy>
  void adjust_to_decl_context(IterTy iter1, IterTy iter2) {
    /*
    llvm::errs() << "into adjust\n";
    for (auto iter = iter2;;iter = std::prev(iter)) {
      llvm::errs() << *iter << ' ';
      if (iter == iter1) {
        break;
      }
    }
    llvm::errs() << '\n';
    for (auto* dc: current_decl_context) {
      llvm::errs() << dc << ' ';
    }
    llvm::errs() << '\n';
    */
    auto iter3 = current_decl_context.begin();
    auto iter = iter2;
    auto open_all = [&](auto iter1, auto iter) {
    /*
    llvm::errs() << "in open_all\n";
    for (auto it = iter;;it= std::prev(it)) {
      llvm::errs() << *it<< ' ';
      if (it== iter1) {
        break;
      }
    }
    */
      while (true) {
        open_decl_context(*iter);
        if (iter == iter1) {
          break;
        }
        iter = std::prev(iter);
      }
    };
    auto close_all = [&](std::remove_reference_t<decltype(iter3)> iter) {
      for (auto end = current_decl_context.end(); iter != end; iter = std::next(iter)) {
        close_decl_context();
      }
    };
    while (true) {
      if (*iter != *iter3) {
        close_all(iter3);
        open_all(iter1, iter);
        return;
      }
      iter3 = std::next(iter3);
      if (iter3 == current_decl_context.end()) {
        if (iter != iter1) {
          open_all(iter1, std::prev(iter));
        }
        return;
      }
      if (iter == iter1) {
        close_all(iter3);
        return;
      }
      iter = std::prev(iter);
    }
  }
  std::vector<clang::DeclContext*> adjust_to_decl_context(clang::CXXRecordDecl* RD) {
    std::vector<clang::DeclContext*> target_context;
    for (auto* c = RD->getDeclContext(); c; c = c->getParent()) {
      target_context.push_back(c);
    }
    for (auto iter = target_context.begin(); iter != target_context.end(); iter = std::next(iter)) {
      clang::DeclContext* dc = *iter;
      if (!dc->isRecord()) {
        adjust_to_decl_context(iter, std::prev(target_context.end()));
        target_context.resize(std::distance(target_context.begin(), iter));
        return target_context;
      }
    }
    assert(false && "root decl context guaranteed to be non-class");
  }
  void traverseRecordDecl(clang::CXXRecordDecl* RD) {
    auto nested_name = adjust_to_decl_context(RD);
  }
  void traverseTypeDecl(clang::TypeDecl* TD) {
    if (auto* RD = llvm::dyn_cast<clang::CXXRecordDecl>(TD)) {
      traverseRecordDecl(RD);
    }
  }
  void traverseNamespaceDecl(clang::NamespaceDecl* ND) {
    traverseDeclContext(ND);
  }
  void traverseLinkageSpecDecl(clang::LinkageSpecDecl* LS) {
    traverseDeclContext(LS);
  }
  void traverseDeclContext(clang::DeclContext* DC) {
    for (auto* decl: llvm::make_range(DC->decls_begin(), DC->decls_end())) {
      if (config.main_only && !ctx.getSourceManager().isInMainFile(decl->getLocation())) {
        continue;
      }
      if (auto* TD = llvm::dyn_cast<clang::TypeDecl>(decl)) {
        /*
        if (adjust_to_namespace(TD->getDeclContext(), TD)) {
        }
        */
        traverseTypeDecl(TD);
      } else if (auto* ND = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
        traverseNamespaceDecl(ND);
      } else if (auto* LS = llvm::dyn_cast<clang::LinkageSpecDecl>(decl)) {
        traverseLinkageSpecDecl(LS);
      }
    }
  }
  void traverseTranslationUnit(clang::TranslationUnitDecl* TU) {
    auto* DC = clang::TranslationUnitDecl::castToDeclContext(TU);
    current_decl_context.push_back(DC);
    traverseDeclContext(DC);
  }
public:
  StaticReflectionASTVisitor(StaticReflectionASTConsumer* consumer_, clang::ASTContext& ctx_, StaticReflectionCommandLineConfig config_): consumer(consumer_), ctx(ctx_), config(config_), current_indent(0) {
  }
  void run() {
    fmt::format_to(out, "namespace static_reflection {{\n");
    current_indent = config.indent;
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
