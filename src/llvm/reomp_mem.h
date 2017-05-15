
#ifdef __cplusplus
extern "C" {
#endif

void reomp_mem_on_alloc(void* ptr, size_t size);
void reomp_mem_on_free(void* ptr);
bool reomp_mem_is_alloced(void* ptr);
void reomp_mem_get_alloc_size(void* ptr, size_t* size);

#ifdef __cplusplus
}
#endif
