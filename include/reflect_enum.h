#ifndef _SCIENUM_H
#define _SCIENUM_H

#include <iostream>
#include <string>


namespace refenum {

namespace details {

    template<class T, T N>
    const char *get_enum_name_static() {
#if defined(_MSC_VER)
        return __FUNCSIG__;
#else
        return __PRETTY_FUNCTION__;
#endif
    }

    template<class T>
    struct get_enum_name_functor {
        int n;
        std::string &s;

        get_enum_name_functor(int n, std::string &s) : n(n), s(s) {}

        /* 通过template函数签名获取类型名称
         *
         * 从C++20开始，可以使用带模板的 lambda 表达式简化 functor 的实现
         * */
        template<int I>
        void call() const {
            if (n == I) {
                s = details::get_enum_name_static<T, (T)I>();
            }
        }
    };

    template<bool Cond>
    struct my_enable_if {};

    template<>
    struct my_enable_if<true> {
        typedef void type;
    };

    // 编译期 if else. 展开了 Beg 到 End 所有 T 枚举类型 I 值对应的 function.
    template<int Beg, int End, class F>
    typename my_enable_if<Beg == End>::type static_for(F const &func) {
    }

    template<int Beg, int End, class F>
    typename my_enable_if<Beg != End>::type static_for(F const &func) {
        func.template call<Beg>();
        static_for<Beg + 1, End>(func);
    }
    /* 从C++17开始，可以使用 if constexpr var == Beg 的方式实现*/
} // namespace details

/* 开放功能函数 */

template<class T, T Beg, T End>
std::string get_enum_name(T n) {
    std::string s;
    details::static_for<Beg, End>(details::get_enum_name_functor<T>(n, s));
    if (s.empty()) {
        return "";
    }

#if defined(_MSC_VER)
    size_t pos = s.find(',');
    ++pos;
    size_t pos2 = s.find_first_of('>', pos);
#else
    size_t pos = s.find("N = ");
    pos += 4;
    size_t pos2 = s.find_first_of(";]", pos);
#endif

    s = s.substr(pos, pos2 - pos);
    size_t pos3 = s.find("::");
    if (pos3 != s.npos) {
        s = s.substr(pos3 + 2);
    }

    return s;
}

template<class T>
std::string get_enum_name(T n) {
    return get_enum_name<T, (T)0, (T)7>(n);
}

/* 暴力搜索 enum name s，返回 enum value
 * */
template<class T, T Beg, T End>
T enum_from_name(std::string const &s) {
    for (int i = (int)Beg; i < (int)End; ++i) {
        if (s == get_enum_name((T)i)) { return (T)i; }
    }

    throw std::runtime_error("Invalid enum value.");
}

template<class T>
T enum_from_name(std::string const &s) {
    return enum_from_name<T, (T)0, (T)7>(s);
}


// /* get type name */
// template <class T>
// std::string get_type_name() {
//     #if defined(_MSC_VER)
//         std::string s = __FUNCSIG__;
//     #else
//         std::string s = __PRETTY_FUNCTION__;
//     #endif

//     auto pos = s.find("T = ");
//     pos += 4;
//     auto pos2 = s.find_first_of(";]", pos);
//     return s.substr(pos, pos2 - pos);
// }
} // namespace refenum

#endif