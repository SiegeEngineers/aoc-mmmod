#pragma once

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
} mmm_mod_info;
