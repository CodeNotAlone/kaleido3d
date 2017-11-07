/*
SimpleCL SPIRV & OpenCL
*/
#include <Core/CoreMinimal.h>
#include <ngfx.h>

#if _WIN32
#pragma comment(linker,"/subsystem:console")
#endif

using namespace ngfx;

void LoadSpv(const char * filePath, void ** ppData, uint32_t * dataSize)
{
  k3d::os::File file(filePath);
  file.Open(k3d::IOFlag::Read);
  *dataSize = file.GetSize();
  char* data = new char[*dataSize];
  file.Read(data, *dataSize);
  file.Close();
  *ppData = data;
}


int main()
{
  Ptr<Factory> factory;
  CreateFactory(factory.GetAddressOf(), true);
  factory->SetName("VulkanFactory");

  Ptr<Device> device;
  uint32_t Count = 0;
  factory->EnumDevice(&Count, nullptr);
  factory->EnumDevice(&Count, device.GetAddressOf());

  Ptr<CommandQueue> queue;
  device->CreateCommandQueue(CommandQueueType::Compute, queue.GetAddressOf());
  queue->SetName("ComputeQueue");

  Ptr<Buffer> buffer;
  BufferDesc bufferDesc{ BufferViewBit::UnOrderedAccess, StorageOption::Managed, 10 };
  device->CreateBuffer(&bufferDesc, buffer.GetAddressOf());
  buffer->SetName("Buffer0");

  Ptr<BufferView> bufferView;
  BufferViewDesc bufferViewDesc = {
    ResourceViewType::UnorderAccessBuffer, 
    ResourceState::UnorderAccess,
    10, 0
  };
  buffer->CreateView(&bufferViewDesc, bufferView.GetAddressOf());

  Ptr<Fence> fence;
  device->CreateFence(fence.GetAddressOf());
  fence->SetName("DefaultFence");

  void* pData = nullptr;
  uint32_t szData = 0;
  // clspv --vkbc
  LoadSpv("../../Data/Test/SimpleCL.spv", &pData, &szData);

  Ptr<Library> library;
  device->CreateLibrary(new CompileOption, pData, szData, library.GetAddressOf());
  Ptr<Function> clFunction;
  library->MakeFunction("main", clFunction.GetAddressOf());

  Ptr<PipelineLayout> pipelineLayout;
  PipelineLayoutDesc pipelineLayoutDesc = {};
  device->CreatePipelineLayout(&pipelineLayoutDesc, pipelineLayout.GetAddressOf());

  Ptr<BindTable> argTable;
  Ptr<Pipeline> computePipeline;
  Ptr<PipelineReflection> reflection;
  device->CreateComputePipeline(clFunction.Get(), computePipeline.GetAddressOf(), reflection.GetAddressOf());

  argTable->SetBuffer(0, ShaderType::Compute, bufferView.Get());

  Ptr<CommandBuffer> commandBuffer;
  Ptr<ComputeCommandEncoder> computeEncoder;
  queue->CreateCommandBuffer(commandBuffer.GetAddressOf());
  commandBuffer->CreateComputeCommandEncoder(computeEncoder.GetAddressOf());
  computeEncoder->SetPipeline(computePipeline.Get());
  computeEncoder->SetBindTable(argTable.Get());
  computeEncoder->Dispatch(1024, 1024, 1);
  computeEncoder->EndEncode();
  commandBuffer->Commit(fence.Get());

  return 0;
}