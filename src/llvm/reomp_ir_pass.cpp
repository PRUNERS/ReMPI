#include <stdio.h>
#include <stdlib.h>
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include "llvm/IR/LLVMContext.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#include <list>
#include <vector>

#include "reomp_ir_pass.h"
#include "reomp.h"
#include "reomp_mem.h"
#include "reomp_drace.h"
#include "mutil.h"



#define REOMP_RR_RACY_LOAD  (1)
#define REOMP_RR_RACY_STORE (1)
#define REOMP_RR_RACY_CALLINST (1)
#define REOMP_RR_RACY_INVOKEINST (1)
#define REOMP_RR_CRITICAL (1)
#define REOMP_RR_OMP_LOCK (1)
#define REOMP_RR_OTHER_LOCK (1)
#define REOMP_RR_REDUCTION (1)
#define REOMP_RR_SINGLE (1)
#define REOMP_RR_MASTER (1)
#define REOMP_RR_ATOMICOP (1)
#define REOMP_RR_ATOMICLOAD (1)
#define REOMP_RR_ATOMICSTORE (1)
/* Flag for debugging flag */
#define REOMP_RR_FORK_CALL (0)


void ReOMP::insert_func(Instruction *I, BasicBlock *BB, int offset, int control, Value* ptr, Value* size)
{
  vector<Value*> arg_vec;
  IRBuilder<> builder(I);
  builder.SetInsertPoint(BB, (offset)? ++builder.GetInsertPoint():builder.GetInsertPoint());
  Constant* func = reomp_func_umap.at(REOMP_CONTROL_STR);
  arg_vec.push_back(REOMP_CONST_INT32TY(control));
  if (!ptr) ptr = REOMP_CONST_INT64PTRTY_NULL;// ConstantPointerNull::get(Type::getInt64PtrTy(*REOMP_CTX));
  arg_vec.push_back(ptr);
  if (!size) size = REOMP_CONST_INT32TY(0); //ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), 0);
  arg_vec.push_back(size);
  builder.CreateCall(func, arg_vec);
  return;
}



/* Outer Handlers */
int ReOMP::handle_function(Function &F)
{
  int modified_counter = 0;
  modified_counter += handle_function_on_main(F);
  return modified_counter;
}

int ReOMP::handle_basicblock(Function &F, BasicBlock &BB)
{
  return 0;
}




int ReOMP::handle_function_on_main(Function &F)
{
  int modified_counter = 0;
  int is_hook_enabled = 0;
  size_t num_locks;
  if (F.getName() == "main") {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
	if (!is_hook_enabled) {
	  num_locks = reomp_drace_get_num_locks();
	  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_MAIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_MAIN), REOMP_CONST_INT64TY(num_locks));
	  modified_counter++;
	  is_hook_enabled = 1;
	}
		
	if (dyn_cast<ReturnInst>(&I)) {
	  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_AFT_MAIN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_MAIN), REOMP_CONST_INT64TY(num_locks));
	  modified_counter++;
	}
	
      }
    }
  } 
  return modified_counter;
}


int ReOMP::handle_instruction(Function &F, BasicBlock &BB, Instruction &I)
{
  int modified_counter = 0;
  modified_counter += handle_instruction_on_critical(F, BB, I); /* insert lock/unlock for omp_ciritcal/reduction*/
  modified_counter += handle_instruction_on_load_store(F, BB, I); /* */
  return modified_counter;
}


int ReOMP::handle_instruction_on_reduction(Function &F, BasicBlock &BB, Instruction &I)
{
  int modified_counter = 0;
  int is_after_reduction_begin = 0;
  string name;
  CallInst *CI = NULL;
  AtomicRMWInst *ARMWI;
  AtomicCmpXchgInst *ACXI;
  
  //  BasicBlock tmp_BB = BB;
  //  Instruction tmp_I = I;

  /* Instrument to before __kmpc_reduce and __kmpc_reduce_nowait */
  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_REDUCE_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), NULL);
  modified_counter++;
  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER, REOMP_AFT_REDUCE_BEGIN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), &I);
  modified_counter++;

  /* Instrument to __kmpc_end_reduce and __kmpc_end_reduce_nowait 
                 or atomic operations
  */
  Function::iterator BB_it     = F.begin();
  Function::iterator BB_it_end = F.end();
  for (; BB_it != BB_it_end; BB_it++) {
    BasicBlock::iterator IN_it     = BB_it->begin();
    BasicBlock::iterator IN_it_end = BB_it->end();
    for (; IN_it != IN_it_end; IN_it++) {
      if ((CI = dyn_cast<CallInst>(&I)) != NULL) {
	name = CI->getCalledValue()->getName();
	if (name == "__kmpc_reduce" || name == "__kmpc_reduce_nowait") is_after_reduction_begin = 1;
      }
      if (!is_after_reduction_begin) continue;

      Instruction *IN = &(*IN_it);
      //      errs() << " >>>>> " << IN->getCalledValue()->getName() << "\n"; 
      //      IN->print(errs());
      //      errs() << "\n";
      if ((CI = dyn_cast<CallInst>(IN)) != NULL) {
	name = CI->getCalledValue()->getName();	
	if (name == "__kmpc_end_reduce" || name == "__kmpc_end_reduce_nowait") {
	  /* Instrument to __kmpc_end_reduce or __kmpc_end_reduce_nowait */
	  insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_REDUCE_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), &I);
	  modified_counter++;
	  insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_REDUCE_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), &I);
	  modified_counter++;
	}
      } else if ((ARMWI = dyn_cast<AtomicRMWInst>(IN)) != NULL) {
	insert_func(ARMWI, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_AFT_REDUCE_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), &I);
	modified_counter++;
      } else if ((ACXI  = dyn_cast<AtomicCmpXchgInst>(IN)) != NULL) {
	insert_func(ACXI, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_AFT_REDUCE_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_REDUCTION), &I);
	modified_counter++;
      }
      if (modified_counter == 5) break;
    }
    if (modified_counter == 5) break;
  }
  if (modified_counter != 5) {
    MUTIL_ERR("Modified counter is not 5 on reduction");
  }
  return modified_counter;
}

int ReOMP::handle_instruction_on_critical(Function &F, BasicBlock &BB, Instruction &I)
{
  int modified_counter = 0;
  string name;
  if (CallInst *CI = dyn_cast<CallInst>(&I)) {
    name = CI->getCalledValue()->getName();
    if (name == "__kmpc_critical" && REOMP_RR_CRITICAL) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_CRITICAL_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_CRITICAL), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if (name == "__kmpc_end_critical" && REOMP_RR_CRITICAL) {      
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_CRITICAL), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if (name == "omp_set_lock" && REOMP_RR_OMP_LOCK) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_CRITICAL_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_OMP_LOCK), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if (name == "omp_unset_lock" && REOMP_RR_OMP_LOCK) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_OMP_LOCK), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if (name == "semop" && REOMP_RR_OTHER_LOCK) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_CRITICAL_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_OTHER_LOCK), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_OTHER_LOCK), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 2;
    } else if ((name == "__kmpc_reduce" || name == "__kmpc_reduce_nowait") && REOMP_RR_REDUCTION) {
      modified_counter = this->handle_instruction_on_reduction(F, BB, I);
    } else if (name == "__kmpc_single" && REOMP_RR_SINGLE) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_CRITICAL_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_SINGLE), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      //      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_SINGLE), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if (name == "__kmpc_master" && REOMP_RR_MASTER) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_CRITICAL_BEGIN, REOMP_CONST_INT64TY(REOMP_RR_TYPE_MASTER), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      //insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_MASTER), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 1;
    } else if ((name == "__kmpc_end_single" && REOMP_RR_SINGLE) ||
	       (name == "__kmpc_end_master" && REOMP_RR_MASTER)) {
      /*__kmpc_end_single/master is executed by an only thread executing __kmpc_single/master */
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_AFT_CRITICAL_END, REOMP_CONST_INT64TY(REOMP_RR_TYPE_SINGLE), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
    } else if (name == "exit") {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_AFT_MAIN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_MAIN), REOMP_CONST_INT64TY(REOMP_RR_LOCK_NULL));
      modified_counter = 1;
    } else if (name == "__kmpc_fork_call" && REOMP_RR_FORK_CALL) {
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_OTHER,  REOMP_CONST_INT64TY(0), REOMP_CONST_INT64TY(REOMP_RR_LOCK_NULL));
      insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER,  REOMP_OTHER,  REOMP_CONST_INT64TY(1), REOMP_CONST_INT64TY(REOMP_RR_LOCK_NULL));
      modified_counter = 1;
    }
  } else if (AtomicRMWInst *ARMWI = dyn_cast<AtomicRMWInst>(&I)) {
    if (REOMP_RR_ATOMICOP) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICOP), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICOP), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 2;
    }
  } else if (AtomicCmpXchgInst *ACXI  = dyn_cast<AtomicCmpXchgInst>(&I)) {
    if (REOMP_RR_ATOMICOP) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICOP), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICOP), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 2;
    }
  } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
    if (SI->isAtomic() && REOMP_RR_ATOMICSTORE) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICSTORE), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICSTORE), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 2;
    }
  } else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
    if (LI->isAtomic() && REOMP_RR_ATOMICLOAD) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICLOAD), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_ATOMICLOAD), REOMP_CONST_INT64TY(REOMP_RR_LOCK_GLOBAL));
      modified_counter = 2;
    }
  } else if (I.isAtomic()) {
    MUTIL_ERR("Missing a atomic");
  } 

  //  if (modified_counter > 0) MUTIL_DBG("INS: %s", name.c_str());    
  return modified_counter;
}

int ReOMP::is_data_racy_access(Function *F, Instruction *I, uint64_t *access)
{
  int lock_id;
  unsigned line, column;
  const char *filename, *dirname;
  if (const DebugLoc &dbloc = I->getDebugLoc()) {
    line = dbloc.getLine();
    column = dbloc.getCol();
    if (DIScope *discope = dyn_cast<DIScope>(dbloc.getScope())) {
      filename = discope->getFilename().data();
      dirname  = discope->getDirectory().data();
    } else {
      MUTIL_ERR("Third operand of DebugLoc is not DIScope");
    }
    //      errs() << " >>>>> " << line << ":" << column << "   " << dirname << "   " << filename << "\n";                                               
    if ((lock_id = reomp_drace_is_data_race(F->getName().data(), dirname, filename, line, column, access)) > 0) {
      return lock_id;
    }
  }
  return 0;
}

int ReOMP::handle_instruction_on_load_store(Function &F, BasicBlock &BB, Instruction &I)
{
  size_t lock_id = 0;
  int modified_counter = 0;
  uint64_t access;
  if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
    if (!REOMP_RR_RACY_STORE) return modified_counter;
    if ((lock_id = this->is_data_racy_access(&F, &I, &access)) != 0) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_STORE), REOMP_CONST_INT64TY(lock_id));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_STORE), REOMP_CONST_INT64TY(lock_id));
      modified_counter += 2;
    }
  } else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
    if (!REOMP_RR_RACY_LOAD) return modified_counter;
    if ((lock_id = this->is_data_racy_access(&F, &I, &access)) != 0) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_LOAD), REOMP_CONST_INT64TY(lock_id));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_LOAD), REOMP_CONST_INT64TY(lock_id));

      modified_counter += 2;
    }    
  } else if (CallInst *CI = dyn_cast<CallInst>(&I)) {
    if (!REOMP_RR_RACY_CALLINST) return modified_counter;
    string name = CI->getCalledValue()->getName();
    if (name == "reomp_control") return modified_counter;
    if ((lock_id = this->is_data_racy_access(&F, &I, &access)) != 0) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_CPP_STL), REOMP_CONST_INT64TY(lock_id));
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_CPP_STL), REOMP_CONST_INT64TY(lock_id));
      modified_counter += 2;
    }
  } else if (InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    if (!REOMP_RR_RACY_INVOKEINST) return modified_counter;
    BasicBlock *nextBB;
    Instruction *frontIN;
    string name = II->getCalledValue()->getName();
    if (name == "reomp_control") return modified_counter;
    if ((lock_id = this->is_data_racy_access(&F, &I, &access)) != 0) {
      insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_IN,  REOMP_CONST_INT64TY(REOMP_RR_TYPE_CPP_STL), REOMP_CONST_INT64TY(lock_id));

      nextBB  = II->getNormalDest();
      frontIN = &(nextBB->front());
      insert_func(frontIN, nextBB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_CPP_STL), REOMP_CONST_INT64TY(lock_id));

      nextBB  = II->getUnwindDest();
      frontIN = &(nextBB->front());
      //NOTE: clang hangs when instrumenting reomp_control in unwind basicblock.
      insert_func(frontIN, nextBB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_GATE_OUT, REOMP_CONST_INT64TY(REOMP_RR_TYPE_CPP_STL), NULL);

      modified_counter += 3;
    }
  }
  //  if (modified_counter > 0)  MUTIL_DBG("INS: race");
  return modified_counter;  
}




void ReOMP::init_inserted_functions(Module &M)
{

  LLVMContext &ctx = M.getContext();
  reomp_func_umap["reomp_rr"] = M.getOrInsertFunction("reomp_rr", 
							 Type::getInt64PtrTy(ctx), 
							 Type::getInt64Ty(ctx), 
							 NULL);
  
  reomp_func_umap[REOMP_CONTROL_STR] =  M.getOrInsertFunction(REOMP_CONTROL_STR,
							      Type::getVoidTy(ctx),
							      Type::getInt32Ty(ctx),
							      Type::getInt64PtrTy(ctx),
							      Type::getInt64Ty(ctx),
							      NULL);
  
  /* For debugging ReOMP */
  reomp_func_umap[REOMP_MEM_PRINT_ADDR] = M.getOrInsertFunction(REOMP_MEM_PRINT_ADDR,
								  Type::getInt64PtrTy(ctx),
								  NULL);

  return;
}

bool ReOMP::doInitialization(Module &M)
{
  REOMP_M   = &M;
  REOMP_CTX = &(M.getContext());
  REOMP_DL  = new DataLayout(REOMP_M);
  init_inserted_functions(M);
  reomp_drace_parse(REOMP_DRACE_LOG_ARCHER);
  return true;
}

bool ReOMP::runOnFunction(Function &F)
{
  int modified_counter = 0;
  modified_counter += handle_function(F);
  for (BasicBlock &BB : F) {
    modified_counter += handle_basicblock(F, BB);
    for (Instruction &I : BB) {
      modified_counter += handle_instruction(F, BB, I);
    }
  }
  return modified_counter > 0;
}


void ReOMP::getAnalysisUsage(AnalysisUsage &AU) const
{
  //  AU.setPreservesCFG();
  // addReauired cause errors
  //  AU.addRequired<DependenceAnalysisWrapperPass>();
}

char ReOMP::ID = 0;
//static RegisterPass<DASample> A("dasample", "dasample", false, false);

static void registerReOMP(const PassManagerBuilder &, legacy::PassManagerBase &PM)
{
  PM.add(new ReOMP);
}

static RegisterStandardPasses RegisterReOMP(PassManagerBuilder::EP_EarlyAsPossible, registerReOMP);


/* ============== End of class ================================= */




















// Unused function below


























/* ===================================================================== */




#if 0

// int ReOMP::insert_init(BasicBlock &BB, Instruction &I)
// {
//   size_t num_locks;
//   num_locks = reomp_drace_get_num_locks();
//   insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_MAIN, NULL,   ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), num_locks));
//   return 1;
// }

// int ReOMP::insert_finalize(BasicBlock &BB, Instruction &I)
// {
//   insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_AFT_MAIN, NULL, NULL);
//   return 1;
// }

// int ReOMP::ci_insert_on_fork(Function &F, BasicBlock &BB, Instruction &I)
// {
//   int modified_counter = 0;
//   CallInst *CI;
//   string name;
//   if (!(CI = dyn_cast<CallInst>(&I))) return 0;
//   name = CI->getCalledValue()->getName();
//   if (name == "__kmpc_fork_call") {
//     insert_func(CI, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEF_FORK, NULL, NULL);
//     insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER , REOMP_AFT_FORK, NULL, NULL);
//     modified_counter = 2;
//   }
//   return 1;
// }



// int ReOMP::ci_rr_insert_rr_on_omp_func(Function &F, BasicBlock &BB, Instruction &I)
// {
//   CallInst *CI;
//   string name;
//   if (!(CI = dyn_cast<CallInst>(&I))) return 0;
//   name = CI->getCalledValue()->getName();
//   if (F.getName().startswith(".omp_outlined.")) return 0;
//   if (!(name == "__kmpc_fork_call")) return 0;

//   //  omp_rr_data = create_omp_rr_data();
//   //  get_responsible_data(CI, omp_rr_data);
//   //  insert_rr(&BB, CI, omp_rr_data);
//   vector<Value*> arg_vec;
//   arg_vec.push_back(ConstantPointerNull::get(Type::getInt64PtrTy(*REOMP_CTX)));
//   arg_vec.push_back(ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), 0));
//   insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER, "reomp_rr", arg_vec);

//   return 1;
// }


// int ReOMP::ci_mem_register_local_var_addr_on_alloca(Function &F, BasicBlock &BB, Instruction &I)
// {
//   AllocaInst *AI;
//   if (!(AI = dyn_cast<AllocaInst>(&I))) { 
//     return 0;  
//   }
//   if (F.getName().startswith(".omp_outlined.")) return 0;

//   //  Module *M = F.getParent();
//   //  DataLayout *DL = new DataLayout(REO);

//   /* TODO: reuse vector */
//   vector<Value*> arg_vec;
//   size_t type_size;

//   arg_vec.push_back(AI);
//   type_size = REOMP_DL->getTypeSizeInBits(AI->getAllocatedType());
//   arg_vec.push_back(ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), type_size));
//   insert_func(AI, &BB, REOMP_IR_PASS_INSERT_AFTER, REOMP_MEM_REGISTER_LOCAL_VAR_ADDR, arg_vec);

//   return 1;
// }


// int ReOMP::ci_on_omp_outline(Function &F)
// {
//   int is_instrumented_begin = 0;
//   if (F.getName().startswith(".omp_outlined.")) {
//     for (BasicBlock &BB : F) {
//       for (Instruction &I : BB) {
// 	if (!is_instrumented_begin) {
// 	  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEG_OMP, NULL, NULL);
// 	  is_instrumented_begin = 1;
// 	}
// 	if (dyn_cast<ReturnInst>(&I)) {
// 	  insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_END_OMP, NULL, NULL);
// 	} 
//       }
//     }
//   } 
//   return 1;
// }

// int ReOMP::insert_rr(BasicBlock *BB, CallInst *kmpc_fork_CI, reomp_omp_rr_data *omp_rr_data)
// {
//   int is_created = 0;
//   LLVMContext &ctx = BB->getContext();
//   vector<Value*> arg_vec;

//   for (Value* GV: *(omp_rr_data->global_var_uset)) {
//     arg_vec.clear();
//     if (GV->getType()->isPointerTy()) {
//       arg_vec.push_back(GV);
//       arg_vec.push_back(ConstantInt::get(Type::getInt64Ty(ctx), 0));
//       insert_func(kmpc_fork_CI, BB, REOMP_IR_PASS_INSERT_AFTER, "reomp_rr", arg_vec);
//       is_created++;
//     }
//   }
  
//   for (Value* AG: *(omp_rr_data->arg_list)) {
//     arg_vec.clear();
//     arg_vec.push_back(AG);
//     arg_vec.push_back(ConstantInt::get(Type::getInt64Ty(ctx), 0));
//     insert_func(kmpc_fork_CI, BB, REOMP_IR_PASS_INSERT_AFTER, "reomp_rr", arg_vec);
//     is_created++;
//   }
//   return is_created;
// }

// howto_type ReOMP::get_howto_handle(Function &F, Instruction &I, int *meta)
// {
//   // string name;

//   // if (F.getName() == "main") {
//   //   if (dyn_cast<ReturnInst>(&I)) {
//   //     return HOWTO_TYPE_DISABLE_HOOK;
//   //   } else if (!is_hook_enabled) {
//   //     return HOWTO_TYPE_ENABLE_HOOK;
//   //   }
//   // }  

//   // if (CallInst *CI = dyn_cast<CallInst>(&I)) {
//   //   name = CI->getCalledValue()->getName();
//   //   if (name == "__kmpc_fork_call") {
//   //     return HOWTO_TYPE_OMP_FUNC;
//   //   } else if (name == "malloc") {
//   //     return HOWTO_TYPE_DYN_ALLOC;
//   //   } else if (name == "calloc") {
//   //     return HOWTO_TYPE_DYN_ALLOC;
//   //   } else if (name == "realloc") {
//   //     return HOWTO_TYPE_DYN_ALLOC;
//   //   }
//   // }
//   return HOWTO_TYPE_OTHERS;
// }

// bool ReOMP::responsible_global_var(Value* value)
// {
//   StringRef var_name;
//   if (!value->getType()->isPointerTy()) {
//     errs() << "Non pointer global variables referenced in .omp_outlined\n";
//     exit(0);
//   }

//   var_name = value->getName();
//   if (var_name.startswith("llvm.")) return false;
//   if (var_name.startswith("__kmpc_")) return false;
//   if (var_name.startswith(".")) return false;
//   if (var_name == "") return false;
//   return true;
// }

// reomp_omp_rr_data* ReOMP::create_omp_rr_data()
// {
//   reomp_omp_rr_data *omp_rr_data = new reomp_omp_rr_data();
//   omp_rr_data->global_var_uset = new unordered_set<GlobalVariable*>();
//   omp_rr_data->arg_list = new list<Value*>();
//   return omp_rr_data;
// }
// void ReOMP::free_omp_rr_data(reomp_omp_rr_data* omp_rr_data)
// {
//   delete omp_rr_data->global_var_uset;
//   delete omp_rr_data->arg_list;
//   delete omp_rr_data;
//   return;
// }



// void ReOMP::get_responsible_global_vars(Function* omp_outlined_F, unordered_set<GlobalVariable*> *omp_global_vars_uset)
// {
//   if (omp_global_vars_uset->size() != 0) {
//     errs() << "omp_global_vars_uset is not empty\n";
//     exit(0);
//   }

//   for (BasicBlock &BB : *omp_outlined_F) {
//     for (Instruction &I : BB) {
//       for (Value *Op : I.operands()) {
// 	if (GlobalVariable *G = dyn_cast<GlobalVariable>(Op)) {
// 	  /* Get only user defined global variable */
// 	  //if (is_user_variable(G->getName())) {
// 	  if (responsible_global_var(G)) {
// 	    if (omp_global_vars_uset->find(G) == omp_global_vars_uset->end()) {
// 	      omp_global_vars_uset->insert(G);
// 	    }
// 	  }
// 	}
//       }
//     }
//   }
//   return;
// }

// void ReOMP::get_responsible_data(CallInst *kmpc_fork_CI, reomp_omp_rr_data *omp_rr_data)
// {
//   Function *omp_outlined_F;

//   /* Get all function arguments to be recorded and replayed */
//   extract_omp_function(kmpc_fork_CI, &omp_outlined_F, omp_rr_data->arg_list);
//   /* Get all global variables to be recorded and replayed */
//   get_responsible_global_vars(omp_outlined_F, omp_rr_data->global_var_uset);

//   return;
// }




// bool ReOMP::on_omp_function(Function &F)
// {
    

//   return false;
// }

// bool ReOMP::is_fork_call(CallInst *CI)
// {
//   return CI->getCalledFunction()->getName() == "__kmpc_fork_call";
// }


// /*
//   e.g.) Input: 
  
//   CallInst fork_CI = 
//       call 
//       void (%ident_t*, i32, void (i32*, i32*, ...)*, ...) 
//       fork_func = @__kmpc_fork_call(
//         %ident_t* nonnull %7, 
// 	i32 6, 
// 	void (i32*, i32*, ...)* bitcast (void (i32*, i32*, i32*, double*, i32*, i32*, [10 x double]*, [10 x double]*)* @.omp_outlined. to void (i32*, i32*, ...)*), 
//         i32* nonnull %1, 
// 	double* nonnull %6, 
// 	i32* nonnull %2, 
// 	i32* nonnull %5, 
// 	[10 x double]* nonnull %3, 
// 	[10 x double]* nonnull %4
//     ) !dbg !98

// */
// void ReOMP::extract_omp_function(CallInst *fork_CI, Function **omp_func, list<Value*> *omp_func_arg_list)
// {
//   Function *fork_func;
//   int first_arg_index = -1;
//   fork_func = dyn_cast<Function>(fork_CI->getCalledValue());
//   /*
//     fork_func = @__kmpc_fork_call(
//         %ident_t* nonnull %7, 
// 	i32 6, 
// 	void (i32*, i32*, ...)* bitcast (void (i32*, i32*, i32*, double*, i32*, i32*, [10 x double]*, [10 x double]*)* @.omp_outlined. to void (i32*, i32*, ...)*), 
//         i32* nonnull %1, 
// 	double* nonnull %6, 
// 	i32* nonnull %2, 
// 	i32* nonnull %5, 
// 	[10 x double]* nonnull %3, 
// 	[10 x double]* nonnull %4
//     )
//    */


//   *omp_func = NULL;
//   for (unsigned op = 0, Eop = fork_CI->getNumArgOperands(); op < Eop; ++op) {
//     Value *vv =fork_CI->getArgOperand(op);
//     if (ConstantExpr *CE = dyn_cast<ConstantExpr>(vv)) {
//       /*
// 	CE = void (i32*, i32*, ...)* bitcast (void (i32*, i32*, i32*, double*, i32*, i32*, [10 x double]*, [10 x double]*)* @.omp_outlined. to void (i32*, i32*, ...)*), 
//        */
//       Function *omp_func_v = dyn_cast<Function>(CE->getOperand(0));
//       /*
// 	omp_func_v = void (i32*, i32*, i32*, double*, i32*, i32*, [10 x double]*, [10 x double]*)* @.omp_outlined.
//        */
//       if (omp_func_v) {
// 	if (*omp_func == NULL) {
// 	  *omp_func = omp_func_v;
// 	  first_arg_index = op + 1;
// 	} else {
// 	  errs() << "Two .omp_outlined functions\n";
// 	  exit(0);
// 	}
//       }
//     }      
//   }

//   if (*omp_func == NULL) {
//     errs() << "Could not find .omp_outlined. function\n";
//     exit(0);    
//   }


//   for (unsigned op = first_arg_index, Eop = fork_CI->getNumArgOperands(); op < Eop; ++op) {
//     Value *arg =fork_CI->getArgOperand(op);
//     omp_func_arg_list->push_back(arg);
//   }
//   return;
// }


// int ReOMP::handle_omp_func(BasicBlock &BB, Instruction &I)
// {
//   reomp_omp_rr_data *omp_rr_data;
//   CallInst *CI;
//   bool is_modified;
//   omp_rr_data = create_omp_rr_data();
//   CI = dyn_cast<CallInst>(&I);
//   get_responsible_data(CI, omp_rr_data);
//   is_modified = insert_rr(&BB, CI, omp_rr_data);
//   free_omp_rr_data(omp_rr_data);
//   return 1;
// }

// int ReOMP::handle_dyn_alloc(BasicBlock &BB, Instruction &I)
// {
//   //  LLVMContext &ctx = BB.getContext();
//   vector<Value*> arg_vec;
//   Value *Vptr, *Vsize;
//   CallInst *CI;

//   Vptr  = dyn_cast<Value>(&I);
//   CI    = dyn_cast<CallInst>(&I);
//   Vsize = CI->getArgOperand(0);
//   errs() << "====" << *Vsize << "\n";
//   arg_vec.push_back(Vptr);
//   arg_vec.push_back(Vsize);
//   insert_func(CI, &BB, REOMP_IR_PASS_INSERT_AFTER, "reomp_mem_on_alloc", arg_vec);
//   return 1;
// }


// bool ReOMP::responsible_arg_var(Argument *A) 
// {
//   Type *ATy = A->getType();
//   uint64_t dbytes = A->getDereferenceableBytes();
//   A->print(errs());
//   errs() << "\n";
//   errs() << A->getName() << "\n";
//   if (ATy->isPointerTy()) {
//     if (dbytes > 0) { 
//       errs() << "We record this\n";
//       return true;
//     }
//   } else {
//     errs() << "Non-pointer arguments passed to .omp_outlined\n";
//     exit(0);
//   }
//   return false;
// }

// int ReOMP::ci_on_function_call(Function &F)
// {
//   int is_instrumented_begin = 0;
//   DISubprogram *DIS;
//   DIS = F.getSubprogram();
//   if (DIS == NULL) return 0;
//   if (DIS != NULL) {
//     if (!reomp_drace_is_in_racy_callstack(DIS->getFilename().data())) return 0;
//   }
//   for (BasicBlock &BB : F) {
//     for (Instruction &I : BB) {
//       IRBuilder<> builder(&I);
//       Value *func_name;
//       StringRef string;
//       size_t hash;

//       string = (DIS == NULL)?"null":DIS->getName();
//       func_name = builder.CreateGlobalStringPtr(string);
//       hash = reomp_util_hash_str(string.data(), string.size());

//       if (!is_instrumented_begin) {
// 	insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_BEG_FUNC_CALL, func_name, ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), hash));
// 	is_instrumented_begin = 1;
//       }
//       if (dyn_cast<ReturnInst>(&I)) {
// 	insert_func(&I, &BB, REOMP_IR_PASS_INSERT_BEFORE, REOMP_END_FUNC_CALL, func_name, ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), hash));
//       } 
//     }
//   } 
//   return 1;
// }

#endif

