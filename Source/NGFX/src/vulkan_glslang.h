#pragma once
#include "ngfx.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef BUILD_VULKAN_GLSLANG
#if _MSC_VER
#define V_API __declspec(dllexport)
#else
#define V_API __attribute__((visibility("default")))
#endif
#else
#if _MSC_VER
#define V_API __declspec(dllimport)
#else
#define V_API
#endif
#endif

typedef struct function_data_t* function_data;
extern V_API void               release_function_data(function_data* data);
extern V_API ngfxShaderType     get_function_data_stage(const function_data* data);
extern V_API void               compile_from_source(const char* source, function_data* data);
extern V_API void               serialize_library(const function_data* data, const char* save_path);

#if __cplusplus

using ByteCode = std::vector<uint32_t>;

struct FunctionData
{
  ByteCode          ByteCodes;
  ngfx::ShaderType  Stage;
};

struct EntryInfo
{
  char      Name[128];
  char      Entry[128];
  uint32_t  ShaderType;
  uint32_t  Size;
  uint32_t  OffSet;
};

using FunctionMap = std::unordered_map<std::string, FunctionData>;

extern V_API 
ngfx::Result CompileFromSource(const ngfx::CompileOption& Opt, const char* pSource, FunctionMap& FuncMap, std::string& ErrorInfo);

extern V_API
ngfx::Result SerializeLibrary(const FunctionMap& Data, const char* Path);

#endif