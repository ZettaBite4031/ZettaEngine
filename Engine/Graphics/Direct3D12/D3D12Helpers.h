#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::D3DX {
	constexpr struct {
		D3D12_HEAP_PROPERTIES default_heap{
					D3D12_HEAP_TYPE_DEFAULT,			// Type;
					D3D12_CPU_PAGE_PROPERTY_UNKNOWN,	// CPUPageProperty
					D3D12_MEMORY_POOL_UNKNOWN,			// MemoryPoolPreference
					0,									// CreationNodeMask
					0									// VisibleNodeMask
		};
		D3D12_HEAP_PROPERTIES upload_heap{
					D3D12_HEAP_TYPE_UPLOAD,				// Type;
					D3D12_CPU_PAGE_PROPERTY_UNKNOWN,	// CPUPageProperty
					D3D12_MEMORY_POOL_UNKNOWN,			// MemoryPoolPreference
					0,									// CreationNodeMask
					0									// VisibleNodeMask
		};
	} heap_properties;

	constexpr struct {
		const D3D12_RASTERIZER_DESC no_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_NONE,
			0,
			0,
			0,
			0,
			1,
			1,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
		const D3D12_RASTERIZER_DESC backface_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			0,
			0,
			0,
			0,
			1,
			1,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
		const D3D12_RASTERIZER_DESC frontface_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_FRONT,
			0,
			0,
			0,
			0,
			1,
			1,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
		const D3D12_RASTERIZER_DESC wireframe_cull{
			D3D12_FILL_MODE_WIREFRAME,
			D3D12_CULL_MODE_NONE,
			0,
			0,
			0,
			0,
			1,
			1,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
	} RasterizerState;

	constexpr struct {
		const D3D12_DEPTH_STENCIL_DESC1 disabled{
			0,
			D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};
	} DepthState;

	class D3D12ResourceBarrier {
	public:
		constexpr static u32 MaxResourceBarriers{ 32 };

		// Add a transition barrier
		constexpr void Add(ID3D12Resource* resource,
			D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
			assert(resource);
			assert(_offset < MaxResourceBarriers);
			D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = flags;
			barrier.Transition.pResource = resource;
			barrier.Transition.StateBefore = before;
			barrier.Transition.StateAfter = after;
			barrier.Transition.Subresource = subresource;
			_offset++;
		}

		// Add a UAV barrier
		constexpr void Add(ID3D12Resource* resource,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
			assert(resource);
			assert(_offset < MaxResourceBarriers);
			D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = flags;
			barrier.UAV.pResource = resource;
			_offset++;
		}
		// Add an aliasing barrier
		constexpr void Add(ID3D12Resource* resource_before, ID3D12Resource* resource_after,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
			assert(resource_before && resource_after);
			assert(_offset < MaxResourceBarriers);
			D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
			barrier.Flags = flags;
			barrier.Aliasing.pResourceBefore = resource_before;
			barrier.Aliasing.pResourceAfter = resource_after;
			_offset++;
		}

		void Apply(ID3D12GraphicsCommandList* cmd_list) {
			assert(_offset);
			cmd_list->ResourceBarrier(_offset, _barriers);
			_offset = 0;
		}

	private:
		D3D12_RESOURCE_BARRIER _barriers[MaxResourceBarriers]{};
		u32 _offset{ 0 };
	};

	void TransitionResource(
		ID3D12GraphicsCommandList* cmd_list,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	ID3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& desc);

	struct D3D12DescriptorRange : public D3D12_DESCRIPTOR_RANGE1 {
		constexpr explicit D3D12DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type,
												u32 count, u32 shader_register, u32 space = 0,
												D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
												u32 offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) 
			: D3D12_DESCRIPTOR_RANGE1{type, count, shader_register, space, flags, offset} { }
	};

	struct D3D12RootParameter : public D3D12_ROOT_PARAMETER1 {
		constexpr void AsConstants(u32 num_constants, D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0) {
			ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			ShaderVisibility = visibility;
			Constants.Num32BitValues = num_constants;
			Constants.ShaderRegister = shader_register;
			Constants.RegisterSpace = space;
		}

		constexpr void AsCBV(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			AsDescriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, visibility, shader_register, space, flags);
		}

		constexpr void AsSRV(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			AsDescriptor(D3D12_ROOT_PARAMETER_TYPE_SRV, visibility, shader_register, space, flags);
		}

		constexpr void AsUAV(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			AsDescriptor(D3D12_ROOT_PARAMETER_TYPE_UAV, visibility, shader_register, space, flags);
		}

		constexpr void AsDescriptorTable(D3D12_SHADER_VISIBILITY visibility,
										const D3D12DescriptorRange* ranges, u32 count) {
			ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			ShaderVisibility = visibility;
			DescriptorTable.NumDescriptorRanges = count;
			DescriptorTable.pDescriptorRanges = ranges;
		}

	private:
		constexpr void AsDescriptor(D3D12_ROOT_PARAMETER_TYPE type, D3D12_SHADER_VISIBILITY visibility,
			u32 shader_register, u32 space, D3D12_ROOT_DESCRIPTOR_FLAGS flags) {
			ParameterType = type;
			ShaderVisibility = visibility;
			Descriptor.ShaderRegister = shader_register;
			Descriptor.RegisterSpace = space;
			Descriptor.Flags = flags;
		}
	};

	struct D3D12RootSignatureDesc : public D3D12_ROOT_SIGNATURE_DESC1 {
		constexpr explicit D3D12RootSignatureDesc(const D3D12RootParameter* params,
												u32 param_count,
												const D3D12_STATIC_SAMPLER_DESC* samplers = nullptr,
												u32 sample_count = 0, D3D12_ROOT_SIGNATURE_FLAGS flag =
												D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
												D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
												D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
												D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
												D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
												D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS) 
			: D3D12_ROOT_SIGNATURE_DESC1{ param_count, params, sample_count, samplers, flag} {}

		ID3D12RootSignature* Create() const {
			return CreateRootSignature(*this);
		}
	};

#pragma warning(push)
#pragma warning(disable : 4324) // Disable padding warning
	template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, typename T>
	class alignas(void*) D3D12PipelineStateSubobject {
	public:
		D3D12PipelineStateSubobject() = default;
		constexpr explicit D3D12PipelineStateSubobject(T subobject) : _type{ type }, _subobject{ subobject } {}
		D3D12PipelineStateSubobject& operator=(const T& subobject) { _subobject = subobject; return *this; }
	private:
		const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _type{ type };
		T _subobject{};
	};


#define PSS(name, ...) using D3D12PipelineStateSubobject##name = D3D12PipelineStateSubobject<__VA_ARGS__>;
	PSS(RootSignature, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*);
	PSS(VS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE);
	PSS(PS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE);
	PSS(DS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE);
	PSS(HS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE);
	PSS(GS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE);
	PSS(CS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE);
	PSS(StreamOutput, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC);
	PSS(Blend, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC);
	PSS(SampleMask, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, u32);
	PSS(Rasterizer, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC);
	PSS(DepthStencil, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC);
	PSS(InputLayout, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC);
	PSS(IBStripCutValue, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
	PSS(PrimitiveTopology, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE);
	PSS(RenderTargetFormats, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
	PSS(DepthStencilFormats, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT);
	PSS(SampleDesc, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC);
	PSS(NodeMask, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, u32);
	PSS(CachedPSO, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE);
	PSS(Flags, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS, D3D12_PIPELINE_STATE_FLAGS);
	PSS(DepthStencil1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1);
	PSS(ViewInstancing, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC);
	PSS(AS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE);
	PSS(MS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE);


	ID3D12PipelineState* CreatePipelineState(D3D12_PIPELINE_STATE_STREAM_DESC desc);
	ID3D12PipelineState* CreatePipelineState(void* stream, u64 size);

	ID3D12Resource* CreateBuffer(const void* data, u32 size, bool is_cpu_accessible = false,
								D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
								D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
								ID3D12Heap* heap = nullptr, u64 heap_offset = 0);
}
