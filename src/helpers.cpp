#include <sstream>
#include <cstring>
#include <iostream>
#include <vector>
#include <functional>
#include <mutex>
#include <fmt/format.h>
#include "thread.hpp"
#include "helpers.h"

using VAddr = u64;
using namespace Dynarmic;

struct fake_valist {
   unsigned int gp_offset;
   unsigned int fp_offset;
   void *overflow_arg_area;
   void *reg_save_area;
};

struct aarch64_va_list {
    u64 __stack;
    u64 __gr_top;
    u64 __vr_top;
    int __gr_offs;
    int __vr_offs;
};

class VAListWrapper {
public:
    VAListWrapper() {}

    static VAListWrapper from_valist(const char *fmt, aarch64_va_list *valist) {
        VAListWrapper self {};
        auto &stack = self.emulated_stack;

        std::vector<Tag> tags = make_tags(fmt);

        auto 
            gr_top = reinterpret_cast<u64*>(valist->__gr_top), 
            gr_bot = reinterpret_cast<u64*>(valist->__gr_top + valist->__gr_offs);
        auto 
            vr_top = reinterpret_cast<u64*>(valist->__vr_top), 
            vr_bot = reinterpret_cast<u64*>(valist->__vr_top + valist->__vr_offs);
        auto
            stack_bot = reinterpret_cast<u64*>(valist->__stack);

        for ( auto tag: tags ) {
            switch( tag ) {
                case Tag::I: {
                    if ( gr_top != gr_bot ) {
                        stack.push_back(*(gr_bot++));
                    }
                    else {
                        throw std::runtime_error("Unhandled case: pop from stack");
                    }
                    break;
                }
                case Tag::V: {
                        throw std::runtime_error("Unhandled case: pop from vector registers");

                    break;
                }
            }
        }

        return std::move(self);
    }
    static VAListWrapper from_jit( const char *fmt,  A64::Jit *jit, int ngr, int nvr ) {
        VAListWrapper self {};
        auto &stack = self.emulated_stack;
        std::vector<Tag> tags = make_tags(fmt);

        int igr = ( 8 - ngr );
        int ivr = ( 8 - nvr );

        for ( auto tag: tags ) {
            switch( tag ) {
                case Tag::I: {
                    if ( igr < 8 ) {
                        stack.push_back(jit->GetRegister(igr));
                        igr++;
                    }
                    break;
                }
                case Tag::V: {
                    if ( ivr < 8 ) {
                        auto vector = jit->GetVector(igr);
                        stack.push_back(vector[0]);
                        stack.push_back(vector[1]);
                        igr++;
                    }
                    //throw std::runtime_error("Unhandled case: pop from vector registers");
                    break;
                }
            }
        }
        return std::move(self);
    }

    template < class R >
    R call(std::function<R(fake_valist*)> fn) {
        fake_valist fake {
            .gp_offset = 48,    // exhausted
            .fp_offset = 304,   // exhausted
            .overflow_arg_area = reinterpret_cast<void*>(this->emulated_stack.data()),
            .reg_save_area = nullptr
        };

        return fn(&fake);
    }
    

private:
    enum class Tag {
        I,
        V
    };

    static std::vector<Tag> make_tags(const char *fmt) {
        std::vector<Tag> tags;
        for (auto it = fmt; *it!= 0; it++) {
            if ( *it == '%' ) {
                it++;
                if ( *it == '%') continue;
                bool should_stop = false;
                while ( !should_stop ) {
                    switch (*it) {
                        case 'o':
                        case 'c':
                        case 'd':
                        case 'i':
                        case 'u':
                        case 'X':
                        case 'x':
                        case 'p':
                        case 's':
                            should_stop = true;
                            tags.push_back(Tag::I);
                            break;
                        case 'f':
                        case 'e':
                        case 'E':
                        case 'A':
                        case 'a':
                            should_stop = true;
                            tags.push_back(Tag::V);
                            break;
                        default:
                            it++;
                    }
                }

            }
            
        }
        return tags;

    }
    std::vector<u64> emulated_stack;

};

int vfprintf_fake(FILE *stream, const char *format, aarch64_va_list *ap ) {
    auto wrapper = VAListWrapper::from_valist(format, ap);

    return wrapper.call<int>([&](fake_valist *va) -> int {
        auto fakefn = reinterpret_cast<int(*)(FILE*,const char*, fake_valist*)>(vfprintf);
        return fakefn(stream, format, va);
    });
}

int printf_fake(const char *format, Dynarmic::A64::Jit *jit) {
    auto wrapper = VAListWrapper::from_jit(format, jit, 6, 8);

    return wrapper.call<int>([&](fake_valist *va) -> int {
        auto fakefn = reinterpret_cast<int(*)(const char*, fake_valist*)>(vprintf);
        return fakefn(format, va);
    });
}

int vsnprintf_fake(char * str, size_t size, const char * format, aarch64_va_list *ap) {
    auto wrapper = VAListWrapper::from_valist(format, ap);
    return wrapper.call<int>([&](fake_valist* va) {
        auto fakefn = reinterpret_cast<int(*)(char * str, size_t size, const char * format, fake_valist*)>(vsnprintf);
        return fakefn(str, size,format, va);
    });
}

int sscanf_fake(const char *str, const char *format, Dynarmic::A64::Jit *jit) {
    auto wrapper = VAListWrapper::from_jit(format, jit, 6, 8);

    return wrapper.call<int>([&](fake_valist *va) -> int {
        auto fakefn = reinterpret_cast<int(*)(const char*,const char*, fake_valist*)>(vsscanf);
        return fakefn(str, format, va);
    });
}

int fscanf_fake(FILE *stream, const char *format, Dynarmic::A64::Jit *jit) {
    auto wrapper = VAListWrapper::from_jit(format, jit, 6, 8);

    return wrapper.call<int>([&](fake_valist *va) -> int {
        const void *ptr = reinterpret_cast<void*>(vfscanf);
        auto fakefn = reinterpret_cast<int(*)(FILE *,const char*, fake_valist*)>(ptr);
        return fakefn(stream, format, va);
    });
}
int fprintf_chk_fake(FILE * stream, const char * format, Dynarmic::A64::Jit *jit) {
    auto wrapper = VAListWrapper::from_jit(format, jit, 5, 8);
    return wrapper.call<int>([&](fake_valist *va) -> int {
        auto fakefn = reinterpret_cast<int(*)(FILE *,const char*, fake_valist*)>(vfprintf);
        return fakefn(stream, format, va);
    });
}

#include <SDL2/SDL_mixer.h>

struct MixHookMusicData {
    u64 fn;
    u64 arg;
    A64ThreadLeak thread;
};

void Mix_HookMusic_fake_cb(MixHookMusicData *udata, Uint8 *stream, int len) {

    
    fmt::print("Oh crap, forwarding Mix_HookMusic callback!\n");

    udata->thread.jit->VMCall<void>(udata->fn, udata->arg, (u64)stream, len );

    fmt::print("Mix_HookMusic callback... forwarded!\n");
    
}

void Mix_HookMusic_fake(u64 fn, u64 arg) {
  
    // leak leak leak
    auto data = new MixHookMusicData{ .fn = fn, .arg = arg, .thread = create_thread_leak(4096*16)};

    auto fakefn = reinterpret_cast<void (*)(void *udata, Uint8 *stream, int len)>(Mix_HookMusic_fake_cb);
    Mix_HookMusic(fakefn, data);
}