justagameloader
---------------
That's a heap of hacks I made to have chocolate-doom, aarch64 build, run under Dynarmic by thunking *SDL2* and *standard C* functions.

I got the game to run early this morning, January the 3rd, there are still things that ain't working, sound's one of them.

Overall this project works because the chocolate-doom executable expects to be fed with imported functions ( sdl2, glibc ones ) and its code to just run, and that's the bare minimum I supplied and it's enough kinda of, so we just:

- Load the ELF file
- Resolve all the damn symbols
- Create the custom A64/Dynarmic JIT
- Have the JIT call the main function.

There's a bit more than that, most of the symbols are functions, so they must be thunked somewhat, mainline Dynarmic doesn't support that, I made a fork just for that.

Though my goal is not confined to run chocolate-doom, these're small steps, I want to learn more on HLE and help port games from Android.

Loads of thanks to all [dynarmic](https://github.com/merryhime/dynarmic) contributors for their work!