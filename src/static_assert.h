/* Copyright (C) 2008 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STATIC_ASSERT
#define STATIC_ASSERT

/* The static_assert is achieved by creating a struct definition which contains
   an array of negative size if the condition is false. To prevent multiple
   static assertions to define the same struct, the line number on which the
   assertion name of the struct is made is appended to the name of the struct.
   However, to do this we need another layer of indirection because the
   arguments of ## are not expanded before pasting.
*/
#define __static_assert(_condition, _line) struct __static_assert_##_line { int static_assert_failed[_condition ? 1 : -1]; }
#define _static_assert(_condition, _line) __static_assert(_condition, _line)
#define static_assert(_condition) _static_assert(_condition, __LINE__)

#endif
