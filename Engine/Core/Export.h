#pragma once

#ifdef _WIN32
#ifdef GINI_ENGINE_EXPORTS
#define GINI_API __declspec(dllexport)
#else
#define GINI_API __declspec(dllimport)
#endif
#else
#ifdef GINI_ENGINE_EXPORTS
#define GINI_API __attribute__((visibility("default")))
#else
#define GINI_API __attribute__((visibility("default")))
#endif
#endif
