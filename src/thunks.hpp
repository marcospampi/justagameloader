#pragma once
#include <dynarmic/interface/A64/a64.h>

const Dynarmic::A64::UserConfig *get_config();
const Dynarmic::ThunkVector<Dynarmic::A64::VAddr, Dynarmic::A64::Jit> *get_thunks();