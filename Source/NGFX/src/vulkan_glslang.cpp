#include <Core/CoreMinimal.h>

#include "vulkan_glslang.h"
using namespace ngfx;

#include <SPIRV/GlslangToSpv.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <spirv-tools/optimizer.hpp>

using namespace glslang;

EShLanguage ConvertShaderTypeToGlslangEnum(ngfx::ShaderType const& e) {
  switch (e) {
  case ShaderType::Vertex:
    return EShLangVertex;
  case ShaderType::Fragment:
    return EShLangFragment;
  case ShaderType::Compute:
    return EShLangCompute;
  case ShaderType::Geometry:
    return EShLangGeometry;
  case ShaderType::TessailationEval:
    return EShLangTessEvaluation;
  case ShaderType::TessailationControl:
    return EShLangTessControl;
  }
  return EShLangCount;
}

ShaderType ConvertGlslangEnum(EShLanguage const& e) {
  switch (e) {
  case EShLangVertex:
    return ShaderType::Vertex;
  case EShLangFragment:
    return ShaderType::Fragment;
  case EShLangCompute:
    return ShaderType::Compute;
  case EShLangGeometry:
    return ShaderType::Geometry;
  case EShLangTessEvaluation:
    return ShaderType::TessailationEval;
  case EShLangTessControl:
    return ShaderType::TessailationControl;
  }
  return ShaderType::Vertex;
}

void sInitializeGlSlang()
{
  static bool sGlSlangIntialized = false;
  if (!sGlSlangIntialized) {
    glslang::InitializeProcess();
    sGlSlangIntialized = true;
  }
}

void sFinializeGlSlang()
{
  static bool sGlSlangFinalized = false;
  if (!sGlSlangFinalized) {
    glslang::FinalizeProcess();
    sGlSlangFinalized = true;
  }
}

void initResources(TBuiltInResource &resources)
{
  resources.maxLights = 32;
  resources.maxClipPlanes = 6;
  resources.maxTextureUnits = 32;
  resources.maxTextureCoords = 32;
  resources.maxVertexAttribs = 64;
  resources.maxVertexUniformComponents = 4096;
  resources.maxVaryingFloats = 64;
  resources.maxVertexTextureImageUnits = 32;
  resources.maxCombinedTextureImageUnits = 80;
  resources.maxTextureImageUnits = 32;
  resources.maxFragmentUniformComponents = 4096;
  resources.maxDrawBuffers = 32;
  resources.maxVertexUniformVectors = 128;
  resources.maxVaryingVectors = 8;
  resources.maxFragmentUniformVectors = 16;
  resources.maxVertexOutputVectors = 16;
  resources.maxFragmentInputVectors = 15;
  resources.minProgramTexelOffset = -8;
  resources.maxProgramTexelOffset = 7;
  resources.maxClipDistances = 8;
  resources.maxComputeWorkGroupCountX = 65535;
  resources.maxComputeWorkGroupCountY = 65535;
  resources.maxComputeWorkGroupCountZ = 65535;
  resources.maxComputeWorkGroupSizeX = 1024;
  resources.maxComputeWorkGroupSizeY = 1024;
  resources.maxComputeWorkGroupSizeZ = 64;
  resources.maxComputeUniformComponents = 1024;
  resources.maxComputeTextureImageUnits = 16;
  resources.maxComputeImageUniforms = 8;
  resources.maxComputeAtomicCounters = 8;
  resources.maxComputeAtomicCounterBuffers = 1;
  resources.maxVaryingComponents = 60;
  resources.maxVertexOutputComponents = 64;
  resources.maxGeometryInputComponents = 64;
  resources.maxGeometryOutputComponents = 128;
  resources.maxFragmentInputComponents = 128;
  resources.maxImageUnits = 8;
  resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
  resources.maxCombinedShaderOutputResources = 8;
  resources.maxImageSamples = 0;
  resources.maxVertexImageUniforms = 0;
  resources.maxTessControlImageUniforms = 0;
  resources.maxTessEvaluationImageUniforms = 0;
  resources.maxGeometryImageUniforms = 0;
  resources.maxFragmentImageUniforms = 8;
  resources.maxCombinedImageUniforms = 8;
  resources.maxGeometryTextureImageUnits = 16;
  resources.maxGeometryOutputVertices = 256;
  resources.maxGeometryTotalOutputComponents = 1024;
  resources.maxGeometryUniformComponents = 1024;
  resources.maxGeometryVaryingComponents = 64;
  resources.maxTessControlInputComponents = 128;
  resources.maxTessControlOutputComponents = 128;
  resources.maxTessControlTextureImageUnits = 16;
  resources.maxTessControlUniformComponents = 1024;
  resources.maxTessControlTotalOutputComponents = 4096;
  resources.maxTessEvaluationInputComponents = 128;
  resources.maxTessEvaluationOutputComponents = 128;
  resources.maxTessEvaluationTextureImageUnits = 16;
  resources.maxTessEvaluationUniformComponents = 1024;
  resources.maxTessPatchComponents = 120;
  resources.maxPatchVertices = 32;
  resources.maxTessGenLevel = 64;
  resources.maxViewports = 16;
  resources.maxVertexAtomicCounters = 0;
  resources.maxTessControlAtomicCounters = 0;
  resources.maxTessEvaluationAtomicCounters = 0;
  resources.maxGeometryAtomicCounters = 0;
  resources.maxFragmentAtomicCounters = 8;
  resources.maxCombinedAtomicCounters = 8;
  resources.maxAtomicCounterBindings = 1;
  resources.maxVertexAtomicCounterBuffers = 0;
  resources.maxTessControlAtomicCounterBuffers = 0;
  resources.maxTessEvaluationAtomicCounterBuffers = 0;
  resources.maxGeometryAtomicCounterBuffers = 0;
  resources.maxFragmentAtomicCounterBuffers = 1;
  resources.maxCombinedAtomicCounterBuffers = 1;
  resources.maxAtomicCounterBufferSize = 16384;
  resources.maxTransformFeedbackBuffers = 4;
  resources.maxTransformFeedbackInterleavedComponents = 64;
  resources.maxCullDistances = 8;
  resources.maxCombinedClipAndCullDistances = 8;
  resources.maxSamples = 4;
  resources.limits.nonInductiveForLoops = 1;
  resources.limits.whileLoops = 1;
  resources.limits.doWhileLoops = 1;
  resources.limits.generalUniformIndexing = 1;
  resources.limits.generalAttributeMatrixVectorIndexing = 1;
  resources.limits.generalVaryingIndexing = 1;
  resources.limits.generalSamplerIndexing = 1;
  resources.limits.generalVariableIndexing = 1;
  resources.limits.generalConstantMatrixVectorIndexing = 1;
}

ngfx::Result CompileFromSource(const ngfx::CompileOption & Opt, const char * pSource, FunctionMap & FuncMap, 
  std::string& ErrorInfo)
{
  ShInitialize();

  EShMessages messages =
      (EShMessages)(EShMsgVulkanRules | EShMsgSpvRules | EShMsgReadHlsl);
  TBuiltInResource Resources;
  initResources(Resources);
  const char* shaderStrings[1];
  TShader* shader = new TShader(EShLangVertex);

  shaderStrings[0] = (const char*)pSource;
  shader->setStrings(shaderStrings, 1);
  shader->setEntryPoint("");

  if (!shader->parse(&Resources, 100, false, messages))
  {
    ErrorInfo = shader->getInfoLog();
    ErrorInfo += std::string("\n");
    ErrorInfo += shader->getInfoDebugLog();
    ShFinalize();
    return Result::Failed;
  }

  for (int i = 0; i < shader->getNumDeclEntryPoints(); i++)
  {
    const char *name = shader->getDeclEntryName(i);
    EShLanguage lang = shader->getDeclEntryStage(name);
    FunctionData data;
    data.Stage = ConvertGlslangEnum(lang);

    TShader* tmpShader = new TShader(lang);
    shaderStrings[0] = (const char*)pSource;
    tmpShader->setStrings(shaderStrings, 1);
    tmpShader->setEntryPoint(name);

    if (!tmpShader->parse(&Resources, 100, false, messages))
    {
      ErrorInfo = tmpShader->getInfoLog();
      ErrorInfo += std::string("\n");
      ErrorInfo += tmpShader->getInfoDebugLog();

      ShFinalize();
      return Result::Failed;
    }
    TProgram& program = *new TProgram;
    program.addShader(tmpShader);

    if (!program.link(messages))
    {
      ErrorInfo += "\n";
      ErrorInfo += program.getInfoLog();
      ErrorInfo += "\n";
      ErrorInfo += program.getInfoDebugLog();

      ShFinalize();
      return Result::Failed;
    }
    if (program.buildReflection())
    {
    }
    GlslangToSpv(*program.getIntermediate(lang), data.ByteCodes);
    spvtools::SpirvTools core(SPV_ENV_VULKAN_1_0);
    spvtools::Optimizer opt(SPV_ENV_VULKAN_1_0);
    opt.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass())
        .RegisterPass(spvtools::CreateUnifyConstantPass())
        /*.RegisterPass(spvtools::CreateStripDebugInfoPass())*/;
    if (!opt.Run(data.ByteCodes.data(), data.ByteCodes.size(), &data.ByteCodes))
    {
        ShFinalize();
        return Result::Failed;
    }
    FuncMap[name] = data;
    delete tmpShader;
  }

  ShFinalize();
  return Result::Ok;
}

/*
struct EntryInfo
{
  char      Name[128];
  char      Entry[128];
  uint32_t  ShaderType;
  uint32_t  Size;
  uint32_t  OffSet;
};
*/

ngfx::Result SerializeLibrary(const FunctionMap& Data, const char* Path)
{
  if (Data.empty()) return Result::Failed;
  k3d::os::File OutputLib(Path);
  OutputLib.Open(k3d::IOFlag::Write);
  k3d::Archive ArchLib;
  ArchLib.SetIODevice(&OutputLib);
  ArchLib.ArrayIn("VKBC", 4);
  ArchLib << (uint32_t)2017u;
  ArchLib << (uint32_t)Data.size();
  uint32_t Offset = 0;
  for (auto pair : Data)
  {
    auto& fData = pair.second;
    auto SzByteCode = fData.ByteCodes.size() * sizeof(uint32_t);
    EntryInfo Info;
    memset(&Info, 0, sizeof(Info));
    Info.OffSet = Offset;
    Info.Size = SzByteCode; // BlobSize
    Info.ShaderType = (uint32_t)pair.second.Stage;
    strncpy(Info.Name, pair.first.c_str(), 127);
    ArchLib << Info;
    
    Offset += Info.Size;
  }

  for (auto pair : Data)
  {
    ArchLib.ArrayIn(pair.second.ByteCodes.data(), pair.second.ByteCodes.size());
  }
  OutputLib.Close();
  return Result::Ok;
}