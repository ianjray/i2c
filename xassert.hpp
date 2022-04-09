#pragma once

#include <cassert>

// https://github.com/llvm/llvm-project/issues/51706
// (Note the semicolon in the macro expansion, caller also supplies semicolon which makes an empty statement to work around the clang bug.)
#define xassert(condition) assert(condition) ;
