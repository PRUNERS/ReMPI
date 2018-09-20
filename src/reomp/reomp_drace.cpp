#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <ctype.h>

#include <unordered_map>
#include <unordered_set>
#include <list>

#include "mutil.h"
#include "reomp_drace.h"
#include "reomp.h"

#define CALL_FUNC_TYPE_FAILED_TO_RESTORE (-1) // CALL_FUNC_TYPE_XXXX must be a negative number

#define ARCHRE_MSG_FAILED_TO_RESTORE ("    [failed to restore the stack]")
#define ARCHRE_MSG_READ_OF_SIZE ("  Read of size ")
#define ARCHRE_MSG_WRITE_OF_SIZE ("  Write of size ")
#define ARCHRE_MSG_PREVIOUS_WRITE_OF_SIZE ("  Previous write of size ")
#define ARCHRE_MSG_PREVIOUS_READ_OF_SIZE ("  Previous read of size ")
#define ARCHRE_MSG_ATOMIC_WRITE_OF_SIZE ("  Atomic write of size ")
#define ARCHRE_MSG_ATOMIC_READ_OF_SIZE ("  Atomic read of size ")
#define ARCHRE_MSG_PREVIOUS_ATOMIC_WRITE_OF_SIZE ("  Previous atomic write of size ")
#define ARCHRE_MSG_PREVIOUS_ATOMIC_READ_OF_SIZE ("  Previous atomic read of size ")


#define DRACE_PATH_MAX (4096)
#define DRACE_STL_LEVEL_INSTRUMENTATION



#define DRACE_PARALLEL   (3)
#define DRACE_SERIAL     (4)


unordered_set<const char*> racy_callstack_uset;

int reomp_drace_is_in_racy_callstack(const char* func_name)
{
  return (racy_callstack_uset.find(func_name) == racy_callstack_uset.end())? 0:1;
}



class call_func 
{
public:
  unsigned int hash_val;
  int level;
  const char* name;
  char* file_path;
  int loc;
  int column;
  char* addr;
  int is_valid;
  int is_stl; 
  int is_candidate;
  int is_noname;
  uint64_t access; //DRACE_WRITE_RACE, DRACE_READ_RACE, DRACE_NULL or DRACE_WRITE_RACE | DRACE_READ_RACE
  int call_stl = 0;
  static call_func* null_call_func;
  static call_func* get_null_call_func() {
    if (null_call_func == NULL) {
      call_func::null_call_func = new call_func(-1, NULL, NULL, -1, -1, NULL);
    }
    return call_func::null_call_func;
  }

  static unsigned int  cf_hash(const char* name, const char* file_path, int loc, int column, const char* addr) {
    unsigned int  v;
    v = 15;
    //    v = reomp_util_hash(v, level);
    //    v = reomp_util_hash(v, (name == NULL)?      v:reomp_util_hash_str(name,      strlen(name     )));
    v = reomp_util_hash(v, (file_path == NULL)? v:reomp_util_hash_str(file_path, strlen(file_path)));
    v *= -298748;
    v = reomp_util_hash(v, loc);
    v *= -23593093;
    v = reomp_util_hash(v, column);
    v *= -112233;
    v = reomp_util_hash(v, (addr      == NULL)? v:reomp_util_hash_str(addr      , strlen(addr)));
    return v;
  }
  
  static call_func* create(int type) {
    switch(type) {
    case CALL_FUNC_TYPE_FAILED_TO_RESTORE:
      return new call_func(type, ARCHRE_MSG_FAILED_TO_RESTORE, NULL, -1, -1, NULL);
      break;
    }
    return NULL;
  }

  call_func(int level, const char* name, char* file_path, int loc, int column, char* addr)
    : level(level)
    , name(name)
    , file_path(file_path)
    , loc(loc)
    , column(column) 
    , addr(addr) 
    , is_valid(0)
    , is_stl(0)
    , is_noname(0)
  {
    hash_val = cf_hash(name, file_path, loc, column, addr);
    if (name == NULL) { 
      is_noname = 1;
      return;
    }
    if (file_path == NULL) return;
    is_valid = 1;
    if (loc == -1) return;
    if (column == -1) return;
    if (strstr(name, "std::") != NULL) { 
      is_stl = 1;
      return;
    }
  };

  void print() {
    MUTIL_DBG("%d %d %s %d %d <%s, %s>", hash_val, level, file_path, loc, column, addr, name);
    //    MUTIL_DBG("%d %d %s %d %d <%s, %s> (is_valid: %d, is_stl: %d)", hash_val, level, file_path, loc, column, addr, name, is_valid, is_stl);
  }
};
call_func* call_func::null_call_func = NULL;

class data_race
{
private:

  call_func* get_first_candidate_call_func(int index) {
    list<call_func*>::reverse_iterator it, it_end;
    call_func *candidate_cfunc = NULL;
    it     = call_stack_list[index].rbegin();
    it_end = call_stack_list[index].rend();
    //    MUTIL_DBG("############ INDEX: %d (size: %lu)", index, call_stack_list[index].size());
    for (; it != it_end; it++) {
      call_func *cfunc = *it;
#ifdef DRACE_STL_LEVEL_INSTRUMENTATION
      if (cfunc->is_valid) {
	if (cfunc->is_stl) {
	  cfunc->call_stl = 1;
	  break;
	}
	candidate_cfunc = cfunc;
	racy_callstack_uset.insert(cfunc->name);
      }
      //      cfunc->print();
#else
      if (cfunc->is_valid) {
	candidate_cfunc = cfunc;
      }
#endif
    }
    //    MUTIL_DBG("###################");
    if (candidate_cfunc == NULL) {
      //      MUTIL_DBG("No candidate call func");
      //      this->print();
    } 
    // if (candidate_cfunc != NULL) {
    //   MUTIL_DBG("--- candidate ---");
    //   candidate_cfunc->print();
    //   MUTIL_DBG("-----------------");
    // }
    if (candidate_cfunc != NULL) {
      candidate_cfunc->access  = (data_race_rw[index] == DRACE_WRITE_RACE)?(1 << 0):(1 << 1);
      candidate_cfunc->access |= (candidate_cfunc->call_stl)?(1 << 2):0;
    }
    return candidate_cfunc;
  }


public:
  unsigned int hash_val;
  int data_race_rw[2];
  int parallel_or_serial[2];
  list<call_func*> call_stack_list[2];

  data_race() {
    for (int i = 0; i < 2; i++) {
      data_race_rw[i] = DRACE_NULL;
      parallel_or_serial[i] = DRACE_NULL;
    }
  }
    
  void filter_out_deterministic_call_stack() {
    this->print();
    for (int sindex = 0; sindex < 2; sindex++) {
      if (data_race_rw[sindex] == DRACE_NULL || 
      	  parallel_or_serial[sindex] == DRACE_NULL) {
      	/* record this unknown race just in case */
      	continue;
      } else 

	if (data_race_rw[sindex]      == DRACE_WRITE_RACE && 
		 parallel_or_serial[sindex] == DRACE_PARALLEL) {
	/* record this write*/
	continue;
      } else if (data_race_rw[sindex]                  == DRACE_READ_RACE && 
		 parallel_or_serial[sindex]            == DRACE_PARALLEL && 
		 (data_race_rw[(sindex + 1) % 2]       == DRACE_WRITE_RACE || data_race_rw[(sindex + 1) % 2]       == DRACE_NULL) && 
		 (parallel_or_serial[(sindex + 1) % 2] == DRACE_PARALLEL   || parallel_or_serial[(sindex + 1) % 2] == DRACE_NULL)) {
	  //		 data_race_rw[(sindex + 1) % 2]       == DRACE_WRITE_RACE  && 
	  //		 parallel_or_serial[(sindex + 1) % 2] == DRACE_PARALLEL) {
	/* record this read */
	continue;
      } else {
	/* Do not record */
	/* TODO: free element */
	call_stack_list[sindex].clear();
	MUTIL_DBG("  ===> Stack %d is filtered out: (write(%d)/read(%d): %d, parallel(%d)/serial(%d): %d)", 
		  sindex, 
		  DRACE_WRITE_RACE, DRACE_READ_RACE, data_race_rw[sindex], 
		  DRACE_PARALLEL, DRACE_SERIAL, parallel_or_serial[sindex]);
      }
    }
    fprintf(stderr, "\n\n");
  }

  void compute_hash_val() {
    unsigned int hash_vals[2] = {15, 15};
    int sindex = 0;
    list<call_func*>::iterator it, it_end;
    unsigned int low_hash, high_hash;
    for (int sindex = 0; sindex < 2; sindex++) {
      for (it = call_stack_list[sindex].begin(), it_end = call_stack_list[sindex].end();
	   it != it_end; it++) {
	call_func *cfunc;
	cfunc = *it;
	hash_vals[sindex] = reomp_util_hash(hash_vals[sindex], cfunc->hash_val);
      }
    }
    int flag = 0;
    if (hash_vals[0] > hash_vals[1]) flag=1;
    low_hash  = hash_vals[ flag      % 2];
    high_hash = hash_vals[(flag + 1) % 2];
    hash_val  = reomp_util_hash(low_hash, high_hash);    
    if (hash_val == 0) MUTIL_DBG("zero: %d %d", low_hash, high_hash);
    return;
  }

  void get_first_call_func(call_func **cf1, call_func **cf2) {
    *cf1 = call_stack_list[0].front();
    *cf2 = call_stack_list[1].front();
    return;
  }


  void get_first_candidate_call_funcs(call_func **cf1, call_func **cf2) {
    *cf1 = get_first_candidate_call_func(0);
    *cf2 = get_first_candidate_call_func(1);
    return;
  }

  void print() {
    list<call_func*>::iterator it, it_end;
    MUTIL_DBG("## hash val: %d ##", this->hash_val);
    for (int sindex = 0; sindex < 2; sindex++) {
      MUTIL_DBG("=== Stack %d (write(%d)/read(%d): %d) ===", sindex, DRACE_WRITE_RACE, DRACE_READ_RACE, data_race_rw[sindex]);
      for (it = call_stack_list[sindex].begin(), it_end = call_stack_list[sindex].end();
           it != it_end; it++) {
        call_func *cfunc;
        cfunc = *it;
	cfunc->print();
      }
    }
  }

};

class data_race_list
{
  list<data_race*> drace_list;
};

static char    *drace_log_line      = NULL;
static size_t   drace_log_line_len  = 0;
static unordered_map<size_t, call_func*> drace_data_race_access_umap;
static unordered_map<size_t, data_race*> drace_data_race_umap;
static list<unordered_set<unsigned int>*> drace_data_race_lock_sets;
static unordered_map<unsigned int, int> drace_data_race_lock_set_id;
static size_t num_locks = 0;



static void drace_parse_archer_error()
{
  MUTIL_ERR("Please specify Archer log: TSAN_OPTIONS=\"log_path=<log_name>\"");
  return;
}

static int drace_extract_int(char* line, int index_start, int length)
{
  char* str;
  int integer;
  str = (char*)reomp_util_malloc(length + 1);
  strncpy(str, &line[index_start], length);
  str[length] = '\0';
  integer = atoi(str);
  free(str);
  return integer;
}

static call_func* drace_extract_call_func(char* line, ssize_t line_length)
{
  int id;
  char *name; 
  char *file_path, *file_path_real;
  int loc;
  int column;
  char *addr;
  int index = 0;
  int index_start, index_end;
  int length;
  
  //  MUTIL_DBG("%s", line);

  while(line[index++] != '#');

  index_start = index; /* Start of id */
  while(line[index++] != ' ');
  length = index - index_start - 1;
  id = drace_extract_int(line, index_start, length);
  //  MUTIL_DBG("--> %d", id);

  index_start = index; /* Start of func name */
  while(1) {
    if (line[index] == '<') {
      if (reomp_util_str_starts_with(&line[index], "<null>")) break;
    } else if (line[index] == '/') {
      break;
    }
    index++;
  }
  length = index - index_start;
  name = (char*)reomp_util_malloc(length + 1);
  strncpy(name, &line[index_start], length);
  name[length] = '\0';
  //  MUTIL_DBG("--> %s", name);
  
  index_start = index; /* Start of file_path */
  index = line_length - 1;
  while(line[index] != ':' && index_start != index) index--;
  if (index_start == index) {
    /* <null case> */
    return new call_func(id, name, NULL, -1, -1, NULL);
  }
  index_end = index;
  index--;
  while(line[index] != ':' && index_start != index) index--;
  if (index_start == index) {
    index = index_end;
  }
  length = index - index_start;
  file_path = (char*)reomp_util_malloc(length + 1);
  strncpy(file_path, &line[index_start], length);
  file_path[length] = '\0';
  file_path_real = realpath(file_path, NULL);
  reomp_util_free(file_path);
  //  if (file_path_real == NULL) {
  //    MUTIL_DBG("%s", line);
  //    MUTIL_ERR("%s: path = %s", strerror(errno), file_path);
  //  }

  
  index++;
  index_start = index; /* Start of loc */
  while(line[index] != ':' && line[index] != ' ' ) index++;
  length = index - index_start;
  loc = drace_extract_int(line, index_start, length);
  //  MUTIL_DBG("--> %d", loc);


  if (line[index] == ':') {
    index++;
    index_start = index; /* Start of coumn */
    //    MUTIL_DBG("line: %s", &line[index_start]);
    while(line[index] != ' ') index++;
    length = index - index_start;
    column = drace_extract_int(line, index_start, length);
  } else if(line[index] == ' ') {
    column = -1;
  } else {
    MUTIL_ERR("parse error: %s", line);
    column = -1;
  }
  //  MUTIL_DBG("--> %d", column);

  /* TODO: in case where two racy lines of code are both on the same LoC */
  addr = NULL;

  //  MUTIL_DBG("%s", line);
  //  MUTIL_DBG("%d %s %s %d %d", id, name, file_path_real, loc, column);

  return new call_func(id, name, file_path_real, loc, column, addr);
}

static ssize_t drace_getline(char **lineptr, size_t *n, FILE *stream) {
  ssize_t read_size;
  read_size = getline(lineptr, n, stream);
  if (read_size > 0) {
    if (reomp_util_str_starts_with(*lineptr, "FATAL")) {
      MUTIL_ERR("Archer output file includes FATAL error: %s", *lineptr);
    }
  }
  //  MUTIL_DBG("--> %s (%d)", *lineptr, read_size);
  return read_size;
}

static int drace_move_to_next_data_race(FILE *fd)
{
#define SEPARATROR "=================="
#define TERMINATOR "ThreadSanitizer: reported"
  call_func* cfunc;
  ssize_t read_size;

  while ((read_size = drace_getline(&drace_log_line, &drace_log_line_len, fd)) != -1) {
    if (reomp_util_str_starts_with(drace_log_line, SEPARATROR)) {
      read_size = drace_getline(&drace_log_line, &drace_log_line_len, fd);
      if (reomp_util_str_starts_with(drace_log_line, TERMINATOR) || read_size == -1) {    
	return 0;
      } else if (reomp_util_str_starts_with(drace_log_line, SEPARATROR)) {
	read_size = drace_getline(&drace_log_line, &drace_log_line_len, fd);
	return 1;
      } else {
	return 1;
      }
    }
  }
  MUTIL_DBG("=========================================================================================");
  MUTIL_DBG("Reached to the end of the file before Summary report: the data race file may be truncated.");
  MUTIL_DBG("=========================================================================================");
  return 0;
}

static call_func* drace_get_next_call_func(FILE *fd)
{
  call_func* cfunc;
  ssize_t read_size;

  while ((read_size = drace_getline(&drace_log_line, &drace_log_line_len, fd)) != -1) {
    if (reomp_util_str_starts_with(drace_log_line, "    #")) {
      return drace_extract_call_func(drace_log_line, read_size);
    } else if (reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_FAILED_TO_RESTORE)) {
      MUTIL_DBG("Warning: Archer failed to restore the stack");
      // new call_func(-1, NULL, NULL, -1, -1, NULL);
      return call_func::get_null_call_func();
      //      return call_func::create(CALL_FUNC_TYPE_FAILED_TO_RESTORE);
    } else if (reomp_util_str_starts_with(drace_log_line, "\n")) {
      return NULL;
    }
  }
  MUTIL_ERR("Unknown Archer reports: %s", drace_log_line);
  //  return NULL;
  //  return new call_func(-1, NULL, NULL, -1, -1, NULL);
  return call_func::get_null_call_func();
}

static int drace_is_write_or_read(FILE *fd)
{
  call_func* cfunc;
  ssize_t read_size;

  while ((read_size = drace_getline(&drace_log_line, &drace_log_line_len, fd)) != -1) {
    if (reomp_util_str_starts_with(drace_log_line, "    #") || reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_FAILED_TO_RESTORE)) {
      goto end;
    } else if (reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_READ_OF_SIZE) || 
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_PREVIOUS_READ_OF_SIZE) ||
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_ATOMIC_READ_OF_SIZE) ||
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_PREVIOUS_ATOMIC_READ_OF_SIZE)) {
      return DRACE_READ_RACE;
    } else if (reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_WRITE_OF_SIZE) || 
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_PREVIOUS_WRITE_OF_SIZE) ||
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_ATOMIC_WRITE_OF_SIZE) ||
	       reomp_util_str_starts_with(drace_log_line, ARCHRE_MSG_PREVIOUS_ATOMIC_WRITE_OF_SIZE)) {
      return DRACE_WRITE_RACE;
    }
  }
 end:
  MUTIL_ERR("Archer file format is broken: '$%s', '%s', '%s', '%s', '%s' '%s' '%s' and '%s' cannot be found", 
	    ARCHRE_MSG_READ_OF_SIZE,
	    ARCHRE_MSG_WRITE_OF_SIZE,
	    ARCHRE_MSG_PREVIOUS_READ_OF_SIZE,
	    ARCHRE_MSG_PREVIOUS_WRITE_OF_SIZE,
	    ARCHRE_MSG_ATOMIC_READ_OF_SIZE,
	    ARCHRE_MSG_ATOMIC_WRITE_OF_SIZE,
	    ARCHRE_MSG_PREVIOUS_ATOMIC_READ_OF_SIZE,
	    ARCHRE_MSG_PREVIOUS_ATOMIC_WRITE_OF_SIZE);
  return -1;
}

static void drace_set_callstack(data_race *drace, FILE *fd, int index) 
{
  call_func *cfunc;
  drace->data_race_rw[index] = drace_is_write_or_read(fd);
  drace->parallel_or_serial[index] = DRACE_SERIAL; /* Init with DRACE_SERIAL */
  while ((cfunc = drace_get_next_call_func(fd)) != NULL) {
    if (cfunc == call_func::get_null_call_func()) {    
      drace->parallel_or_serial[index] = drace->data_race_rw[index] = DRACE_NULL;
    } else {
      if (reomp_util_str_starts_with(cfunc->name, ".omp_outlined.")) {
	drace->parallel_or_serial[index] = DRACE_PARALLEL;
      } 
    }
    drace->call_stack_list[index].push_back(cfunc);
  }
  return;
}

static void drace_is_false_positve(data_race *drace) 
{
  
}

static void drace_parse_archer_file(char* log_dir, char* log_file)
{
  FILE *fd;
  char log_file_path[DRACE_PATH_MAX];
  call_func *cfunc;
  data_race *drace;
  int previous_level = -1;
  int index = 0;
  int duplication = 0, sum=0;

  sprintf(log_file_path, "%s/%s", log_dir, log_file);
  fd = fopen(log_file_path,"r");
  if (fd == NULL) {
    MUTIL_ERR("fopen failed: %s", log_file_path);
  }

  while(drace_move_to_next_data_race(fd)) {
    drace = new data_race();
    /* Get first racy call stack */
    drace_set_callstack(drace, fd, 0);
    /* Get second racy call stack */
    drace_set_callstack(drace, fd, 1);

    drace->compute_hash_val();    
    drace->filter_out_deterministic_call_stack();

    
    if (drace_data_race_umap.find(drace->hash_val) != drace_data_race_umap.end()) {
      // MUTIL_DBG("######## FRIST ###############################");
      // drace_data_race_umap[drace->hash_val]->print();
      // MUTIL_DBG("######## SECOND ###############################");
      // drace->print();
      duplication++;
      delete(drace);
    } else {
      drace_data_race_umap[drace->hash_val] = drace;
      //      drace->print();
    }
    sum++;
  }
  fclose(fd);
  MUTIL_DBG("%s has %d duplicated reports out of %d", log_file_path, duplication, sum);
  return;
}

static void drace_print_data_race_access()
{
  MUTIL_DBG("====== Racy accesses ========");
  unordered_map<size_t, call_func*>::iterator it, it_end;
  it     = drace_data_race_access_umap.begin();
  it_end = drace_data_race_access_umap.end();
  for (; it != it_end; it++) {
    call_func *cfunc = it->second;
    cfunc->print();
  }
  MUTIL_DBG("=============================");  
}

static void drace_add_data_race_access(call_func* cfunc)
{
  if (cfunc == NULL) return;
  if (drace_data_race_access_umap.find(cfunc->hash_val) == drace_data_race_access_umap.end()) {
    drace_data_race_access_umap[cfunc->hash_val] = cfunc;
    //    fprintf(stderr, "    #0 %s %s:%d:%d\n", cfunc->name, cfunc->file_path, cfunc->loc, cfunc->column );
  } else {
    // add information about read, write or NULL
    drace_data_race_access_umap[cfunc->hash_val]->access = drace_data_race_access_umap[cfunc->hash_val]->access | cfunc->access;
  }


  return;
}

static void drace_update_data_race_lock_sets(call_func *cfunc1, call_func *cfunc2)
{
  list<unordered_set<unsigned int>*> inclusion_set_list;
  list<unordered_set<unsigned int>*>::iterator it, it_end;
  bool include_cfunc1 = false, include_cfunc2 = false;
  

  // if (cfunc1 != NULL) {
  //   MUTIL_DBG("Input 1: %d", cfunc1->hash_val);
  // }
  // if (cfunc2 != NULL) {
  //   MUTIL_DBG("Input 2: %d", cfunc2->hash_val);
  // }
  
  inclusion_set_list.clear();
  for (it     = drace_data_race_lock_sets.begin(),
       it_end = drace_data_race_lock_sets.end();
       it != it_end;
       it++ ) {
    unordered_set<unsigned int> *s = *it;
    include_cfunc1 = false; include_cfunc2 = false;
    if (cfunc1 != NULL) {
      if (s->find(cfunc1->hash_val) != s->end()) include_cfunc1 = true;
    }
    if (cfunc2 != NULL) {
      if (s->find(cfunc2->hash_val) != s->end()) include_cfunc2 = true;
    }
    if (include_cfunc1 || include_cfunc2) {
      inclusion_set_list.push_back(s);
    }
  }

  //  MUTIL_DBG(" Inclusion: %lu / %lu", inclusion_set_list.size(), drace_data_race_lock_sets.size());
  if (inclusion_set_list.size() == 0) {
    unordered_set<unsigned int> *new_set = new unordered_set<unsigned int>;
    if (cfunc1 != NULL) new_set->insert(cfunc1->hash_val);
    if (cfunc2 != NULL) new_set->insert(cfunc2->hash_val);
    drace_data_race_lock_sets.push_back(new_set);
  } else if (inclusion_set_list.size() == 1) {
    unordered_set<unsigned int> *set = inclusion_set_list.front();
    if (cfunc1 != NULL) set->insert(cfunc1->hash_val);
    if (cfunc2 != NULL) set->insert(cfunc2->hash_val);
  } else if (inclusion_set_list.size() == 2) {
    unordered_set<unsigned int> *new_set = new unordered_set<unsigned int>;
    list<unordered_set<unsigned int>*>::iterator it, it_end;
    for (it     = inclusion_set_list.begin(),
	 it_end = inclusion_set_list.end();
	 it != it_end; it++) {
      unordered_set<unsigned int> *in_set = *it;
      new_set->insert(in_set->begin(), in_set->end());
      drace_data_race_lock_sets.remove(in_set);
      delete in_set;
    }
    if (cfunc1 != NULL) new_set->insert(cfunc1->hash_val);
    if (cfunc2 != NULL) new_set->insert(cfunc2->hash_val);
    drace_data_race_lock_sets.push_back(new_set);
  } else {
    MUTIL_ERR("Error");
  }
  
  return;
}

static void drace_create_data_race_lock_set_id()
{
  int id = REOMP_RR_LOCK_DATARACE_BEGIN;
  list<unordered_set<unsigned int>*>::iterator it, it_end;
  unordered_set<unsigned int>::iterator it2, it2_end;
  for (it     = drace_data_race_lock_sets.begin(),
       it_end = drace_data_race_lock_sets.end();
       it != it_end;
       it++ ) {  
    unordered_set<unsigned int> *s = *it;
    for (it2 = s->begin(), it2_end = s->end(); it2 != it2_end; it2++) {
      unsigned int hash_val = *it2;
      drace_data_race_lock_set_id[hash_val] = id;
    }
    id++;
  }
  num_locks = drace_data_race_lock_sets.size();
}

size_t reomp_drace_get_num_locks()
{
  return num_locks;
}

static void  reomp_parse_archer_post_process()
{
  unordered_map<size_t, data_race*>::iterator it, it_end;
  for (it     = drace_data_race_umap.begin(), 
       it_end = drace_data_race_umap.end();
       it != it_end;
       it++) {
    call_func *cfunc1, *cfunc2;
    data_race *dr = it->second;
    //    dr->get_first_call_func(&cfunc1, &cfunc2);
    dr->get_first_candidate_call_funcs(&cfunc1, &cfunc2);
    drace_add_data_race_access(cfunc1);
    drace_add_data_race_access(cfunc2);
    drace_update_data_race_lock_sets(cfunc1, cfunc2);
  }
  drace_create_data_race_lock_set_id();
  //  drace_print_data_race_access();
}

static void drace_parse_archer_log()
{
  char* env;
  char* log_path, *log_path_start, *log_path_end;
  char log_path_name[DRACE_PATH_MAX];
  char* log_dir, *log_name, *log_dir_dup, *log_name_dup;
  DIR *dir;
  int is_archer_file = 0;
  struct dirent *dp;

  if (NULL == (env = getenv("TSAN_OPTIONS"))) {
    // drace_parse_archer_error();
    return;
  } 
  /* e.g.)  env="history_size=7 log_path=rempi_drace_test.log" */
  log_path = strstr(env, "log_path");
  if(log_path == NULL) {
    drace_parse_archer_error();
  }
  /* e.g.)  log_path="log_path=rempi_drace_test.log" */
  log_path_start = strstr(log_path, "=");
  if(log_path_start == NULL) {
    drace_parse_archer_error();
  }
  log_path_end = strstr(log_path, " ");
  if(log_path_end == NULL) {
    log_path_end = log_path + strlen(log_path);
  }
  //  MUTIL_DBG("==> %s %p %p", log_path);
  /* e.g.)  log_name="rempi_drace_test.log" */


  memset(log_path_name, 0, DRACE_PATH_MAX);
  strncpy(log_path_name, log_path_start + 1, log_path_end - log_path_start - 1);
  //  log_path_name[log_path_end - log_path_start] = '\0';
  log_dir_dup  = strdup(log_path_name);
  log_dir      = dirname(log_dir_dup);
  log_name_dup = strdup(log_path_name);
  log_name = basename(log_name_dup);
  /* e.g.)  log_dir=".", log_name="rempi_drace_test.log" */

  dir = opendir(log_dir);
  size_t log_name_len = strlen(log_name);
  //  MUTIL_DBG("==========================");
  while ((dp = readdir(dir)) != NULL) {
    is_archer_file = 1;
    size_t len = strlen(dp->d_name);
    if (!reomp_util_str_starts_with(dp->d_name, log_name))  {
      //      MUTIL_DBG("%s / %s", dp->d_name, log_name);
      continue;
    } else {
      //      MUTIL_DBG("Matched: %s / %s", dp->d_name, log_name);
    }

    for (size_t i = 1; i < len - log_name_len; i++) {
      if (!isdigit(dp->d_name[log_name_len + i])) {
	is_archer_file = 0;
      }
    }
    if (is_archer_file) {
      //      MUTIL_DBG("open: %s", dp->d_name);
      drace_parse_archer_file(log_dir, dp->d_name);
    }
  }
  free(drace_log_line);
  closedir(dir);  
  //  MUTIL_DBG("==========================");

  reomp_parse_archer_post_process();
  
  return;
}

void reomp_drace_parse(reomp_drace_log_type type)
{
  switch(type) {
  case REOMP_DRACE_LOG_ARCHER:
    drace_parse_archer_log();
    break;
  case REOMP_DRACE_LOG_HELGRIND:
    MUTIL_ERR("Not helgrind is supported");
    break;
  case REOMP_DRACE_LOG_INSPECTOR:
    MUTIL_ERR("Not inspector is supported");
    break;
  }
}

//int  reomp_drace_is_data_race(const char* func, const char* dir, const char* file, int line, int column, uint64_t* access)
int  reomp_drace_is_data_race(const char* func, const char* dir, const char* file, int line, int column, uint64_t* access)
{
  unsigned int hash_val;
  char* file_path = NULL;
  char *file_path_real;
  size_t len;

  if (reomp_util_str_starts_with(file, "/")) {
    len = strlen(file);
    file_path_real = realpath(file, NULL);
  } else {
    len = strlen(dir) + strlen(file);
    file_path = (char*)reomp_util_malloc(len + 2);
    sprintf(file_path, "%s/%s", dir, file);
    file_path[len + 1] = '\0';
    file_path_real = realpath(file_path, NULL);
  }

  if (file_path_real == NULL) {
    MUTIL_ERR("realpath failed");
  }


  hash_val = call_func::cf_hash(func, file_path_real, line, column, NULL);


  if (drace_data_race_access_umap.find(hash_val) != drace_data_race_access_umap.end()) {
    int lock_set_id;
    call_func *cfunc;

    lock_set_id = drace_data_race_lock_set_id.at(hash_val);
    cfunc       = drace_data_race_access_umap.at(hash_val);
    
    MUTIL_DBG("############ HIT: %d #############", hash_val);
    MUTIL_DBG(" Line of instrumented code: %s:%d:%d (lock_set: %d)", file_path_real, line, column, lock_set_id);
    cfunc->print();
    MUTIL_DBG("##############################");
    
    return lock_set_id;
  } 
  if (file_path != NULL) {
    reomp_util_free(file_path);
  }
  reomp_util_free(file_path_real);

  return 0;
}





