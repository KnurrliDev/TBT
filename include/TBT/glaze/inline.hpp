/*
MIT License

Copyright (c) 2019 - present, Stephen Berry

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
https://github.com/stephenberry/glaze - stephenberry.developer@gmail.com
*/

#pragma once

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
#ifndef GLZ_USE_ALWAYS_INLINE
#define GLZ_USE_ALWAYS_INLINE
#endif
#endif

#if defined(GLZ_USE_ALWAYS_INLINE) && defined(NDEBUG)
#ifndef GLZ_ALWAYS_INLINE
#if defined(_MSC_VER) && !defined(__clang__)
#define GLZ_ALWAYS_INLINE [[msvc::forceinline]] inline
#else
#define GLZ_ALWAYS_INLINE inline __attribute__((always_inline))
#endif
#endif
#endif

#ifndef GLZ_ALWAYS_INLINE
#define GLZ_ALWAYS_INLINE inline
#endif

// IMPORTANT: GLZ_FLATTEN should only be used with extreme care
// It often adds to the binary size and greatly increases compilation times.
// It should only be applied in very specific circumstances.
// It is best to more often rely on the compiler.

#if (defined(__clang__) || defined(__GNUC__)) && defined(NDEBUG)
#ifndef GLZ_FLATTEN
#define GLZ_FLATTEN inline __attribute__((flatten))
#endif
#endif

#ifndef GLZ_FLATTEN
#define GLZ_FLATTEN inline
#endif

#ifndef GLZ_NO_INLINE
#if defined(__clang__) || defined(__GNUC__)
#define GLZ_NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define GLZ_NO_INLINE __declspec((noinline))
#endif
#endif
