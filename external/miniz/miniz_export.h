#pragma once

#ifndef MINIZ_EXPORT
  #ifdef _WIN32
    #ifdef MINIZ_EXPORTS
      #define MINIZ_EXPORT __declspec(dllexport)
    #else
      #define MINIZ_EXPORT
    #endif
  #else
    #define MINIZ_EXPORT
  #endif
#endif
