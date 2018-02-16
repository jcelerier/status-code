/* Proposed SG14 status_code testing
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#include "system_error2.hpp"

#include <cstdio>
#include <cstring>  // for strlen
#include <memory>
#include <string>

#define CHECK(expr)                                                                                                                                                                                                                                                                                                            \
  if(!(expr))                                                                                                                                                                                                                                                                                                                  \
  {                                                                                                                                                                                                                                                                                                                            \
    fprintf(stderr, #expr " failed at line %d\n", __LINE__);                                                                                                                                                                                                                                                                   \
    retcode = 1;                                                                                                                                                                                                                                                                                                               \
  }

// An error coding with multiple success values
enum class Code : size_t
{
  success1,
  nospace,
  success2,
  error2
};
class Code_domain_impl;
using StatusCode = system_error2::status_code<Code_domain_impl>;
// Category for Code
class Code_domain_impl : public system_error2::status_code_domain
{
  using _base = system_error2::status_code_domain;

public:
  using value_type = Code;

  // Custom string_ref using a shared_ptr
  class string_ref : public _base::string_ref
  {
  public:
    using shared_ptr_type = std::shared_ptr<std::string>;

  protected:
    virtual void _copy(_base::string_ref *dest) const & override final { new(static_cast<string_ref *>(dest)) string_ref(_begin, _end, _state); }
    virtual void _move(_base::string_ref *dest) && noexcept override final { new(static_cast<string_ref *>(dest)) string_ref(_begin, _end, _state); }
  public:
    string_ref() { new(reinterpret_cast<shared_ptr_type *>(this->_state)) shared_ptr_type(); }
    // Allow explicit cast up
    explicit string_ref(_base::string_ref v) { static_cast<string_ref &&>(v)._move(this); }
    // Construct from a C string literal, holding ref counted copy of string
    explicit string_ref(const char *str)
    {
      static_assert(sizeof(shared_ptr_type) <= sizeof(this->_state), "A shared_ptr does not fit into status_code's state");
      auto len = strlen(str);
      auto p = std::make_shared<std::string>(str, len);
      new(reinterpret_cast<shared_ptr_type *>(this->_state)) shared_ptr_type(p);
      this->_begin = p->data();
      this->_end = p->data() + p->size();
    }
    // Construct from a set of data
    string_ref(pointer begin, pointer end, void *const state[])
        : _base::string_ref(begin, end, nullptr, nullptr)
    {
      // Increase the ref count
      new(reinterpret_cast<shared_ptr_type *>(_state)) shared_ptr_type(*reinterpret_cast<const shared_ptr_type *>(state));
    }
    virtual ~string_ref()
    {
      // Decrease the ref count
      auto *p = reinterpret_cast<shared_ptr_type *>(_state);
      p->~shared_ptr_type();
    }
  };
  constexpr Code_domain_impl()
      : _base(0x430f120194fc06c7)
  {
  }
  static inline constexpr const Code_domain_impl *get();
  virtual _base::string_ref name() const noexcept override final
  {
    static string_ref v("Code_category_impl");
    return v;
  }
  virtual bool _failure(const system_error2::status_code<void> &code) const noexcept override final
  {
    assert(code.domain() == *this);
    return (static_cast<size_t>(static_cast<const StatusCode &>(code).value()) & 1) != 0;
  }
  virtual bool _equivalent(const system_error2::status_code<void> &code1, const system_error2::status_code<void> &code2) const noexcept override final
  {
    assert(code1.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code1);
    if(code2.domain() == *this)
    {
      const auto &c2 = static_cast<const StatusCode &>(code2);
      return c1.value() == c2.value();
    }
    // If the other category is generic
    if(code2.domain() == system_error2::generic_code_domain)
    {
      const auto &c2 = static_cast<const system_error2::generic_code &>(code2);
      switch(c1.value())
      {
      case Code::success1:
      case Code::success2:
        return static_cast<system_error2::errc>(c2.value()) == system_error2::errc::success;
      case Code::nospace:
        switch(static_cast<system_error2::errc>(c2.value()))
        {
        case system_error2::errc::filename_too_long:
        case system_error2::errc::no_buffer_space:
        case system_error2::errc::no_space_on_device:
        case system_error2::errc::not_enough_memory:
        case system_error2::errc::too_many_files_open_in_system:
        case system_error2::errc::too_many_files_open:
        case system_error2::errc::too_many_links:
          return true;
        default:
          return false;
        }
      }
    }
    return false;
  }
  virtual system_error2::generic_code _generic_code(const system_error2::status_code<void> &code) const noexcept override final
  {
    assert(code.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code);
    switch(c1.value())
    {
    case Code::success1:
    case Code::success2:
      return system_error2::generic_code(system_error2::errc::success);
    case Code::nospace:
      return system_error2::generic_code(system_error2::errc::no_buffer_space);
      // error2 gets no mapping to generic_code
    }
    return {};
  }
  virtual _base::string_ref _message(const system_error2::status_code<void> &code) const noexcept override final
  {
    assert(code.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code);
    switch(c1.value())
    {
    case Code::success1:
    {
      static string_ref v("success1");
      return v;
    }
    case Code::nospace:
    {
      static string_ref v("nospace");
      return v;
    }
    case Code::success2:
    {
      static string_ref v("success2");
      return v;
    }
    case Code::error2:
    {
      static string_ref v("error2");
      return v;
    }
    }
    return string_ref{};
  }
  virtual void _throw_exception(const system_error2::status_code<void> &code) const override final
  {
    assert(code.domain() == *this);
    const auto &c = static_cast<const StatusCode &>(code);
    throw system_error2::status_error<Code_domain_impl>(c);
  }
};
constexpr Code_domain_impl Code_domain;
inline constexpr const Code_domain_impl *Code_domain_impl::get()
{
  return &Code_domain;
}


int main()
{
  using namespace system_error2;
  int retcode = 0;

  constexpr generic_code empty1, success1(errc::success), failure1(errc::filename_too_long);
  CHECK(empty1.empty());
  CHECK(!success1.empty());
  CHECK(!failure1.empty());
  printf("generic_code empty has value %d (%s) is success %d is failure %d\n", empty1.value(), empty1.message().c_str(), empty1.success(), empty1.failure());
  printf("generic_code success has value %d (%s) is success %d is failure %d\n", success1.value(), success1.message().c_str(), success1.success(), success1.failure());
  printf("generic_code failure has value %d (%s) is success %d is failure %d\n", failure1.value(), failure1.message().c_str(), failure1.success(), failure1.failure());

  constexpr StatusCode empty2, success2(Code::success1), failure2(Code::nospace);
  printf("\nStatusCode empty has value %zu (%s) is success %d is failure %d\n", empty2.value(), empty2.message().c_str(), empty2.success(), empty2.failure());
  printf("StatusCode success has value %zu (%s) is success %d is failure %d\n", success2.value(), success2.message().c_str(), success2.success(), success2.failure());
  printf("StatusCode failure has value %zu (%s) is success %d is failure %d\n", failure2.value(), failure2.message().c_str(), failure2.success(), failure2.failure());

  printf("\n(empty1 == empty2) = %d\n", empty1 == empty2);        // True, empty ec's always compare equal no matter the type
  printf("(success1 == success2) = %d\n", success1 == success2);  // True, success maps onto success
  printf("(success1 == failure2) = %d\n", success1 == failure2);  // False, success does not map onto failure
  printf("(failure1 == success2) = %d\n", failure1 == success2);  // False, failure does not map onto success
  printf("(failure1 == failure2) = %d\n", failure1 == failure2);  // True, filename_too_long maps onto nospace

  status_code<erased<int>> success3(success1), failure3(failure1);
  printf("\nerased<int> success has value %d (%s) is success %d is failure %d\n", success3.value(), success3.message().c_str(), success3.success(), success3.failure());
  printf("erased<int> failure has value %d (%s) is success %d is failure %d\n", failure3.value(), failure3.message().c_str(), failure3.success(), failure3.failure());
  return retcode;
}