#include <Core/CoreMinimal.h>
#include <ngfx.h>
#include <ngfxu.h>
#include "gtest/gtest.h"

using namespace k3d;

#if _WIN32
#pragma comment(linker,"/subsystem:console")
#endif

using namespace ngfx;

Ptr<Factory> GlobalTestFactory;
Ptr<Device> GlobalTestDevice;
Ptr<CommandQueue> gfxQueue;
Ptr<RenderPass> GlobalTestRenderPass;
Ptr<BindTableEncoder> GlobalTableEncoder;
Ptr<PipelineLayout> GlobalPipelineLayout;
Ptr<ngfx::Function> ComputeFunction;

Ptr<ngfx::Function> VertexFunction;
Ptr<ngfx::Function> PixelFunction;
Ptr<Texture> DefaultRenderTexture2D;
Ptr<Pipeline> GlobalPipeline;
Ptr<Pipeline> GlobalComputePipeline; 

using PtrLib = k3d::SharedPtr<os::LibraryLoader>;

PtrLib GlobalGfxLib;

typedef Result(*PFNCreateFactory)(Factory ** ppFactory, bool debugEnabled);
PFNCreateFactory fnCreateFactory;

TEST(LibLoader, rhi)
{
    GlobalGfxLib = MakeShared<os::LibraryLoader>("ngfx_vulkan.dll");
    fnCreateFactory = reinterpret_cast<PFNCreateFactory>(GlobalGfxLib->ResolveSymbol("CreateFactory"));
    ASSERT_TRUE(fnCreateFactory);
}

TEST(CreateFactory, ngfxFactory)
{
  fnCreateFactory(GlobalTestFactory.GetAddressOf(), true);
  ASSERT_TRUE(GlobalTestFactory.Get());
}

TEST(CreateDevice, ngfxDevice)
{
  uint32_t Count = 0;
  GlobalTestFactory->EnumDevice(&Count, nullptr);
  ASSERT_TRUE(Count > 0);
  GlobalTestFactory->EnumDevice(&Count, GlobalTestDevice.GetAddressOf());
  ASSERT_TRUE(GlobalTestDevice.Get());
}

TEST(CreateLibrary, ngfxLibrary)
{
  Ptr<Library> library;
  Ptr<ngfx::Function> function;
  os::MemMapFile blobFile;
  String path = os::Join(GetEnv().GetDataDir(), "Test", String("Test.blob"));
  ASSERT_TRUE(blobFile.Open(path.CStr(), IOFlag::Read));
  GlobalTestDevice->CreateLibrary(nullptr, blobFile.FileData(), blobFile.GetSize(), library.GetAddressOf());
  blobFile.Close();
  ASSERT_TRUE(library.Get());
  library->MakeFunction("MainVS", VertexFunction.GetAddressOf());
  ASSERT_TRUE(VertexFunction.Get());
  ASSERT_STREQ("MainVS", VertexFunction->Name());
  ASSERT_EQ(ShaderType::Vertex, VertexFunction->Type());
  library->MakeFunction("MainCS", ComputeFunction.GetAddressOf());
  ASSERT_STREQ("MainCS", ComputeFunction->Name());
  ASSERT_EQ(ShaderType::Compute, ComputeFunction->Type());

  library->MakeFunction("MainPS", PixelFunction.GetAddressOf());
  ASSERT_TRUE(PixelFunction.Get());
  ASSERT_STREQ("MainPS", PixelFunction->Name());
  ASSERT_EQ(ShaderType::Fragment, PixelFunction->Type());
}

TEST(CreateQueue, ngfxQueue)
{
  Ptr<CommandQueue> compQueue, copyQueue;
  GlobalTestDevice->CreateCommandQueue(CommandQueueType::Graphics, gfxQueue.GetAddressOf());
  ASSERT_TRUE(gfxQueue.Get());
  GlobalTestDevice->CreateCommandQueue(CommandQueueType::Compute, compQueue.GetAddressOf());
  ASSERT_TRUE(compQueue.Get());
  GlobalTestDevice->CreateCommandQueue(CommandQueueType::Copy, copyQueue.GetAddressOf());
  ASSERT_TRUE(copyQueue.Get());
  ASSERT_TRUE(__is_enum(BufferViewBit));
}

TEST(CreateBindTableEncoder, ngfxBindTableEncoder)
{
    DynArray<ArgumentDesc> argumentDescs;
    argumentDescs.Resize(4);
    argumentDescs[0].index = 2;
    argumentDescs[0].dataType = DataType::Texture;
    argumentDescs[0].textureDim = TextureDimension::Buffer;
    argumentDescs[0].access |= ArgumentAccess::WriteOnly;
    argumentDescs[0].access |= ArgumentAccess::ReadOnly;
    argumentDescs[0].stage = ShaderStageBit::Compute;

    argumentDescs[1].index = 3;
    argumentDescs[1].dataType = DataType::Texture;
    argumentDescs[1].textureDim = TextureDimension::Buffer;
    argumentDescs[1].access |= ArgumentAccess::WriteOnly;
    argumentDescs[1].access |= ArgumentAccess::ReadOnly;
    argumentDescs[1].stage = ShaderStageBit::Compute;

    argumentDescs[2].index = 1;
    argumentDescs[2].dataType = DataType::Array;
    argumentDescs[2].access = ArgumentAccess::ReadOnly;
    argumentDescs[2].stage = ShaderStageBit::Compute;

    argumentDescs[3].index = 0;
    argumentDescs[3].dataType = DataType::Array;
    argumentDescs[3].access = ArgumentAccess::ReadOnly;
    argumentDescs[3].stage = ShaderStageBit::Compute;

    GlobalTestDevice->MakeBindTableEncoder(
        argumentDescs.Data(), 
        argumentDescs.Count(), 
        GlobalTableEncoder.GetAddressOf());
/*
  Ptr<BindTableLayoutInitializer> btlInitializer;
  GlobalTestDevice->CreateBindTableLayoutInitializer(btlInitializer.GetAddressOf());
  btlInitializer->AddBuffer(0, 1, ShaderStageBit::Compute);
  btlInitializer->Initialize(GlobalTableLayout.GetAddressOf());
*/
  ASSERT_TRUE(GlobalTableEncoder.Get() != nullptr);
}

TEST(CreateBindTable, ngfxBindTable)
{
  Ptr<BindTable> bindTable;
  ASSERT_TRUE(Result::Ok == GlobalTableEncoder->Allocate(bindTable.GetAddressOf()));
  ASSERT_TRUE(bindTable.Get() != nullptr);

  bindTable->SetBuffer(0, ShaderType::Fragment, nullptr);
  bindTable->SetSampler(1, ShaderType::Fragment, nullptr);
  bindTable->SetTexture(2, ShaderType::Fragment, nullptr);

  Ptr<BindTable> bindTable2;
  ASSERT_TRUE(Result::Ok == GlobalTableEncoder->Allocate(bindTable2.GetAddressOf()));

  bindTable2->SetBuffer(0, ShaderType::Vertex, nullptr);
  bindTable2->SetSampler(1, ShaderType::Vertex, nullptr);
  bindTable2->SetTexture(2, ShaderType::Vertex, nullptr);
}

TEST(CreatePipelineLayout, ngfxPipelineLayout)
{
  PipelineLayoutDesc desc = { GlobalTableEncoder.Get(), 1 };
  GlobalTestDevice->CreatePipelineLayout(&desc, GlobalPipelineLayout.GetAddressOf());
  ASSERT_TRUE(GlobalPipelineLayout.Get() != nullptr);
}

Ptr<Texture> texture;
Ptr<TextureView> textureView;

TEST(CreateTexture, ngfxTexture)
{
  TextureDesc desc;
  desc.allowedViewBits |= TextureViewBit::ShaderRead;
  desc.SetWidth(1024).SetHeight(1024).SetDepth(1).SetLayers(1).SetMipLevels(1);
  GlobalTestDevice->CreateTexture(&desc, texture.GetAddressOf());
  ASSERT_TRUE(texture.Get() != nullptr);

  Ptr<Texture> DepthStencilTexture = ngfxu::CreateDepthStencilTexture(
    GlobalTestDevice, PixelFormat::D32Float, 1024, 1024);
  ASSERT_TRUE(DepthStencilTexture.Get());

  DefaultRenderTexture2D = ngfxu::CreateRenderTexture2D(
    GlobalTestDevice, PixelFormat::RGBA8UNorm, 1024, 1024);
  ASSERT_TRUE(DefaultRenderTexture2D.Get());

  Ptr<Texture> SampledTexture2D = ngfxu::CreateSampledTexture2D(
    GlobalTestDevice, PixelFormat::RGBA8UNorm, 1024, 1024);
  ASSERT_TRUE(SampledTexture2D.Get());

  uint32_t * pData = (uint32_t *)SampledTexture2D->Map(0, 4 * 8);
  *pData = 2;
  *(pData + 7) = 9;
  ASSERT_TRUE(pData);
  SampledTexture2D->UnMap();
}

TEST(CreateTextureView, ngfxTextureView)
{
  TextureViewDesc desc{};
  desc.SetDimension(TextureDimension::Tex2D)
    .SetView(ResourceViewType::SampledTexture);
  texture->CreateView(&desc, textureView.GetAddressOf());
  ASSERT_TRUE(textureView.Get() != nullptr);
}

Ptr<Buffer> buffer;
Ptr<BufferView> bufferView;

TEST(CreateBuffer, ngfxBuffer)
{
  BufferDesc bufferDesc{ BufferViewBit::UnOrderedAccess, StorageOption::Managed, 10 };
  GlobalTestDevice->CreateBuffer(&bufferDesc, buffer.GetAddressOf());
  buffer->SetName("Buffer0");
  ASSERT_TRUE(buffer.Get() != nullptr);
}

TEST(CreateBufferView, ngfxBufferView)
{
  BufferViewDesc bufferViewDesc = {
    ResourceViewType::UnorderAccessBuffer,
    ResourceState::UnorderAccess,
    10, 0
  };
  buffer->CreateView(&bufferViewDesc, bufferView.GetAddressOf());
  ASSERT_TRUE(bufferView.Get() != nullptr);
}

/* Render Pass Creation */
TEST(CreateRenderPass, ngfxRenderPass)
{
    RenderPassDesc desc;
    GlobalTestDevice->CreateRenderPass(&desc, GlobalTestRenderPass.GetAddressOf());
    ASSERT_TRUE(GlobalTestRenderPass.Get());
    ColorAttachmentDesc colorDesc;
    colorDesc.SetLoadAction(LoadAction::Load)
        .SetStoreAction(StoreAction::Store)
        .SetTexture(DefaultRenderTexture2D.Get());
    desc.SetColorAttachmentsCount(1).SetPColorAttachments(&colorDesc);
    desc.SetPDepthStencilAttachment(nullptr);
    GlobalTestDevice->CreateRenderPass(&desc, GlobalTestRenderPass.GetAddressOf());
    ASSERT_TRUE(GlobalTestRenderPass.Get());
}

TEST(CreateFrameBuffer, ngfxFrameBuffer)
{
  Ptr<FrameBuffer> frameBuffer;
  Ptr<TextureView> colorAttachView;
  TextureViewDesc renderTextureDesc{};
  renderTextureDesc.SetDimension(TextureDimension::Tex2D)
      .SetView(ResourceViewType::SampledTexture)
      .SetMipLevel(1)
      .SetState(ResourceState::FrameBuffer);
  auto Ret = DefaultRenderTexture2D->CreateView(&renderTextureDesc, colorAttachView.GetAddressOf());
  ASSERT_TRUE(colorAttachView.Get() && Ret == Result::Ok);

  const TextureView* attachments[] = { colorAttachView.Get() };
  Ret = GlobalTestRenderPass->MakeFrameBuffer(attachments, 1, 1024, 1024, 1, frameBuffer.GetAddressOf());
  ASSERT_TRUE(frameBuffer.Get() && Ret == Result::Ok);
}

TEST(CreateSampler, ngfxSampler)
{
  Ptr<Sampler> sampler;
  SamplerDesc samplerDesc = {};
  GlobalTestDevice->CreateSampler(&samplerDesc, sampler.GetAddressOf());
  ASSERT_TRUE(sampler.Get());
}

TEST(CreateComputePipeline, ngfxComputePipeline)
{
  Ptr<Pipeline> SimplePipeline;
  Ptr<PipelineReflection> SimpleReflection;
  auto Ret = GlobalTestDevice->CreateComputePipeline(
      ComputeFunction.Get(), 
      GlobalPipelineLayout.Get(),
      SimplePipeline.GetAddressOf());
  ASSERT_TRUE(Ret == Result::Ok);
}

TEST(CreateRenderPipeline, ngfxRenderPipeline)
{
  RenderPipelineDesc desc;
  // functions
  desc.SetVertexFunction(VertexFunction.Get())
      .SetPixelFunction(PixelFunction.Get());
  // assemble
  VertexAttribute vertexAttrib;
  vertexAttrib.slot = 0;
  vertexAttrib.offset = 0;
  vertexAttrib.format = VertexFormat::F32C3;
  VertexLayout vertexLayout;
  vertexLayout.rate = VertexInputRate::PerVertex;
  vertexLayout.stride = sizeof(float) * 3;
  VertexInputState inputState;
  inputState.pAttributes = &vertexAttrib;
  inputState.attributeCount = 1;
  inputState.pLayouts = &vertexLayout;
  inputState.layoutCount = 1;
  desc.SetInputState(inputState);
  // blend state
  BlendState blendState;
  blendState.renderTargets[0] = {};
  blendState.logicOpEnable = false;
  blendState.alphaToCoverageEnable = false;
  desc.SetBlendState(blendState);
  desc.SetNumRenderTargets(1);
  desc.SetRenderTargetFormat(0, PixelFormat::RGBA8UNorm);
  // depth stencil
  DepthStencilState dsState = {};
  desc.SetDepthStencil(dsState);
  desc.SetDepthStencilFormat(PixelFormat::D32Float);
  // raster
  RasterizerState rsState = {};
  desc.SetRasterState(rsState);
  GlobalTestRenderPass->MakeRenderPipeline(&desc,
    GlobalPipelineLayout.Get(),
    GlobalPipeline.GetAddressOf());
  ASSERT_TRUE(GlobalPipeline.Get() != nullptr);
}

TEST(CreateCommandBuffer, ngfxCommandBuffer)
{
  Ptr<CommandBuffer> commandBuffer;
  gfxQueue->CreateCommandBuffer(commandBuffer.GetAddressOf());
  ASSERT_TRUE(commandBuffer.Get());
  Ptr<RenderCommandEncoder> renderEncoder;
  commandBuffer->CreateRenderCommandEncoder(nullptr, nullptr, renderEncoder.GetAddressOf());
  ASSERT_TRUE(renderEncoder.Get());
}

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}