#include "libc/calls/calls.h"
#include "libc/stdio/stdio.h"
#include "libc/str/str.h"
#include "libc/errno.h"
#include "libc/mem/mem.h"
#include "libc/mem/gc.h"
#include "libc/sysv/consts/ok.h"
#include "libc/runtime/runtime.h"

#define OPT(x) opts[(int)(x)]

int ParseOpt(int len, char* arg, int* opts) {
  if(len==0 || arg[0] != '-') {
    return 0;
  }

  for(int j = 1; j < len; ++j) {
    OPT(arg[j]) = 1;
  }
  return 1;
}

void ParseOpts(int *argc, char** argv, int* opts) {
  int j = 0;
  for(int i = 0; i < *argc; ++i) {
    char* arg = argv[i];
    int alen = strlen(arg);
    if(!ParseOpt(alen, arg, opts)) {
      argv[j++] = arg;
    }
  }
  *argc = j;
}

int MaxArgLen(int argc, char** argv) {
  int max = 0;
  for(int i = 0; i < argc; ++i) {
    int len = strlen(argv[i]);
    if(len > max) max = len;
  }
  return max;
}

int CheckFile(char *path, int sep_idx, int *opts) {
  int mode = OPT('r')*R_OK | OPT('w')*W_OK | OPT('x')*X_OK;
  int use_dir = OPT('d') | OPT('/');
  int brk = 0;
  if(0 == access(path, mode)) {
    if(use_dir) {
      path[sep_idx + OPT('/')] = 0;
      brk = 1;
    }
    printf("%s\n", path);
    if(!OPT('a')) exit(0);
  }

  return brk;
}

char* GetWD() {
  int wd_max = 256;
  char* wd = NULL;
  WD_LOOP:
  wd = realloc(wd, wd_max);
  if(!getcwd(wd, wd_max)) {
    if(errno != ERANGE) {
      _exit(2);
    }
    wd_max *= 2;
    goto WD_LOOP;
  }

  return wd;
}

char *ActualPath(char *path, int padding) {
  char *wd;
  if(path[0] == '/') {
    wd = "";
    path++;
  }
  else {
    wd = _gc(GetWD());
  }
  int wd_len = strlen(wd);
  int path_len = strlen(path);

  char *full_path = _gc(malloc(wd_len + path_len + 2));
  memcpy(full_path, wd, wd_len);
  full_path[wd_len] = '/';
  memcpy(full_path+wd_len+1, path, path_len);
  full_path[wd_len+path_len+2] = 0;

  char *abs_path = malloc(wd_len + path_len + 2 + padding);

  char *dst = abs_path;
  for(;full_path != NULL;) {
    char *tok = strsep(&full_path, "/");
    if(strcmp(tok, ".") == 0) continue;
    if(strcmp(tok, "..") == 0) {
      dst--;
      if(dst <= abs_path) exit(4);
      for(;dst[-1] != '/'; --dst);
      continue;
    }
    int len = strlen(tok);
    memcpy(dst, tok, len);
    dst += len;
    *(dst++) = '/';
    *dst = 0;
  }
  dst[-1] = 0;
  return abs_path;
}

int main(int argc, char **argv) {
  
  argc--; argv++;

  int opts[256];
  ParseOpts(&argc, argv, opts);

  char *filename = "";
  if(OPT('f')) {
    filename = argv[0];
    argc--; argv++;
    if(argc < 0) exit(8);
  }

  char* path = _gc(ActualPath(filename, MaxArgLen(argc, argv)));

  int path_len = strlen(path);

  for(int i = path_len; i >= 0; --i) {
    for(int j = 0; path[i] == '/' && j < argc; ++j) {
      strcpy(path+i+1, argv[j]);
      if(CheckFile(path, i, opts)) break;
    }
  }

  return !OPT('a');
}