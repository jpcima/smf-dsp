//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <iosfwd>

template <class T> class PointT;
typedef PointT<int> Point;

template <class T> struct Point_Traits;

//------------------------------------------------------------------------------
template <class T>
class PointT : public Point_Traits<T>::storage_type {
 public:
  typedef typename Point_Traits<T>::storage_type storage_type;

  PointT() : storage_type{} {}
  PointT(storage_type s) : storage_type(s) {}
  PointT(T x, T y) : storage_type{x, y} {}

  PointT<T> off_by(const PointT<T> &p) const {
    return PointT<T>(this->x + p.x, this->y + p.y);
  }
};

//------------------------------------------------------------------------------
#include <SDL_rect.h>
template <> struct Point_Traits<int> {
  typedef SDL_Point storage_type;
};

template <class T> struct Point_Traits {
  struct storage_type { T x, y; };
};

//------------------------------------------------------------------------------
template <class T, class Ch>
std::basic_ostream<Ch> &operator<<(std::basic_ostream<Ch> &o, const PointT<T> &p) {
  return o << "{" << p.x << ", " << p.y << "}";
}
