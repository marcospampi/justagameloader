#include "thread.hpp"


std::unique_ptr<A64Thread> create_thread(size_t stack_size) {
    auto stack = std::unique_ptr<u8[]>(new (std::align_val_t(stack_size)) u8[stack_size]);
    auto jit = std::make_unique<Dynarmic::A64::Jit>(*get_config());

    jit->SetSP(reinterpret_cast<uintptr_t>(stack.get()) + stack_size);

    auto thread = std::unique_ptr<A64Thread>(new A64Thread{
        .jit = std::move(jit), .stack = std::move(stack)
    });

    return thread;
}
A64ThreadLeak create_thread_leak(size_t stack_size) {
    auto stack = new (std::align_val_t(stack_size)) u8[stack_size];
    auto jit = new Dynarmic::A64::Jit(*get_config());

    jit->SetSP(reinterpret_cast<uintptr_t>(stack) + stack_size);


    return { jit, stack };
}