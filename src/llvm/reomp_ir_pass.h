#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Function.h>

#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace llvm;
using namespace std;

#define REOMP_IR_PASS_INSERT_AFTER  (1)
#define REOMP_IR_PASS_INSERT_BEFORE (0)

#define REOMP_CONST_INT64TY(val)     ConstantInt::get(Type::getInt64Ty(*REOMP_CTX), val)
#define REOMP_CONST_INT32TY(val)     ConstantInt::get(Type::getInt32Ty(*REOMP_CTX), val)
#define REOMP_CONST_INT64PTRTY_NULL  ConstantPointerNull::get(Type::getInt64PtrTy(*REOMP_CTX))

typedef enum {
  HOWTO_TYPE_OMP_FUNC,
  HOWTO_TYPE_ENABLE_HOOK,
  HOWTO_TYPE_DISABLE_HOOK,
  HOWTO_TYPE_DYN_ALLOC,
  HOWTO_TYPE_OTHERS
} howto_type;

class reomp_omp_rr_data
{
  /*Using unordered_set to check if extracted global var was already inserted or not */
 public:
  unordered_set<GlobalVariable*> *global_var_uset; 
  list<Value*> *arg_list;
};

class ReOMP: public FunctionPass
{
 public:
  static char ID;

 ReOMP()
   : FunctionPass(ID) {}
  ~ReOMP(){}

  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  
 private:
  unordered_map<string, Constant*> reomp_func_umap;
  Module *REOMP_M;
  LLVMContext *REOMP_CTX;
  DataLayout *REOMP_DL;
  reomp_omp_rr_data* create_omp_rr_data();
  
  void init_inserted_functions(Module &M);
  int is_data_racy_access(Function *F, Instruction *I, uint64_t *access);

  void insert_func(Instruction *I, BasicBlock *BB, int offset, int control, Value* ptr, Value* size);
  //  void insert_func(Instruction *I, BasicBlock *BB, int offset, string func_name, vector<Value*> &arg_vec);
  //  void insert_func(Instruction *I, BasicBlock *BB, int offset, string func_name);

  /* Function handlers */
  int handle_function(Function &F);
  int handle_function_on_main(Function &F);
  /* BasicBlock handlers */
  int handle_basicblock(Function &F, BasicBlock &BB);
  /* Instruction handlers */
  int handle_instruction(Function &F, BasicBlock &BB, Instruction &I);
  int handle_instruction_on_critical(Function &F, BasicBlock &BB, Instruction &I);
  int handle_instruction_on_reduction(Function &F, BasicBlock &BB, Instruction &I);
  int handle_instruction_on_load_store(Function &F, BasicBlock &BB, Instruction &I);


  //    int insert_init(BasicBlock &BB, Instruction &I);
  //  int insert_finalize(BasicBlock &BB, Instruction &I);
  //  void free_omp_rr_data(reomp_omp_rr_data* omp_rr_data);
  //  void get_responsible_data(CallInst *CI, reomp_omp_rr_data *omp_rr_data);
  //  bool is_fork_call(CallInst *CI);
  //  void extract_omp_function(CallInst *CI, Function **omp_func, list<Value*> *omp_func_args_list);
  //  void get_responsible_global_vars(Function* omp_outlined_F, unordered_set<GlobalVariable*> *omp_global_vars_uset);
  //  bool responsible_global_var(Value* value);
  //  bool responsible_arg_var(Argument* value);
  //  howto_type get_howto_handle(Function &F, Instruction &I, int* meta);
  //  int insert_rr(BasicBlock *BB, CallInst *kmpc_fork_CI, reomp_omp_rr_data *omp_rr_data);
  //  bool on_omp_function(Function &F);
  //int ci_on_omp_outline(Function &F);
  //  int ci_on_function_call(Function &F);  
  //  int ci_mem_register_local_var_addr_on_alloca(Function &F, BasicBlock &BB, Instruction &I);
  //  int ci_rr_insert_rr_on_omp_func(Function &F, BasicBlock &BB, Instruction &I);
  //  int ci_insert_on_fork(Function &F, BasicBlock &BB, Instruction &I);


};



class DCEliminatePass: public FunctionPass {

 public:
  static char ID;
 DCEliminatePass(): FunctionPass(ID){}
  ~DCEliminatePass() {}

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

};


class LPCountPass: public LoopPass
{
 public:
  static char ID;
 LPCountPass(): LoopPass(ID){}
  ~LPCountPass(){}

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};


class SCEVSample: public LoopPass
{
 public:
  static char ID;
 SCEVSample(): LoopPass(ID){}
  ~SCEVSample(){}

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

class DASample: public FunctionPass
{
 public:
  static char ID;
 DASample(): FunctionPass(ID){}
  ~DASample(){}

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

