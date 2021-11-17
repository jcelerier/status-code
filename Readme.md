# Reference implementation for proposed SG14 `status_code` (`<system_error2>`) in C++ 11

(C) 2018 - 2021 Niall Douglas [http://www.nedproductions.biz/](http://www.nedproductions.biz/)
Please send feedback to the SG14 study group mailing list at [https://lists.isocpp.org/mailman/listinfo.cgi/sg14/](https://lists.isocpp.org/mailman/listinfo.cgi/sg14/).

Docs: [https://ned14.github.io/status-code/](https://ned14.github.io/status-code/)
(reference API docs are at bottom of page) Linux: [![Build Status](https://github.com/ned14/status-code/workflows/Unit%20tests%20Linux/badge.svg?branch=master)](https://github.com/ned14/status-code/actions) Windows: [![Build status](https://github.com/ned14/status-code/workflows/Unit%20tests%20Windows/badge.svg?branch=master)](https://github.com/ned14/status-code/actions)

Solves the problems for low latency/large code base users with `<system_error>`
as listed by [WG21 P0824](https://wg21.link/P0824). This proposed `<system_error2>`
library is EXPERIMENTAL and is subject to change as the committee evolves the design.
The proposal paper for this library is [WG21 P1028](https://wg21.link/P1028).
To fetch a drop-in standalone single file implementation:

```
wget https://github.com/ned14/status-code/raw/master/single-header/system_error2.hpp
```

If you'd like a 'ready to go' `T`-or-`E` variant return solution where your functions
just return a `result<T>` which can transport either a success or a failure, consider using
https://ned14.github.io/outcome/experimental/ which bundles a copy of this library
inside standalone Outcome and Boost.Outcome. You can find an example of use at
https://github.com/ned14/status-code/tree/master/example/variant_return.cpp. Outcome
works great with C++ exceptions globally disabled, and includes only a very minimal
set of C++ headers.

## Features:

- Portable to any C++ 11 compiler. These are known to work:
    - &gt;= GCC 5 (due to requiring libstdc++ 5 for sufficient C++ 11 type traits)
    - &gt;= clang 3.3 with a new enough libstdc++ (previous clangs don't implement inheriting constructors)
    - &gt;= Visual Studio 2015 (previous MSVC's don't implement inheriting constructors)
- Comes with built in POSIX, Win32, NT kernel, Microsoft COM, `getaddrinfo()` and `std::error_code`
status code domains.
- Implements `std::error` as proposed by [P0709 Zero-overhead deterministic exceptions](https://wg21.link/P0709).
- Aims to cause zero code generated by the compiler most of the time.
- Never calls `malloc()`.
- Header-only library friendly.
- Type safe yet with type erasure in public interfaces so it can scale
across huge codebases.
- Minimum compile time load, making it suitable for use in the global headers of
multi-million line codebases.
- Has a 'not POSIX' configuration `SYSTEM_ERROR2_NOT_POSIX`, suitable for using this
library on non-POSIX non-Windows platforms.

## Example of use:

<table width="100%">
<tr>
<th>POSIX</th>
<th>Windows</th>
</tr>
<tr>
<td valign="top">
<pre><code class="c++">using native_handle_type = int;
native_handle_type open_file(const char *path,
  system_error2::system_code &sc) noexcept
{
  sc.clear();  // clears to empty
  native_handle_type h = ::open(path, O_RDONLY);
  if(-1 == h)
  {
    // posix_code type erases into system_code
    sc = system_error2::posix_code(errno);
  }
  return h;
}
</code></pre>
</td>
<td valign="top">
<pre><code class="c++">using native_handle_type = HANDLE;
native_handle_type open_file(const wchar_t *path,
  system_error2::system_code &sc) noexcept
{
  sc.clear();  // clears to empty
  native_handle_type h = CreateFile(path, GENERIC_READ,
    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  );
  if(INVALID_HANDLE_VALUE == h)
  {
    // win32_code type erases into system_code
    sc = system_error2::win32_code(GetLastError());
  }
  return h;
}
</code></pre>
</td>
</tr>
<tr>
<th colspan="2">Portable code</th>
</tr>
<tr>
<td colspan="2">
<pre><code class="c++" style="display: inline-block; position: relative; left: 50%; transform: translateX(-50%);">system_error2::system_code sc;  // default constructs to empty
native_handle_type h = open_file(path, sc);
// Is the code a failure?
if(sc.failure())
{
  // Do semantic comparison to test if this was a file not found failure
  // This will match any system-specific error codes meaning a file not found
  if(sc != system_error2::errc::no_such_file_or_directory)
  {
    std::cerr << "FATAL: " << sc.message().c_str() << std::endl;
    std::terminate();
  }
}
</code></pre>
</tr>
</table>

### Quick synthesis of a custom status code domain for any arbitrary enumeration type:

Defining a custom status code domain requires writing a lot of tedious boilerplate. End user
Jesse Towner suggested a simplified declarative API so arbitrary enumeration types can
be wrapped into a custom status code domain with a minimum of effort.

Note that this support requires a minimum of C++ 14 in the compiler. It is not defined if
in C++ 11.

<pre><code class="c++">// This is some third party enumeration type in another namespace
namespace another_namespace
{
  // "Initialiser list" custom status code domain
  enum class AnotherCode : size_t
  {
    success1,
    goaway,
    success2,
    error2
  };
}  // namespace another_namespace

// To synthesise a custom status code domain for `AnotherCode`, inject the following
// template specialisation:
SYSTEM_ERROR2_NAMESPACE_BEGIN
template &lt;&gt;
struct quick_status_code_from_enum&lt;another_namespace::AnotherCode&gt;
  : quick_status_code_from_enum_defaults&lt;another_namespace::AnotherCode&gt;
{
  // Text name of the enum
  static constexpr const auto domain_name = "Another Code";

  // Unique UUID for the enum. PLEASE use https://www.random.org/cgi-bin/randbyte?nbytes=16&format=h
  static constexpr const auto domain_uuid = "{be201f65-3962-dd0e-1266-a72e63776a42}";

  // Map of each enum value to its text string, and list of semantically equivalent errc's
  static const std::initializer_list&lt;mapping&lg; &value_mappings()
  {
    static const std::initializer_list&lt;mapping&gt; v = {
    // Format is: { enum value, "string representation", { list of errc mappings ... } }
    {another_namespace::AnotherCode::success1, "Success 1", {errc::success}},             //
    {another_namespace::AnotherCode::goaway, "Go away", {errc::permission_denied}},       //
    {another_namespace::AnotherCode::success2, "Success 2", {errc::success}},             //
    {another_namespace::AnotherCode::error2, "Error 2", {errc::function_not_supported}},  //
    };
    return v;
  }

  // Completely optional definition of mixin for the status code synthesised from `Enum`.
  // It can be omitted.
  template &lt;class Base&gt; struct mixin : Base
  {
    using Base::Base;
    
    // A custom method on the synthesised status code
    constexpr int custom_method() const { return 42; }
  };
};
SYSTEM_ERROR2_NAMESPACE_END

// If you wish easy manufacture of status codes from AnotherCode:
namespace another_namespace
{
  // ADL discovered, must be in same namespace as AnotherCode
  constexpr inline
  SYSTEM_ERROR2_NAMESPACE::quick_status_code_from_enum_code&lt;another_namespace::AnotherCode&gt;
  status_code(AnotherCode c) { return c; }
}  // namespace another_namespace


// Make a status code of the synthesised code domain for `AnotherCode`
SYSTEM_ERROR2_CONSTEXPR14 auto v = status_code(another_namespace::AnotherCode::error2);
assert(v.value() == another_namespace::AnotherCode::error2);
assert(v.custom_method() == 42);

// If you don't need custom methods, just use system_code, all erased
// status codes recognise quick_status_code_from_enum&lt;Enum&gt;
SYSTEM_ERROR2_NAMESPACE::system_code v2(another_namespace::AnotherCode::error2);
</code></pre>


## Problems with `<system_error>` solved:

1. Does not cause `#include <string>`, and thus including the entire STL allocator and algorithm
machinery, thus preventing use in freestanding C++ as well as substantially
impacting compile times which can be a showstopper for very large C++ projects.
Only includes the following headers:
    - `<atomic>` to reference count localised strings retrieved from the OS.
    - `<cassert>` to trap when misuse occurs.
    - `<cerrno>` for the generic POSIX error codes (`errno`) which is required to define `errc`.
    - `<cstddef>` for the definition of `size_t` and other types.
    - `<cstring>` for the system call to fetch a localised string and C string functions.
    - `<exception>` for the basic `std::exception` type so we can optionally throw STL exceptions.
    - `<initializer_list>` so we can permit in-place construction.
    - `<new>` so we can perform placement new.
    - `<type_traits>` as we need to do some very limited metaprogramming.
    - `<utility>` if on C++ 17 or later for `std::in_place`.
    
	All of the above headers are on the "fast parse" list at https://github.com/ned14/stl-header-heft.

    These may look like a lot, but in fact just including `<atomic>` on libstdc++ actually
brings in most of the others in any case, and a total of 200Kb (8,000 lines) of text is including by
`system_error2.hpp` on libstdc++ 7. Compiling a file including `status_code.hpp` takes
less than 150 ms with clang 3.3 as according to the `-ftime-report` diagnostic (a completely
empty file takes 5 ms).

2. Unlike `std::error_code` which was designed before `constexpr`, this proposed
implementation has all-`constexpr` construction and destruction with as many operations
as possible being trivial or literal, with only those exact minimum operations which
require runtime code generation being non-trivial (note: requires C++ 14 for a complete
implementation of this).

3. This in turn means that we solve a long standing problem with `std::error_category`
in that it is not possible to define a safe custom C++ 11 error category in a header
only library where semantic comparisons would randomly break depending on the direction of wind
blowing when the linker ran. This proposed design is 100% safe to use in header only libraries.

4. `std::error_code`'s boolean conversion operator i.e. `if(ec) ...` has become
unfortunately ambiguous in real world C++ out there. Its correct meaning is
"if `ec` has a non-zero value". Unfortunately, much code out in the wild uses
it as if "if `ec` is errored". This is incorrect, though safe most of the time
where `ec`'s category is well known i.e. non-zero values are always an error.
For unknown categories supplied by third party code however, it is dangerous and leads
to unpleasant, hard-to-debug, surprise.

    The `status_code` proposed here suffers from no such ambiguity. It can be one of
exactly three meanings: (i) success (ii) failure (iii) empty (uninitialised). There
is no boolean conversion operator, so users must write out exactly what they mean
e.g. `if(sc.success()) ...`, `if(sc.failure()) ...`, `if(sc.empty()) ...`.

5. Relatedly, `status_code` can now represent successful (informational) codes as
well as failure codes. Unlike `std::error_code` where zero is given special meaning,
we impose no requirements at all on the choice of coding. This permits safe usage of more
complex C status coding such as the NT kernel's `NTSTATUS`, which is a `LONG` whereby bits
31 and 30 determine which of four categories the status is (success, informational, warning,
error), or the very commone case where negative numbers mean failure and positive numbers
mean success-with-information.

6. The relationship between `std::error_code` and `std::error_condition` is
confusing to many users reading code based on `<system_error>`, specifically when is
a comparison between codes *semantic* or *literal*? `status_code` makes all
comparisons *semantic*, **always**. If you want a literal comparison, you can do one
by hand by comparing domains and values directly.

7. `std::error_code` enforced its value to always be an `int`. This is problematic
for coding systems which might use a `long` and implement coding namespaces within
the extended number of bits, or for end users wishing to combine a code with a `void *`
in order to transmit payload or additional context. As a result, `status_code` is
templated to its domain, and the domain sets its type. A type erased edition of
`status_code<D>` is available as `status_code<void>`, this is for obvious reasons
non-copyable, non-movable and non-destructible.

    A more useful type erased edition is `status_code<erased<T>>` 
which is available if `D::value_type` is trivially copyable, `T` is an integral
type, and `sizeof(T) >= sizeof(D::value_type)`. This lets you use
`status_code<erased<T>>` in all your public interfaces without
restrictions. As a pointer to the original category is retained, and trivially
copyable types may be legally copied by `memcpy()`, type erased status codes
work exactly as normal, except that publicly it does not advertise its type.

8. `std::system_category` assumes that there is only one "system" error coding,
something mostly true on POSIX, but not elsewhere. This library defines
`system_code` to a type erased status code sufficiently large enough to carry
any of the system error codings on the current platform. This allows code to
construct the precise error code for the system failure in question, and
return it type erased from the function. Depending on the system call which
failed, a function may therefore return any one of many system code domains.

9. Too much `<system_error>` code written for POSIX uses `std::generic_category`
when they really meant `std::system_category` because the two are interchangeable
on POSIX. Further confusion stems from `std::error_condition` also sharing the same
coding and type. This causes portability problems. This library's `generic_code` has
a value type of `errc` which is a strong enum. This prevents implicit confusion
with `posix_code`, whose value type is an `int` same as `errno` returns. There is
no distinction between codes and conditions in this library, rather we treat
`generic_code` as something special, because it represents `errc`. The cleanup
of these ambiguities in `<system_error>` should result in users writing clearer
code with fewer unintended portability problems.
