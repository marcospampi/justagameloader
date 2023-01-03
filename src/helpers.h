#pragma once
#include <dynarmic/interface/A64/a64.h>
#include <string>
#include "types.hpp"

struct aarch64_va_list;

int vfprintf_fake(FILE *stream, const char *format, aarch64_va_list *ap );
int vsnprintf_fake(char * str, size_t size, const char * format, aarch64_va_list *ap);
int printf_fake(const char *format, Dynarmic::A64::Jit *jit);
int sscanf_fake(const char *str, const char *format, Dynarmic::A64::Jit *jit);
int fscanf_fake(FILE *stream, const char *format, Dynarmic::A64::Jit *jit);

int fprintf_chk_fake(FILE * stream, const char * format, Dynarmic::A64::Jit *jit);

// this gets directly called by the guest
void Mix_HookMusic_fake(u64 fn, u64 arg);