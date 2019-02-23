#include <windows.h>
#include <stdio.h>
#include "hook.h"

#ifdef DEBUG
#  define dbg_print(...) printf("[mmmod] " __VA_ARGS__); fflush(stdout)
#else
#  define dbg_print(...)
#endif

typedef void (*cb_mmm_load) (void*);
typedef void (*cb_mmm_before_setup) (void*);
typedef void (*cb_mmm_after_setup) (void*);
typedef void (*cb_mmm_unload) (void*);

static char** up_mod_name = (char**) 0x7A5058;
static char** up_mod_dir = (char**) 0x7A506C;

// should be enough for anyone, I'd hope
static HINSTANCE loaded_mods[100] = {0};

static char ends_with (char* haystack, char* needle) {
  size_t hl = strlen(haystack);
  size_t nl = strlen(needle);

  return strncmp(&haystack[hl - nl], needle, nl) == 0;
}

static void print_error (HRESULT hresult) {
  LPTSTR errorText = NULL;
  FormatMessage(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      hresult,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&errorText,
      0, NULL);

  if (NULL != errorText) {
    fprintf(stderr, "[mmmod] Failed to load mod dll: %s\n", errorText);
    LocalFree(errorText);
    errorText = NULL;
  }
}

typedef int __thiscall (*fn_game_setup) (void*);
static fn_game_setup aoc_game_setup = (fn_game_setup) 0x43AE90;
static int __thiscall game_setup_hook (void* game) {
  dbg_print("setup hook\n");

  char is_up_mod = strncmp(*up_mod_name, "age2_x1", 7) != 0;
  char mmmods_glob[MAX_PATH];
  char dll_name_template[MAX_PATH];
  if (is_up_mod) {
    sprintf(mmmods_glob, "%s\\mmmods\\*.dll", *up_mod_dir);
    sprintf(dll_name_template, "%s\\mmmods\\%%s", *up_mod_dir);
  } else {
    // resolve from cwd
    strcpy(mmmods_glob, "mmmods\\*.dll");
    strcpy(dll_name_template, "mmmods\\%s");
  }

  dbg_print("loading mods from %s\n", mmmods_glob);

  memset(loaded_mods, 0, sizeof(loaded_mods));
  WIN32_FIND_DATA file_data;
  HANDLE dir = FindFirstFile(mmmods_glob, &file_data);
  if (dir == INVALID_HANDLE_VALUE) {
    printf("[mmmod] No mods found in '%s'\n", mmmods_glob);
    return aoc_game_setup(game);
  }

  int i = 0;
  char dll_name[MAX_PATH];
  do {
    if (ends_with(file_data.cFileName, ".dll")) {
      sprintf(dll_name, dll_name_template, file_data.cFileName);
      printf("[mmmod] Loading mod file: %s, index %d\n", dll_name, i);
      loaded_mods[i] = LoadLibrary(dll_name);
      if (loaded_mods[i] == NULL) {
        print_error(HRESULT_FROM_WIN32(GetLastError()));
      } else {
        cb_mmm_load load = (cb_mmm_load) GetProcAddress(loaded_mods[i], "mmm_load");
        if (load) {
          load(NULL);
        }
      }
      i += 1;
    }
  } while (FindNextFile(dir, &file_data));
  loaded_mods[i] = NULL;
  FindClose(dir);

  for (int i = 0; loaded_mods[i] != NULL; i++) {
    cb_mmm_before_setup before_setup = (cb_mmm_before_setup)
      GetProcAddress(loaded_mods[i], "mmm_before_setup");
    if (before_setup) {
      dbg_print("  calling before_setup for index %d\n", i);
      before_setup(NULL);
    }
  }

  dbg_print("starting original setup\n");
  int result = aoc_game_setup(game);
  dbg_print("original setup said %d\n", result);

  if (result == 1) {
    for (int i = 0; loaded_mods[i] != NULL; i++) {
      cb_mmm_after_setup after_setup = (cb_mmm_after_setup)
        GetProcAddress(loaded_mods[i], "mmm_after_setup");
      if (after_setup) {
        dbg_print("  calling after_setup for index %d\n", i);
        after_setup(NULL);
      }
    }
  }

  return result;
}

static void init () {
  dbg_print("init()\n");

  install_callhook((void*) 0x43A6A5, (void*) game_setup_hook);
}

static void deinit() {
  dbg_print("deinit()\n");
  for (int i = 0; loaded_mods[i]; i++) {
    cb_mmm_unload unload = (cb_mmm_unload) GetProcAddress(loaded_mods[i], "mmm_unload");
    if (unload != NULL) {
      unload(NULL);
    }
    FreeLibrary(loaded_mods[i]);
    loaded_mods[i] = NULL;
  }
}

BOOL WINAPI DllMain (HINSTANCE dll, int reason, void*_) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(dll);
    init();
  }
  if (reason == DLL_PROCESS_DETACH) {
    deinit();
  }
  return 1;
}
