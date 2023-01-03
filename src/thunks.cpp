#include <fmt/format.h>
#include <iostream>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>

#include <sys/stat.h>
#include <dynarmic/interface/A64/a64.h>
#include "so_module.h"
#include "thunking.hpp"
#include "helpers.h"
#include <cstring>
#include "thunks.hpp"
#include "thread.hpp"

using namespace Dynarmic;
extern void * __libc_start_main;
static const auto thunks = ThunkVector<A64::VAddr, A64::Jit> {
    make_thunk<__ctype_b_loc,"__ctype_b_loc">,
    make_thunk<__ctype_tolower_loc,"__ctype_tolower_loc">, 
    make_thunk<__ctype_toupper_loc,"__ctype_toupper_loc">, 
    make_stub<"__assert_fail">,
    make_stub<"__cxa_finalize">,
    make_thunk< __errno_location,"__errno_location">,
    make_thunk_lambda<"__fprintf_chk">([](A64::Jit *jit) {

        FILE *stream = reinterpret_cast<FILE*>(jit->GetRegister(0));
        const char* format = reinterpret_cast<const char*>(jit->GetRegister(2));
        

        jit->SetRegister(0, fprintf_chk_fake(stream, format, jit));

    }),
    make_stub<"__gmon_start__">,
    make_thunk_lambda<"__isoc99_fscanf">([](A64::Jit *jit) {

        FILE *stream = reinterpret_cast<FILE*>(jit->GetRegister(0));
        const char* format = reinterpret_cast<const char*>(jit->GetRegister(1));
        

        jit->SetRegister(0, fscanf_fake(stream, format, jit));

    }),

    make_thunk_lambda<"__isoc99_sscanf">([](A64::Jit *jit) {

        const char* str = reinterpret_cast<const char*>(jit->GetRegister(0));
        const char* format = reinterpret_cast<const char*>(jit->GetRegister(1));
        

        jit->SetRegister(0, sscanf_fake(str, format, jit));

    }),

    make_thunk_lambda<"__libc_start_main", false>([](A64::Jit *jit) {
        auto regs = jit->GetRegisters();
        std::cout << fmt::format("__libc_start_main({:x},{:x},{:x},{:x},{:x},{:x},{:x})\n", regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6]);
        jit->HaltExecution(HaltReason::ExitAddress);
    }),
    make_stub<"__memcpy_chk">,
    make_thunk_lambda<"__printf_chk">([](A64::Jit *jit) {
        auto format = reinterpret_cast<const char*>(jit->GetRegister(1));
        auto result = printf_fake(format, jit);

        jit->SetRegister(0, result);
    }),
    make_stub_retn<"__stack_chk_fail", 0>,
    make_thunk_lambda<"__vsnprintf_chk">([](A64::Jit *jit) {
      
        char *str = reinterpret_cast<char*>(jit->GetRegister(0));
        size_t max_len = jit->GetRegister(1);
        const char *format = reinterpret_cast<const char*>(jit->GetRegister(4));
        aarch64_va_list *va = reinterpret_cast<aarch64_va_list*>(jit->GetRegister(5));

       
        jit->SetRegister(0, vsnprintf_fake(str, max_len, format, va));

    }),
    make_thunk_lambda<"__vfprintf_chk">([](A64::Jit *jit) {

        FILE* fp = reinterpret_cast<FILE*>(jit->GetRegister(0));
        auto format = reinterpret_cast<const char*>(jit->GetRegister(2));
        auto va = reinterpret_cast<aarch64_va_list*>(jit->GetRegister(3));
        
        auto result = vfprintf_fake(fp, format, va);
        jit->SetRegister(0, result);

    }),
    make_thunk<abort,"abort">,
    make_thunk<calloc,"calloc">,
    make_thunk<close,"close">,
    make_thunk<exit,"exit">,
    make_thunk<fclose,"fclose">,
    make_thunk<feof,"feof">,
    make_thunk<fflush,"fflush">,
    make_thunk<fgetc,"fgetc">,
    make_thunk<fgets,"fgets">,
    make_thunk<fileno,"fileno">,
    make_thunk<fopen,"fopen">,
    make_thunk<fputc,"fputc">,
    make_thunk<fread,"fread">,
    make_thunk<free,"free">,
    make_thunk<fseek,"fseek">,
    make_thunk<ftell,"ftell">,
    make_thunk<fwrite,"fwrite">,
    make_thunk<getenv,"getenv">,
    make_stub<"ioctl">,
    make_thunk<isatty,"isatty">,
    make_thunk<lseek,"lseek">,
    make_thunk<malloc, "malloc">,
    make_thunk<memcmp, "memcmp">, 
    make_thunk<memcpy, "memcpy">, 
    make_thunk<memmove, "memmove">, 
    make_thunk<memset, "memset">, 
    make_thunk<Mix_AllocateChannels,"Mix_AllocateChannels">, 
    make_thunk<Mix_CloseAudio,"Mix_CloseAudio">, 
    make_thunk<Mix_FreeMusic,"Mix_FreeMusic">, 
    make_thunk<Mix_HaltChannel,"Mix_HaltChannel">, 
    make_thunk<Mix_HaltMusic,"Mix_HaltMusic">, 
    //make_thunk_lambda<"Mix_HookMusic">([](A64::Jit *jit) {  
    //    void *fn = jit->GetRegister(0);
    //    void *arg = jit->GetRegister(1);
    //    Mix_HookMusic()
    //}), 
    make_thunk<Mix_HookMusic_fake,"Mix_HookMusic">,
    make_thunk<Mix_LoadMUS,"Mix_LoadMUS">, 
    make_thunk<Mix_OpenAudio,"Mix_OpenAudio">, 
    make_thunk<Mix_PlayChannelTimed,"Mix_PlayChannelTimed">, 
    make_thunk<Mix_Playing,"Mix_Playing">, 
    make_thunk<Mix_PlayingMusic,"Mix_PlayingMusic">, 
    make_thunk<Mix_PlayMusic,"Mix_PlayMusic">, 
    make_thunk<Mix_QuerySpec,"Mix_QuerySpec">, 
    make_thunk<Mix_RegisterEffect,"Mix_RegisterEffect">, 
    make_thunk<Mix_SetMusicCMD,"Mix_SetMusicCMD">, 
    make_thunk<Mix_SetMusicPosition,"Mix_SetMusicPosition">, 
    make_thunk<Mix_SetPanning,"Mix_SetPanning">, 
    make_thunk<Mix_SetPostMix,"Mix_SetPostMix">, 
    make_thunk<Mix_VolumeMusic,"Mix_VolumeMusic">, 
    make_thunk<mkdir,"mkdir">,
    make_stub<"mmap">,
    make_stub<"open">,
    make_stub<"perror">, 
    make_thunk<putc,"putc">,
    make_thunk<putchar,"putchar">,
    make_thunk<putenv,"putenv">,
    make_thunk<puts, "puts">,
    make_stub<"read">,
    make_thunk<realloc,"realloc">,
    make_stub<"remove">,
    make_stub<"rename">,
    make_thunk<SDL_BuildAudioCVT,"SDL_BuildAudioCVT">,
    make_thunk<SDL_CondSignal,"SDL_CondSignal">,
    make_thunk<SDL_CondWait,"SDL_CondWait">,
    make_thunk<SDL_ConvertAudio,"SDL_ConvertAudio">,
    make_thunk<SDL_CreateCond,"SDL_CreateCond">,
    make_thunk<SDL_CreateMutex,"SDL_CreateMutex">,
    make_thunk<SDL_CreateRenderer,"SDL_CreateRenderer">,
    make_thunk<SDL_CreateRGBSurface,"SDL_CreateRGBSurface">,
    make_thunk<SDL_CreateRGBSurfaceFrom,"SDL_CreateRGBSurfaceFrom">,
    make_thunk<SDL_CreateTexture,"SDL_CreateTexture">,
    make_thunk<SDL_CreateTextureFromSurface,"SDL_CreateTextureFromSurface">,
    make_stub<"SDL_CreateThread">,
    make_thunk<SDL_CreateWindow, "SDL_CreateWindow">,
    make_thunk<SDL_Delay, "SDL_Delay">,
    make_thunk<SDL_DestroyCond, "SDL_DestroyCond">,
    make_thunk<SDL_DestroyMutex, "SDL_DestroyMutex">,
    make_thunk<SDL_DestroyRenderer, "SDL_DestroyRenderer">,
    make_thunk<SDL_DestroyTexture, "SDL_DestroyTexture">,
    make_thunk<SDL_FillRect, "SDL_FillRect">,
    make_thunk<SDL_FreeSurface, "SDL_FreeSurface">,
    make_thunk<SDL_GetCurrentDisplayMode, "SDL_GetCurrentDisplayMode">,
    make_thunk<SDL_GetDisplayBounds, "SDL_GetDisplayBounds">,
    make_thunk<SDL_GetError, "SDL_GetError">,
    make_thunk<SDL_GetKeyFromScancode, "SDL_GetKeyFromScancode">,
    make_thunk<SDL_GetKeyName, "SDL_GetKeyName">,
    make_thunk<SDL_GetModState, "SDL_GetModState">,
    make_thunk<SDL_GetMouseState, "SDL_GetMouseState">,
    make_thunk<SDL_GetNumVideoDisplays, "SDL_GetNumVideoDisplays">,
    make_thunk<SDL_GetPrefPath,"SDL_GetPrefPath">,
    make_thunk<SDL_GetRelativeMouseState,"SDL_GetRelativeMouseState">,
    make_thunk<SDL_GetRendererInfo,"SDL_GetRendererInfo">,
    make_thunk<SDL_GetRendererOutputSize,"SDL_GetRendererOutputSize">,
    make_thunk<SDL_GetTicks,"SDL_GetTicks">,
    make_thunk<SDL_GetWindowDisplayIndex,"SDL_GetWindowDisplayIndex">,
    make_thunk<SDL_GetWindowFlags,"SDL_GetWindowFlags">,
    make_thunk<SDL_GetWindowID,"SDL_GetWindowID">,
    make_thunk<SDL_GetWindowPixelFormat,"SDL_GetWindowPixelFormat">,
    make_thunk<SDL_GetWindowSize,"SDL_GetWindowSize">,
    make_stub_retn<"SDL_GL_GetProcAddress", 0>,
    make_thunk<SDL_Init,"SDL_Init">,
    make_thunk<SDL_JoystickClose,"SDL_JoystickClose">,
    make_thunk<SDL_JoystickEventState,"SDL_JoystickEventState">,
    make_thunk<SDL_JoystickGetAxis,"SDL_JoystickGetAxis">,
    make_thunk<SDL_JoystickGetButton,"SDL_JoystickGetButton">,
    make_thunk<SDL_JoystickGetDeviceGUID,"SDL_JoystickGetDeviceGUID">,
    make_thunk<SDL_JoystickGetGUIDFromString,"SDL_JoystickGetGUIDFromString">,
    make_thunk<SDL_JoystickGetHat,"SDL_JoystickGetHat">,
    make_thunk<SDL_JoystickName,"SDL_JoystickName">,
    make_thunk<SDL_JoystickNumAxes,"SDL_JoystickNumAxes">,
    make_thunk<SDL_JoystickNumHats,"SDL_JoystickNumHats">,
    make_thunk<SDL_JoystickOpen,"SDL_JoystickOpen">,
    make_thunk<SDL_LockAudio,"SDL_LockAudio">,
    make_thunk<SDL_LockMutex,"SDL_LockMutex">,
    make_thunk<SDL_LockSurface,"SDL_LockSurface">,
    make_thunk<SDL_LowerBlit,"SDL_LowerBlit">,
    make_thunk<SDL_NumJoysticks,"SDL_NumJoysticks">,
    make_thunk<SDL_PauseAudio,"SDL_PauseAudio">,
    make_thunk<SDL_PeepEvents,"SDL_PeepEvents">,
    make_thunk<SDL_PixelFormatEnumToMasks,"SDL_PixelFormatEnumToMasks">,
    make_thunk<SDL_PollEvent,"SDL_PollEvent">,
    make_thunk<SDL_PumpEvents,"SDL_PumpEvents">,
    make_thunk<SDL_Quit,"SDL_Quit">,
    make_thunk<SDL_QuitSubSystem,"SDL_QuitSubSystem">,
    make_thunk<SDL_RenderClear,"SDL_RenderClear">,
    make_thunk<SDL_RenderCopy,"SDL_RenderCopy">,
    make_thunk<SDL_RenderPresent,"SDL_RenderPresent">,
    make_thunk<SDL_RenderSetIntegerScale,"SDL_RenderSetIntegerScale">,
    make_thunk<SDL_RenderSetLogicalSize,"SDL_RenderSetLogicalSize">,
    make_thunk<SDL_SetHint,"SDL_SetHint">,
    make_thunk<SDL_SetPaletteColors,"SDL_SetPaletteColors">,
    make_thunk<SDL_SetRelativeMouseMode,"SDL_SetRelativeMouseMode">,
    make_thunk<SDL_SetRenderDrawColor,"SDL_SetRenderDrawColor">,
    make_thunk<SDL_SetRenderTarget,"SDL_SetRenderTarget">,
    make_thunk<SDL_SetWindowFullscreen,"SDL_SetWindowFullscreen">,
    make_thunk<SDL_SetWindowIcon,"SDL_SetWindowIcon">,
    make_thunk<SDL_SetWindowMinimumSize,"SDL_SetWindowMinimumSize">,
    make_thunk<SDL_SetWindowSize,"SDL_SetWindowSize">,
    make_thunk<SDL_SetWindowTitle,"SDL_SetWindowTitle">,
    make_thunk<SDL_ShowSimpleMessageBox,"SDL_ShowSimpleMessageBox">,
    make_thunk<SDL_StartTextInput,"SDL_StartTextInput">,
    make_thunk<SDL_StopTextInput,"SDL_StopTextInput">,
    make_thunk<SDL_UnlockAudio,"SDL_UnlockAudio">,
    make_thunk<SDL_UnlockMutex,"SDL_UnlockMutex">,
    make_thunk<SDL_UnlockSurface,"SDL_UnlockSurface">,
    make_thunk<SDL_UpdateTexture,"SDL_UpdateTexture">,
    make_thunk<SDL_WaitEvent,"SDL_WaitEvent">,
    make_thunk<SDL_WaitThread,"SDL_WaitThread">,
    make_thunk<SDL_WarpMouseInWindow,"SDL_WarpMouseInWindow">,
    make_thunk<SDLNet_AllocPacket,"SDLNet_AllocPacket">,
    make_thunk<SDLNet_GetError,"SDLNet_GetError">,
    make_thunk<SDLNet_Init,"SDLNet_Init">,
    make_thunk<SDLNet_ResolveHost,"SDLNet_ResolveHost">,
    make_thunk<SDLNet_UDP_Open,"SDLNet_UDP_Open">,
    make_thunk<SDLNet_UDP_Recv,"SDLNet_UDP_Recv">,
    make_thunk<SDLNet_UDP_Send,"SDLNet_UDP_Send">,    
    make_stub<"src_simple">,
    make_thunk<strcmp, "strcmp">,
    make_thunk<strdup,"strdup">,
    make_thunk<strerror,"strerror">,
    make_thunk<strlen,"strlen">,

    make_thunk_lambda<"strchr">([](A64::Jit *jit) {
        auto s = reinterpret_cast<const char *>(jit->GetRegister(0));
        auto c = static_cast<int>(jit->GetRegister(1));
        
        jit->SetRegister(0, reinterpret_cast<u64>(strchr(s,c)));

    }), 
    make_thunk_lambda<"strrchr">([](A64::Jit *jit) {
        auto s = reinterpret_cast<const char *>(jit->GetRegister(0));
        auto c = static_cast<int>(jit->GetRegister(1));
        
        jit->SetRegister(0, reinterpret_cast<u64>(strrchr(s,c)));

    }),     
    //make_stub<"strstr">, 
    //make_stub<"strcasecmp">, 
    make_thunk_lambda<"strstr">([](A64::Jit *jit) {
        auto haystack = reinterpret_cast<const char *>(jit->GetRegister(0));
        auto needle = reinterpret_cast<const char *>(jit->GetRegister(1));
        
        jit->SetRegister(0, reinterpret_cast<u64>(strstr(haystack,needle)));

    }),     
    make_thunk<strcasecmp, "strcasecmp">, 

    make_thunk<strncasecmp, "strncasecmp">, 
    make_thunk<strncmp, "strncmp">, 
    make_thunk<strncpy, "strncpy">, 
    make_thunk<strtod, "strtod">, 
    make_thunk<strtol, "strtol">, 
    make_thunk<system, "system">,
};
static FastmemEnv env;
static const Dynarmic::A64::UserConfig config {
    .callbacks = &env,
    .fastmem_pointer = nullptr,
    .fastmem_allow_zero_base = true,
    .fastmem_address_space_bits = 64,
    .fastmem_exclusive_access = true,
    .enable_cycle_counting = false,
    .thunk_vector = &thunks
};

const Dynarmic::A64::UserConfig *get_config() {
    return &config;
}
const Dynarmic::ThunkVector<Dynarmic::A64::VAddr, Dynarmic::A64::Jit> *get_thunks() {
    return &thunks;
}