#include "D3D12Helpers.h"
#include "D3D12Core.h"
#include "D3D12Upload.h"

namespace Zetta::Graphics::D3D12::D3DX {
	namespace {

	}

	void TransitionResource(
		ID3D12GraphicsCommandList* cmd_list, ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
		D3D12_RESOURCE_BARRIER_FLAGS flags, u32 subresource) {

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = flags;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = subresource;

		cmd_list->ResourceBarrier(1, &barrier);
	}

	ID3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& desc) {
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_desc{};
		versioned_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versioned_desc.Desc_1_1 = desc;

		using namespace Microsoft::WRL;
		ComPtr<ID3DBlob> signature_blob{ nullptr };
		ComPtr<ID3DBlob> error_blob{ nullptr };
		HRESULT hr{ S_OK };
		if (FAILED(hr = D3D12SerializeVersionedRootSignature(&versioned_desc, &signature_blob, &error_blob))) {
			DEBUG_OP(const char* error_msg{ error_blob ? (const char*)error_blob->GetBufferPointer() : "" });
			DEBUG_OP(OutputDebugStringA(error_msg));
			return nullptr;
		}

		ID3D12RootSignature* signature{ nullptr };
		DXCall(hr = Core::Device()->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&signature)));
		if (FAILED(hr)) Core::Release(signature);

		return signature;
	}

	HRESULT CreatePipelineState(D3D12_PIPELINE_STATE_STREAM_DESC desc, ID3D12PipelineState** pso) {
		assert(desc.pPipelineStateSubobjectStream && desc.SizeInBytes);
		try {
			Core::Device()->CreatePipelineState(&desc, IID_PPV_ARGS(pso));
			assert(*pso);
		}
		catch (std::exception e) {
			return 69;
		}

		return S_OK;
	}

	HRESULT CreatePipelineState(void* stream, u64 size, ID3D12PipelineState** pso) {
		assert(stream && size);
		D3D12_PIPELINE_STATE_STREAM_DESC desc{};
		desc.SizeInBytes = size;
		desc.pPipelineStateSubobjectStream = stream;
		CreatePipelineState(desc, pso);
		return S_OK;
	}

	ID3D12Resource* CreateBuffer(const void* data, u32 size, bool is_cpu_accessible,
								D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, 
								ID3D12Heap* heap, u64 heap_offset) {
		assert(size);

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc = { 1,0 };
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = is_cpu_accessible ? D3D12_RESOURCE_FLAG_NONE : flags;

		assert(desc.Flags == D3D12_RESOURCE_FLAG_NONE ||
			desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		ID3D12Resource* resource{ nullptr };
		const D3D12_RESOURCE_STATES resource_state{ is_cpu_accessible ? D3D12_RESOURCE_STATE_GENERIC_READ : state };

		if (heap) {
			DXCall(Core::Device()->CreatePlacedResource(
				heap, heap_offset, &desc, resource_state,
				nullptr, IID_PPV_ARGS(&resource)));
		}
		else {
			DXCall(Core::Device()->CreateCommittedResource(
				is_cpu_accessible ? &heap_properties.upload_heap : &heap_properties.default_heap,
				D3D12_HEAP_FLAG_NONE, &desc, resource_state,
				nullptr, IID_PPV_ARGS(&resource)));
		}

		if (data) {
			if (is_cpu_accessible) {
				D3D12_RANGE range{};
				void* cpu_addr{ nullptr };
				DXCall(resource->Map(0, &range, reinterpret_cast<void**>(&cpu_addr)));
				assert(cpu_addr);
				memcpy(cpu_addr, data, size);
				resource->Unmap(0, nullptr);
			}
			else {
				Upload::D3D12UploadContext context{ size };
				memcpy(context.CPUAddress(), data, size);
				context.CommandList()->CopyResource(resource, context.UploadBuffer());
				context.EndUpload();
			}
		}

		assert(resource);
		return resource;
	}
}