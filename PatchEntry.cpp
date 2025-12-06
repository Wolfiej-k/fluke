#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class BoundsCheckPass : public PassInfoMixin<BoundsCheckPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();

    // Resets entry function to public after clam
    Function *EntryFn = M.getFunction("entry");
    if (EntryFn && !EntryFn->isDeclaration()) {
      EntryFn->setLinkage(GlobalValue::ExternalLinkage);
      EntryFn->setVisibility(GlobalValue::DefaultVisibility);
    }

    return PreservedAnalyses::none();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "patch-entry", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "patch-entry") {
                    MPM.addPass(BoundsCheckPass());
                    return true;
                  }
                  return false;
                });
          }};
}
