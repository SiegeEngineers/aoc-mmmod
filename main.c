#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "hook.h"
#include "mmmod.h"

#define MMMOD_VERSION "0.1.0"

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
static void** game_instance = (void**) 0x7912A0;
static mmm_meta meta;

// should be enough for anyone, I'd hope
static mmm_mod_info loaded_mods[100];
static HMODULE original_langfile = NULL;
static hook_t hooks[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static size_t count_hooks() {
  size_t i = 0;
  for (; hooks[i] != NULL; i++);
  return i;
}

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

static char do_before_setup_code () {
  memset(&meta, 0, sizeof(meta));
  meta.version = MMMOD_VERSION;
  meta.game_base_dir = ".";
  meta.mod_short_name = *up_mod_name;

  char is_up_mod = strncmp(*up_mod_name, "age2_x1", 7) != 0;
  char fallback_lang_dll[MAX_PATH];
  char mmmods_glob[MAX_PATH];
  char dll_name_template[MAX_PATH];
  if (is_up_mod) {
    meta.mod_base_dir = *up_mod_dir;
    sprintf(mmmods_glob, "%s\\mmmods\\*.dll", *up_mod_dir);
    sprintf(dll_name_template, "%s\\mmmods\\%%s", *up_mod_dir);
    sprintf(fallback_lang_dll, "%s\\Data\\language_x1_m.dll", *up_mod_dir);
  } else {
    meta.mod_base_dir = NULL;
    // resolve from cwd
    strcpy(mmmods_glob, "mmmods\\*.dll");
    strcpy(dll_name_template, "mmmods\\%s");
    strcpy(fallback_lang_dll, "language_x1_m.dll");
  }

  original_langfile = LoadLibrary(fallback_lang_dll);
  if (original_langfile != NULL) {
    hooks[count_hooks()] = install_bytes((void*) 0x43CF11, (void*) &original_langfile, sizeof(HMODULE*));
  }

  dbg_print("loading mods from %s\n", mmmods_glob);

  memset(loaded_mods, 0, sizeof(loaded_mods));
  WIN32_FIND_DATA file_data;
  HANDLE dir = FindFirstFile(mmmods_glob, &file_data);
  if (dir == INVALID_HANDLE_VALUE) {
    printf("[mmmod] No mods found in '%s'\n", mmmods_glob);
    return 0;
  }

  int i = 0;
  char dll_name[MAX_PATH];
  do {
    if (ends_with(file_data.cFileName, ".dll")) {
      sprintf(dll_name, dll_name_template, file_data.cFileName);
      printf("[mmmod] Loading mod file: %s, index %d\n", dll_name, i);
      mmm_mod_info* info = &loaded_mods[i];
      info->meta = &meta;
      info->instance = LoadLibrary(dll_name);
      if (info->instance == NULL) {
        print_error(HRESULT_FROM_WIN32(GetLastError()));
      } else {
        cb_mmm_load load = (cb_mmm_load) GetProcAddress(info->instance, "mmm_load");
        if (load) {
          load(info);
        }
      }

      printf("[mmmod] name: %s, version: %s\n", info->name, info->version);
      i += 1;
    }
  } while (FindNextFile(dir, &file_data));
  FindClose(dir);

  for (int i = 0; loaded_mods[i].instance != NULL; i++) {
    cb_mmm_before_setup before_setup = (cb_mmm_before_setup)
      GetProcAddress(loaded_mods[i].instance, "mmm_before_setup");
    if (before_setup) {
      dbg_print("  calling before_setup for index %d\n", i);
      before_setup(&loaded_mods[i]);
    }
  }

  return 1;
}

static char do_after_setup_code () {
  for (int i = 0; loaded_mods[i].instance != NULL; i++) {
    cb_mmm_after_setup after_setup = (cb_mmm_after_setup)
      GetProcAddress(loaded_mods[i].instance, "mmm_after_setup");
    if (after_setup) {
      dbg_print("  calling after_setup for index %d\n", i);
      after_setup(&loaded_mods[i]);
    }
  }

  return 1;
}

typedef int __thiscall (*fn_game_setup) (void*);
static fn_game_setup aoc_game_setup = (fn_game_setup) 0x43AE90;
static int __thiscall game_setup_hook (void* game) {
  dbg_print("setup hook\n");

  do_before_setup_code();

  dbg_print("starting original setup\n");
  int result = aoc_game_setup(game);
  dbg_print("original setup said %d\n", result);

  return result;
}

typedef int __thiscall (*fn_game_run) (void*);
static fn_game_run aoc_game_run = (fn_game_run) NULL;
static int __thiscall game_run_hook (void* game) {
  assert(aoc_game_run != NULL);

  do_after_setup_code();

  return aoc_game_run(game);
}

typedef void __cdecl (*fn_close_archive)(const char*);
static fn_close_archive aoc_close_archive = (fn_close_archive) 0x543580;
typedef void __cdecl (*fn_open_archive)(const char*, const char*, char*, int);
static fn_open_archive aoc_open_archive = (fn_open_archive) 0x543370;
static void undo_initial_setup () {
  aoc_close_archive("sounds_x1.drs");
  aoc_close_archive("gamedata_x1.drs");
  aoc_close_archive("gamedata_x1_p1.drs");
}

static void redo_initial_setup () {
  void* start_options = *(void**) (*(size_t*) game_instance + 0x28);
  printf("%p\n", start_options);
  char* base_dir = (char*) ((size_t) start_options + 0x1467);
  printf("%p\n", base_dir);
  printf("%s\n", base_dir);

  aoc_open_archive("sounds_x1.drs", "tribe", base_dir, 1);
  aoc_open_archive("gamedata_x1.drs", "tribe", base_dir, 0);
  aoc_open_archive("gamedata_x1_p1.drs", "tribe", base_dir, 1);
}

static void init () {
  dbg_print("init()\n");

  // try to load this thing lol
  /* LoadLibrary("mmmods\\rempires.dll"); */

  if (*game_instance == NULL) {
    // Called before setup, install Game::setup() hook
    hooks[count_hooks()] = install_callhook((void*) 0x43A6A5, (void*) game_setup_hook);
  } else {
    // Called during setup, we need to undo the initial loading stuff and redo it after mods are ready
    undo_initial_setup();
    do_before_setup_code();
    redo_initial_setup();
  }

  void** game_vtbl = *(void**) *game_instance;
  printf("fn_game_run %p [1] %p &[1] %p\n", game_vtbl, game_vtbl[1], &game_vtbl[1]);
  aoc_game_run = (fn_game_run) game_vtbl[1];
  hooks[count_hooks()] = install_vtblhook((void*) &game_vtbl[1], (void*) game_run_hook);
}

static void deinit() {
  dbg_print("deinit()\n");

  for (size_t i = 0; loaded_mods[i].instance != NULL; i++) {
    cb_mmm_unload unload = (cb_mmm_unload) GetProcAddress(loaded_mods[i].instance, "mmm_unload");
    if (unload != NULL) {
      unload(&loaded_mods[i]);
    }
    FreeLibrary(loaded_mods[i].instance);
  }
  memset(loaded_mods, 0, sizeof(loaded_mods));

  for (size_t i = 0; hooks[i] != NULL; i++) {
    revert_hook(hooks[i]);
    hooks[i] = NULL;
  }

  if (original_langfile != NULL) {
    FreeLibrary(original_langfile);
    original_langfile = NULL;
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
