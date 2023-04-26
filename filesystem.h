#ifndef LEGACYUTIL_H
#define LEGACYUTIL_H

#ifdef USE_EXPR_FILESYSTEM
#include <experimental/filesystem>
#define FILESYSTEM_NAMESPACE std::experimental::filesystem
#else
#include <filesystem>
#define FILESYSTEM_NAMESPACE std::filesystem
#endif

#endif
