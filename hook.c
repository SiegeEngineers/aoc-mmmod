#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifdef NDEBUG
#define dbg_print(...)
#else
#define dbg_print(...) printf("[mmmod] [hook] " __VA_ARGS__)
#endif

struct hook {
  void* ptr;
  void* orig_data;
  size_t orig_size;
};

typedef struct hook* hook_t;

static hook_t new_hook(void* ptr, size_t size) {
  hook_t hook = (hook_t)calloc(1, sizeof(struct hook));
  hook->ptr = ptr;
  hook->orig_size = size;
  return hook;
}

/**
 * Overwrite some bytes in the current process.
 * Returns a pointer to a buffer containing the original data if it worked, NULL
 * if not. This buffer must be freed by the caller.
 */
static void* overwrite_bytes(void* ptr, void* value, size_t size) {
  assert(ptr != NULL);
  assert(value != NULL);
  assert(size > 0);

  void* orig_data = malloc(size);
  if (orig_data == NULL)
    return NULL;
  DWORD old;
  DWORD tmp;
  if (!VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &old)) {
    dbg_print("Couldn't unprotect?! @ %p\n", ptr);
    return NULL;
  }
  memcpy(orig_data, ptr, size);
  memcpy(ptr, value, size);
  VirtualProtect(ptr, size, old, &tmp);
  return orig_data;
}

/**
 * Install a hook that works by JMP-ing to the new implementation.
 * This is handy for hooking existing functions, override their first bytes by a
 * JMP and all the arguments will still be on the stack/in registries. C
 * functions can just be used (with appropriate calling convention) as if they
 * were called directly by the application.
 */
hook_t install_jmphook(void* orig_address, void* hook_address) {
  assert(orig_address != NULL);
  assert(hook_address != NULL);

  char patch[6] = {
      0xE9,      // jmp
      0, 0, 0, 0 // addr
  };
  int offset = PtrToUlong(hook_address) - PtrToUlong(orig_address + 5);
  memcpy(&patch[1], &offset, sizeof(offset));
  dbg_print("installing hook at %p JMP %x (%p %p)\n", orig_address, offset,
            hook_address, orig_address);
  void* orig_data = overwrite_bytes(orig_address, patch, 6);
  if (orig_data == NULL) {
    return NULL;
  }
  hook_t hook = new_hook(orig_address, 5);
  hook->orig_data = orig_data;
  return hook;
}

/**
 * Install a hook that works by CALL-ing to the new implementation.
 * Handy for hooking existing CALL-sites, overriding essentially just the
 * address.
 */
hook_t install_callhook(void* orig_address, void* hook_address) {
  assert(orig_address != NULL);
  assert(hook_address != NULL);

  char patch[5] = {
      0xE8,      // call
      0, 0, 0, 0 // addr
  };
  int offset = PtrToUlong(hook_address) - PtrToUlong(orig_address + 5);
  memcpy(&patch[1], &offset, sizeof(offset));
  dbg_print("installing hook at %p CALL %x (%p %p)\n", orig_address, offset,
            hook_address, orig_address);
  void* orig_data = overwrite_bytes(orig_address, patch, 5);
  if (orig_data == NULL)
    return NULL;
  hook_t hook = new_hook(orig_address, 5);
  hook->orig_data = orig_data;
  return hook;
}

/**
 * Install a hook that works by overriding a pointer in a vtable.
 * Handy for hooking class methods.
 */
hook_t install_vtblhook(void* orig_address, void* hook_address) {
  assert(orig_address != NULL);
  assert(hook_address != NULL);

  int offset = PtrToUlong(hook_address);
  dbg_print("installing hook at %p VTBL %x (%p %p)\n", orig_address, offset,
            hook_address, orig_address);
  void* orig_data = overwrite_bytes(orig_address, (char*)&offset, 4);
  if (orig_data == NULL)
    return NULL;
  hook_t hook = new_hook(orig_address, 4);
  hook->orig_data = orig_data;
  return hook;
}

hook_t install_bytes(void* orig_address, void* buffer, size_t buffer_size) {
  assert(orig_address != NULL);
  assert(buffer != NULL);
  assert(buffer_size > 0);

  dbg_print("installing hook at %p BYTES (%p %d)\n", orig_address, buffer,
            buffer_size);
  void* orig_data = overwrite_bytes(orig_address, buffer, buffer_size);
  if (orig_data == NULL)
    return NULL;
  hook_t hook = new_hook(orig_address, buffer_size);
  hook->orig_data = orig_data;
  return hook;
}

void revert_hook(hook_t hook) {
  assert(hook != NULL);
  assert(hook->ptr != NULL);
  assert(hook->orig_data != NULL);
  assert(hook->orig_size > 0);

  void* new_data = overwrite_bytes(hook->ptr, hook->orig_data, hook->orig_size);
  if (new_data != NULL)
    free(new_data);
  hook->ptr = NULL;
  hook->orig_data = NULL;
  hook->orig_size = 0;
}
