#ifndef __REOMP_DRACE__
#define __REOMP_DRACE__

typedef enum {
  REOMP_DRACE_LOG_ARCHER,     // Archer
  REOMP_DRACE_LOG_HELGRIND, // Helgrind
  REOMP_DRACE_LOG_INSPECTOR // Intel Inspector
} reomp_drace_log_type;

#ifdef __cplusplus
extern "C" {
#endif

void reomp_drace_parse(reomp_drace_log_type type);
int  reomp_drace_is_data_race(const char* dir, const char* file, const char* func, int line, int column);
int reomp_drace_is_in_racy_callstack(const char* func_name);
size_t reomp_drace_get_num_locks();


#ifdef __cplusplus
}
#endif

#endif
