// -*- c-basic-offset: 4 -*-

// taken from the GNU/sgi stl extensions

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


#ifndef _STL_UTILS_H
#define _STL_UTILS_H


#include <functional>
#include <utility>

template<class _Pair>
struct select1st : public std::unary_function<_Pair,
  typename _Pair::first_type> {
  typename _Pair::first_type& operator()(_Pair& __x) const {
    return __x.first;
  }
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};

/// An \link SGIextensions SGI extension \endlink.
template <class _Operation1, class _Operation2>
class unary_compose
  : public std::unary_function<typename _Operation2::argument_type,
                               typename _Operation1::result_type>
{
protected:
  _Operation1 _M_fn1;
  _Operation2 _M_fn2;
public:
  unary_compose(const _Operation1& __x, const _Operation2& __y)
    : _M_fn1(__x), _M_fn2(__y) {}
  typename _Operation1::result_type
  operator()(const typename _Operation2::argument_type& __x) const {
    return _M_fn1(_M_fn2(__x));
  }
};

/// An \link SGIextensions SGI extension \endlink.
template <class _Operation1, class _Operation2>
inline unary_compose<_Operation1,_Operation2>
compose1(const _Operation1& __fn1, const _Operation2& __fn2)
{
  return unary_compose<_Operation1,_Operation2>(__fn1, __fn2);
}


//
template<typename _Container>
//inline bool set_contains(const _Container & c, const _Container::key_type & key)
inline bool set_contains(const _Container & c, const typename _Container::key_type & key)
{
    return c.find(key) != c.end();
}



#if 0
/// triple holds three objects of arbitrary type.
template <class _T1, class _T2, class _T3>
struct triple {
  typedef _T1 first_type;    ///<  @c first_type is the first bound type
  typedef _T2 second_type;   ///<  @c second_type is the second bound type
  typedef _T3 third_type;   ///<  @c third_type is the third bound type

  _T1 first;                 ///< @c first is a copy of the first object
  _T2 second;                ///< @c second is a copy of the second object
  _T3 thrid;                ///< @c third is a copy of the third object
  triple() : first(_T1()), second(_T2()), third(_T3()) {}
  /** Two objects may be passed to a @c triple constructor to be copied.  */
  triple(const _T1& __a, const _T2& __b) : first(__a), second(__b) {}

  /** There is also a templated copy ctor for the @c triple class itself.  */
  template <class _U1, class _U2, class _U3>
  triple(const triple<_U1, _U2, _U3>& __p) : first(__p.first), second(__p.second), third(__p.third {}
};

/// Two triples of the same type are equal iff their members are equal.
template <class _T1, class _T2, class T3>
inline bool operator==(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y)
{
    return __x.first == __y.first && __x.second == __y.second && __x.third == __y.third;
}

#if 0
/// <http://gcc.gnu.org/onlinedocs/libstdc++/20_util/howto.html#triplelt>
template <class _T1, class _T2, class T3>
inline bool operator<(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y)
{
  return __x.first < __y.first ||
         (!(__y.first < __x.first) && __x.second < __y.second);
}
#endif

/// Uses @c operator== to find the result.
template <class _T1, class _T2, class T3>
inline bool operator!=(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y) {
  return !(__x == __y);
}

#if 0
/// Uses @c operator< to find the result.
template <class _T1, class _T2, class T3>
inline bool operator>(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y) {
  return __y < __x;
}

/// Uses @c operator< to find the result.
template <class _T1, class _T2, class T3>
inline bool operator<=(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y) {
  return !(__y < __x);
}

/// Uses @c operator< to find the result.
template <class _T1, class _T2, class T3>
inline bool operator>=(const triple<_T1, _T2, _T3>& __x, const triple<_T1, _T2, _T3>& __y) {
  return !(__x < __y);
}
#endif

/**
 *  @brief A convenience wrapper for creating a triple from two objects.
 *  @param  x  The first object.
 *  @param  y  The second object.
 *  @return   A newly-constructed triple<> object of the appropriate type.
 *
 *  The standard requires that the objects be passed by reference-to-const,
 *  but LWG issue #181 says they should be passed by const value.  We follow
 *  the LWG by default.
*/
template <class _T1, class _T2, class T3>
inline triple<_T1, _T2, _T3> make_triple(_T1 __x, _T2 __y, _T3, __z)
{
  return triple<_T1, _T2, _T3>(__x, __y);
}

#endif

#endif // _STL_UTILS_H
