#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib> // Required for getenv
#include <set>
#include <sstream>
#include <vector>

using namespace llvm;

namespace {

class BoundsCheckPass : public PassInfoMixin<BoundsCheckPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();

    // Promote verified assertions to llvm.assume
    std::set<int> SafeSet;
    const char *EnvStr = std::getenv("VERIFIED_IDS");
    bool HasVerifiedIDs = (EnvStr != nullptr && EnvStr[0] != '\0');
    if (HasVerifiedIDs) {
      std::stringstream SS(EnvStr);
      std::string Segment;
      while (std::getline(SS, Segment, ',')) {
        if (!Segment.empty()) {
          SafeSet.insert(std::stoi(Segment));
        }
      }
    }

    Function *CrabAssert = M.getFunction("__CRAB_assert");
    Function *LlvmAssume = Intrinsic::getDeclaration(&M, Intrinsic::assume);

    int CheckCounter = 0;
    std::vector<CallInst *> ToPromote;

    for (Function &F : M) {
      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (CI->getCalledFunction() == CrabAssert) {
              CheckCounter++;
              if (HasVerifiedIDs && SafeSet.count(CheckCounter)) {
                ToPromote.push_back(CI);
              }
            }
          }
        }
      }
    }

    for (CallInst *CI : ToPromote) {
      IRBuilder<> B(CI);
      Value *Cond = CI->getArgOperand(0);
      if (Cond->getType()->isIntegerTy(32)) {
        Cond = B.CreateICmpNE(Cond, B.getInt32(0));
      } else if (Cond->getType()->isIntegerTy(8)) {
        Cond = B.CreateICmpNE(Cond, B.getInt8(0));
      }

      B.CreateCall(LlvmAssume, {Cond});
      CI->eraseFromParent();
    }

    // Reset entry function visibility
    Function *EntryFn = M.getFunction("entry");
    if (EntryFn && !EntryFn->isDeclaration()) {
      EntryFn->setLinkage(GlobalValue::ExternalLinkage);
      EntryFn->setVisibility(GlobalValue::DefaultVisibility);
      EntryFn->removeFnAttr(Attribute::OptimizeNone);
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
