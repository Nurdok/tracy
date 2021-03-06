#include "tracy.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


__TRACY_NAMESPACE_BEGIN


enum { 
  MAX_STACK_SIZE = 1024,
  MAX_USER_ERR_MSG_SIZE = 512,
  MAX_ERR_STR_BUF_SIZE = 1024
};


/* Holds a single trace point in the trace stack. */
typedef struct {
  char const * file;
  char const * func;
  int line;
} stack_buff_t;


/* The trace stack. */
typedef struct {
  stack_buff_t buffer[MAX_STACK_SIZE];
} error_stack_t;


/* The initial error message. */
typedef struct {
  char buff[MAX_USER_ERR_MSG_SIZE + 1];
} msg_data_t;


/* ----------------------------- Static Data ------------------------------ */


/* Get the string error corresponding to the numeric value of `err`. */
static char const * get_error_string(int err);


static __thread error_stack_t local_stack;
static __thread int stack_ptr = 0;
static __thread int stack_ptr_save = 0;
static __thread msg_data_t msg_buf;


/* --------------------------- Public Functions --------------------------- */


/* Clear the error traceback. */
void clear_error(void) {
  stack_ptr = 0;
  msg_buf.buff[0] = '\0';
}


/* Save the current stack position in a temporary variable. */
void save_traceback_position(void) {
  stack_ptr_save = stack_ptr;
}


/* Restore the stack position from the temprary variable. */
void restore_traceback_position(void) {
  stack_ptr = stack_ptr_save;
}


/* Format the traceback and print it to stderr. */
void log_traceback(err_t err) {
  int i;
  fprintf(stderr, "CC Traceback:\n");
  for (i = stack_ptr - 1; i >= 0; --i) {
    fprintf(stderr, "  File \"%s\", line %d, in %s\n", 
        local_stack.buffer[i].file,
        local_stack.buffer[i].line,
        local_stack.buffer[i].func);
  }
  
  fprintf(stderr, "Error: %s (%d)\n", get_error_string(err), err);

  if (msg_buf.buff[0] != '\0') {
    fprintf(stderr, "Error message: %s\n", msg_buf.buff);
  }
}


/* Same as `log_traceback`, but also clears the error afterwards. */
void log_and_clear_error(err_t err) {
  log_traceback(err);
  clear_error();
}


/* Same as `log_and_clear_error`, but only if `err` != OK. */
void log_and_clear_on_error(err_t err) {
  if (err != OK) {
    log_and_clear_error(err);
  }
}


/* -------------------------- Private Functions --------------------------- */


/* Start an error traceback. */
void __start_error(char const * file, char const * func, int line) {
  __add_error_trace(file, func, line);
}


/* Add a trace point to the error traceback. */
void __add_error_trace(char const * file, char const * func, int line) {
  if (stack_ptr >= MAX_STACK_SIZE) {
    fprintf(stderr, "Warning: error stack overflow (no room for stack trace)\n");
    return;
  }

  local_stack.buffer[stack_ptr].file = file;
  local_stack.buffer[stack_ptr].func = func;
  local_stack.buffer[stack_ptr].line = line;
  ++stack_ptr;
}


/* Save the initial error message. */
void __set_error_msg(char const * fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg_buf.buff, MAX_USER_ERR_MSG_SIZE, fmt, args);
  va_end(args);
}


/* Get the string error corresponding to the numeric value of `err`. */
static char const * get_error_string(int err) {
  static __thread char errbuf[MAX_ERR_STR_BUF_SIZE + 1];

  #if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE)
  if (strerror_r(err, errbuf, MAX_ERR_STR_BUF_SIZE) != 0) {
    fprintf(stderr, "Error: strerror_r returned errno %d\n", errno);
    return NULL;
  }
  return errbuf;
#else
  return strerror_r(err, errbuf, MAX_ERR_STR_BUF_SIZE);
#endif
}


__TRACY_NAMESPACE_END
