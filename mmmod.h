#pragma once
#include <windef.h> // for HINSTANCE

typedef void* mmm_hook;

typedef struct mmm_api {
  void* _mmm_bookkeeping_;
  mmm_hook (*hook_jmp) (size_t orig_address, size_t hook_address);
  mmm_hook (*hook_call) (size_t orig_address, size_t hook_address);
  mmm_hook (*hook_vtbl) (size_t orig_address, size_t hook_address);
  mmm_hook (*patch_bytes) (size_t orig_address, const void* buffer, size_t buf_size);
  void (*revert_hook) (mmm_hook hook);
} mmm_api;

typedef struct mmm_meta {
  const char* version;
  const char* game_base_dir;
  const char* mod_base_dir;
  const char* mod_short_name;
} mmm_meta;

typedef struct mmm_mod_info {
  HINSTANCE instance;
  mmm_meta* meta;
  const char* name;
  const char* version;
  mmm_api* api;
} mmm_mod_info;
