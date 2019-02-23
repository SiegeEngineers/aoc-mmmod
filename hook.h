#pragma once
#include <stdio.h>

typedef void* hook_t;

/**
 * Install a hook that works by JMP-ing to the new implementation.
 * This is handy for hooking existing functions, override their first bytes by a JMP and all the arguments will still be on the stack/in registries. C functions can just be used (with appropriate calling convention) as if they were called directly by the application.
 */
hook_t install_jmphook (void* orig_address, void* hook_address);

/**
 * Install a hook that works by CALL-ing to the new implementation.
 * Handy for hooking existing CALL-sites, overriding essentially just the address.
 */
hook_t install_callhook (void* orig_address, void* hook_address);

/**
 * Install a hook that works by overriding a pointer in a vtable.
 * Handy for hooking class methods.
 */
hook_t install_vtblhook (void* orig_address, void* hook_address);

/**
 * Revert an existing hook.
 */
void revert_hook (hook_t hook);
