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
    const DataLayout &DL = M.getDataLayout();
    PointerType *I8PtrTy = PointerType::getUnqual(Type::getInt8Ty(Ctx));
    Type *Int64Ty = Type::getInt64Ty(Ctx);

    // Runtime function for bounds checking
    FunctionCallee BoundsCheckFn = M.getOrInsertFunction(
        "__bounds_check",
        FunctionType::get(Type::getVoidTy(Ctx), {I8PtrTy, Int64Ty}, false));

    // Runtime function for bounds assumptions
    FunctionCallee BoundsAssumeFn = M.getOrInsertFunction(
        "__bounds_assume",
        FunctionType::get(Type::getVoidTy(Ctx), {I8PtrTy, Int64Ty}, false));

    // Annotate assumptions in entry function
    Function *EntryFn = M.getFunction("entry");
    if (EntryFn && !EntryFn->isDeclaration()) {
      BasicBlock &EntryBB = EntryFn->getEntryBlock();
      IRBuilder<> B(&*EntryBB.getFirstInsertionPt());
      instrumentGlobalsAndFunctions(M, B, BoundsAssumeFn);
    }

    for (Function &F : M) {
      if (F.isDeclaration()) {
        continue;
      }

      for (BasicBlock &BB : F) {
        for (auto Inst = BB.begin(); Inst != BB.end(); ++Inst) {
          Instruction &I = *Inst;
          IRBuilder<> B(&I);

          // Annotate memory accesses
          if (auto *LI = dyn_cast<LoadInst>(&I)) {
            instrumentPointer(B, LI->getPointerOperand(),
                              getTypeStoreSize(M, LI->getType()),
                              BoundsCheckFn);
          } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
            instrumentPointer(
                B, SI->getPointerOperand(),
                getTypeStoreSize(M, SI->getValueOperand()->getType()),
                BoundsCheckFn);
          } else if (auto *AMW = dyn_cast<AtomicRMWInst>(&I)) {
            instrumentPointer(
                B, AMW->getPointerOperand(),
                getTypeStoreSize(M, AMW->getValOperand()->getType()),
                BoundsCheckFn);
          } else if (auto *CX = dyn_cast<AtomicCmpXchgInst>(&I)) {
            instrumentPointer(
                B, CX->getPointerOperand(),
                getTypeStoreSize(M, CX->getNewValOperand()->getType()),
                BoundsCheckFn);
          }

          // Assume stack allocations are safe
          else if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            B.SetInsertPoint(I.getNextNode());
            uint64_t TypeSize = DL.getTypeAllocSize(AI->getAllocatedType());
            Value *SizeVal = ConstantInt::get(Int64Ty, TypeSize);
            if (AI->isArrayAllocation()) {
              Value *ArrSize = B.CreateZExtOrTrunc(AI->getArraySize(), Int64Ty);
              SizeVal = B.CreateMul(SizeVal, ArrSize);
            }
            injectCall(B, BoundsAssumeFn, AI, SizeVal);
          }

          // Assume heap allocations are safe
          else if (auto *Call = dyn_cast<CallBase>(&I)) {
            Function *Callee = Call->getCalledFunction();
            if (!Callee || isa<InvokeInst>(Call)) {
              continue;
            }

            StringRef Name = Callee->getName();
            bool IsMalloc = Name.equals("malloc");
            bool IsCalloc = Name.equals("calloc");
            bool IsRealloc = Name.equals("realloc");

            if ((IsMalloc || IsCalloc || IsRealloc) &&
                Call->getType()->isPointerTy()) {
              B.SetInsertPoint(I.getNextNode());

              Value *SizeVal = ConstantInt::get(Int64Ty, 0);

              if (IsCalloc && Call->arg_size() >= 2) {
                Value *Count =
                    B.CreateZExtOrTrunc(Call->getArgOperand(0), Int64Ty);
                Value *ElemSize =
                    B.CreateZExtOrTrunc(Call->getArgOperand(1), Int64Ty);
                SizeVal = B.CreateMul(Count, ElemSize);
              } else if (IsRealloc && Call->arg_size() >= 2) {
                SizeVal = B.CreateZExtOrTrunc(Call->getArgOperand(1), Int64Ty);
              } else if (IsMalloc && Call->arg_size() >= 1) {
                SizeVal = B.CreateZExtOrTrunc(Call->getArgOperand(0), Int64Ty);
              }

              injectCall(B, BoundsAssumeFn, Call, SizeVal);
            }
          }
        }
      }
    }

    return PreservedAnalyses::none();
  }

private:
  static uint64_t getTypeStoreSize(Module &M, Type *T) {
    return M.getDataLayout().getTypeStoreSize(T);
  }

  static void injectCall(IRBuilder<> &B, FunctionCallee &Fn, Value *Ptr,
                         Value *Size) {
    LLVMContext &Ctx = B.getContext();
    PointerType *I8PtrTy = PointerType::getUnqual(Type::getInt8Ty(Ctx));
    Type *Int64Ty = Type::getInt64Ty(Ctx);
    B.CreateCall(Fn, {B.CreateBitCast(Ptr, I8PtrTy),
                      B.CreateZExtOrTrunc(Size, Int64Ty)});
  }

  static void instrumentPointer(IRBuilder<> &B, Value *Ptr, uint64_t Size,
                                FunctionCallee &Fn) {
    LLVMContext &Ctx = B.getContext();
    injectCall(B, Fn, Ptr, ConstantInt::get(Type::getInt64Ty(Ctx), Size));
  }

  static void instrumentGlobalsAndFunctions(Module &M, IRBuilder<> &B,
                                            FunctionCallee &AssumeFn) {
    LLVMContext &Ctx = M.getContext();
    const DataLayout &DL = M.getDataLayout();

    for (GlobalVariable &G : M.globals()) {
      if (G.getName().startswith("llvm."))
        continue;
      uint64_t Size = DL.getTypeAllocSize(G.getValueType());
      injectCall(B, AssumeFn, &G,
                 ConstantInt::get(Type::getInt64Ty(Ctx), Size));
    }

    for (Function &F : M) {
      if (F.isDeclaration() || F.getName().startswith("llvm."))
        continue;
      injectCall(B, AssumeFn, &F, ConstantInt::get(Type::getInt64Ty(Ctx), 1));
    }
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "bounds-check", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "bounds-check") {
                    MPM.addPass(BoundsCheckPass());
                    return true;
                  }
                  return false;
                });
          }};
}
