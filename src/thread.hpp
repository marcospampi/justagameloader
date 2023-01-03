#pragma once
#include <memory>
#include <iostream>
#include <fmt/format.h>
#include <dynarmic/interface/A64/a64.h>
#include "types.hpp"
#include "thunks.hpp"

using VAddr = Dynarmic::A64::VAddr;
struct FastmemEnv: Dynarmic::A64::UserCallbacks {
    ~FastmemEnv() = default;

    // All reads through this callback are 4-byte aligned.
    // Memory must be interpreted as little endian.
    std::optional<std::uint32_t> MemoryReadCode(VAddr vaddr) { return *reinterpret_cast<std::uint32_t*>(vaddr); }

    template <class T>
    T read(VAddr vaddr) {
        std::cerr << fmt::format("Invalid memory access at {}", fmt::ptr(reinterpret_cast<uintptr_t*>(vaddr)))  << std::endl;
        exit(1);
    }

    template <class T>
    void write(VAddr vaddr, T value) {
        std::cerr << fmt::format("Invalid memory access at {}", fmt::ptr(reinterpret_cast<uintptr_t*>(vaddr)))  << std::endl;
        exit(1);
    }


    // Reads through these callbacks may not be aligned.
    std::uint8_t MemoryRead8(VAddr vaddr) {
        return read<uint8_t>(vaddr);
    };
    std::uint16_t MemoryRead16(VAddr vaddr) {
        return read<uint16_t>(vaddr);
    };
    std::uint32_t MemoryRead32(VAddr vaddr) {
        return read<uint32_t>(vaddr);
    };
    std::uint64_t MemoryRead64(VAddr vaddr) {
        return read<uint64_t>(vaddr);
    };
    Dynarmic::A64::Vector MemoryRead128(VAddr vaddr) {
        return read<Dynarmic::A64::Vector>(vaddr);
    };

    // Writes through these callbacks may not be aligned.
    void MemoryWrite8(VAddr vaddr, std::uint8_t value) {
        return write<uint8_t>( vaddr, value );
    };
    void MemoryWrite16(VAddr vaddr, std::uint16_t value) {
        return write<uint16_t>( vaddr, value );
    };
    void MemoryWrite32(VAddr vaddr, std::uint32_t value) {
        return write<uint32_t>( vaddr, value );
    };
    void MemoryWrite64(VAddr vaddr, std::uint64_t value) {
        return write<uint64_t>( vaddr, value );
    };
    void MemoryWrite128(VAddr vaddr, Dynarmic::A64::Vector value) {
        return write<Dynarmic::A64::Vector>( vaddr, value );
    }

    // Writes through these callbacks may not be aligned.
    bool MemoryWriteExclusive8(VAddr /*vaddr*/, std::uint8_t /*value*/, std::uint8_t /*expected*/) { return false; }
    bool MemoryWriteExclusive16(VAddr /*vaddr*/, std::uint16_t /*value*/, std::uint16_t /*expected*/) { return false; }
    bool MemoryWriteExclusive32(VAddr /*vaddr*/, std::uint32_t /*value*/, std::uint32_t /*expected*/) { return false; }
    bool MemoryWriteExclusive64(VAddr /*vaddr*/, std::uint64_t /*value*/, std::uint64_t /*expected*/) { return false; }
    bool MemoryWriteExclusive128(VAddr /*vaddr*/, Dynarmic::A64::Vector /*value*/,Dynarmic::A64::Vector /*expected*/) { return false; }

    // If this callback returns true, the JIT will assume MemoryRead* callbacks will always
    // return the same value at any point in time for this vaddr. The JIT may use this information
    // in optimizations.
    // A conservative implementation that always returns false is safe.
    bool IsReadOnlyMemory(VAddr /*vaddr*/) { return false; }

    /// The interpreter must execute exactly num_instructions starting from PC.
    void InterpreterFallback(VAddr pc, size_t num_instructions) {
        fmt::print("Called ::InterpreterFallback(0x{:x}, {})\n", pc, num_instructions);
        for ( size_t i = 0 ;  i < num_instructions; ++i ) {
            auto address = pc + i * 4;
            fmt::print("\t0x{:x}: {:x}\n", address, *(u32*)(address) );
        } 
        exit(1);
    };

    // This callback is called whenever a SVC instruction is executed.
    void CallSVC(std::uint32_t swi) {
        fmt::print("Called ::CallSVC({})", swi);
        exit(1);

    };

    void ExceptionRaised(VAddr pc, Dynarmic::A64::Exception exception) { };
    void DataCacheOperationRaised(Dynarmic::A64::DataCacheOperation /*op*/, VAddr /*value*/) {}
    void InstructionCacheOperationRaised(Dynarmic::A64::InstructionCacheOperation /*op*/, VAddr /*value*/) {}
    void InstructionSynchronizationBarrierRaised() {}

    // Timing-related callbacks
    // ticks ticks have passed
    void AddTicks(std::uint64_t ticks) { };
    // How many more ticks am I allowed to execute?
    std::uint64_t GetTicksRemaining() { return 0; }
    // Get value in the emulated counter-timer physical count register.
    std::uint64_t GetCNTPCT() { return 0; }

};

struct A64Thread {
    std::unique_ptr<Dynarmic::A64::Jit> jit;
    std::unique_ptr<u8[]> stack;
};
struct A64ThreadLeak {
    Dynarmic::A64::Jit *jit;
    u8* stack;
};

std::unique_ptr<A64Thread> create_thread(size_t stack_size = 4096);
A64ThreadLeak create_thread_leak(size_t stack_size = 4096);

