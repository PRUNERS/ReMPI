#ifdef __cplusplus
extern "C" {
#endif

void reomp_mem_init();
#define REOMP_MEM_PRINT_ADDR  "reomp_mem_print_addr"
void reomp_mem_print_addr(void* ptr);
void reomp_mem_dump_heap(FILE *fp);
void reomp_mem_restore_heap(FILE *fp);
#define REOMP_MEM_REGISTER_LOCAL_VAR_ADDR "reomp_mem_register_local_var_addr"
void reomp_mem_register_local_var_addr(void* local_ptr, size_t size);
void reomp_mem_record_or_replay_all_local_vars(FILE *fp, int is_replay);
void reomp_mem_rr_loc();
#define REOMP_MEM_ENABLE_HOOK  "reomp_mem_enable_hook"
void reomp_mem_enable_hook(void);
#define REOMP_MEM_DISABLE_HOOK "reomp_mem_disable_hook"
void reomp_mem_disable_hook(void);
void reomp_mem_on_alloc(void* ptr, size_t size);
void reomp_mem_on_free(void* ptr);
bool reomp_mem_is_alloced(void* ptr);
void reomp_mem_get_alloc_size(void* ptr, size_t* size);

#ifdef __cplusplus
}
#endif
