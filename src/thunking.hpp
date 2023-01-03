#include <dynarmic/interface/A64/a64.h>
#include <functional>
#include <tuple>
#include <mcl/type_traits/function_info.hpp>
#include <mcl/mp/typelist/lower_to_tuple.hpp>

template <auto Size>
struct string_literal
{
    constexpr string_literal(const char (&str)[Size])
    {
        std::copy(str, str + Size, value);
    }

    char value[Size];
};

template < string_literal Stub, bool halt = true>
void stub(Dynarmic::A64::Jit *jit) {
    using namespace Dynarmic;

    std::cout << fmt::format("STUB:\tNot implemented {}\n", Stub.value );
    if constexpr ( halt ) {
        jit->HaltExecution(HaltReason::ExitAddress);
    }
}

template < string_literal Stub, int N>
void stub_retn(Dynarmic::A64::Jit *jit) {
    using namespace Dynarmic;

    std::cout << fmt::format("STUB_RET_{}:\t{}\n", N, Stub.value );
    jit->SetRegister(0, N);
}


template <typename R, typename... T>
std::tuple<T...> function_args(R (*)(T...))
{
    return std::tuple<T...>();
}
template <typename R, typename... T>
R function_ret(R (*)(T...))
{
    return R{};
}

template <auto Func>
void thunk(Dynarmic::A64::Jit *jit) {
    using namespace Dynarmic::A64;
    using FnType = decltype(Func);
    
    using R = decltype(function_ret(Func));

    auto args = function_args(Func);
    
    int iregs = 0;

    auto extract_argument = [&]( auto &argument ) {
        using T = std::remove_reference_t<decltype(argument)>;
        if constexpr ( std::is_integral_v<T> || std::is_pointer_v<T> ) {
            argument = (T)(jit->GetRegister(iregs++));
        }
    };

    std::apply([&](auto&... e){ (extract_argument(e), ...);}, args);
    if constexpr ( std::is_void_v<R> ) {
        std::apply(Func, args);
    }
    else {
        R result = std::apply(Func, args);
        if constexpr ( std::is_integral_v<R> || std::is_pointer_v<R> ) {
            jit->SetRegister(0, (u64)(result));
        }

    }

}

template <string_literal FuncName, bool returns = true >
constexpr Dynarmic::Thunk make_stub {
    .address = reinterpret_cast<uint64_t>(&stub<FuncName>),
    .name = FuncName.value,
    .thunk = stub<FuncName>,
    .return_back = returns
};

template <string_literal FuncName, int N >
constexpr Dynarmic::Thunk make_stub_retn {
    .address = reinterpret_cast<uint64_t>(&stub_retn<FuncName, N>),
    .name = FuncName.value,
    .thunk = stub_retn<FuncName, N>,
    .return_back = true
};

template <auto Func, string_literal FuncName, bool returns = true >
constexpr Dynarmic::Thunk make_thunk {
    .address = reinterpret_cast<uint64_t>(&thunk<Func>),
    .name = FuncName.value,
    .thunk = thunk<Func>,
    .return_back = returns
};
template <string_literal FuncName, bool returns = true >
constexpr Dynarmic::Thunk<uint64_t,Dynarmic::A64::Jit>  make_thunk_lambda( auto func ) {
    return  {
        .address = reinterpret_cast<uint64_t>(&stub<FuncName>),
        .name = FuncName.value,
        .thunk = func,
        .return_back = returns
    };
};