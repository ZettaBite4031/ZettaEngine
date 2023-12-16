#include "D3D12Resources.h"
#include "D3D12Core.h"
#include "D3D12Helpers.h"

namespace Zetta::Graphics::D3D12 {
/* ------------------------------- DESCRIPTOR HEAP --------------------------------------------------------------------------- */
#pragma region Descriptor Heap
	bool DescriptorHeap::Initialize(u32 capacity, bool is_shader_visible) {
		std::lock_guard lock{ _mutex };
		assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
		assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER &&
				capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
		
		if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
			_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) is_shader_visible = false;

		Release();
		
		ID3D12Device* const device{ Core::Device() };
		assert(device);

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = is_shader_visible ?
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = capacity;
		desc.Type = _type;
		desc.NodeMask = 0;
		
		HRESULT hr{ S_OK };
		DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
		if (FAILED(hr)) return true;

		_free_handles = std::move(std::make_unique<u32[]>(capacity));
		_capacity = capacity;
		_size = 0;

		for (u32 i{ 0 }; i < capacity; i++) _free_handles[i] = i;
		for (u32 i{ 0 }; i < FrameBufferCount; i++) assert(_deferred_free_indices[i].empty());

		_descriptor_size = device->GetDescriptorHandleIncrementSize(_type);
		_cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
		_gpu_start = is_shader_visible ?
			_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

		return true;
	}

	void DescriptorHeap::Release() {
		assert(!_size);
		Core::DeferredRelease(_heap);
	}

	void DescriptorHeap::ProcessDeferredFree(u32 frame_idx) {
		std::lock_guard lock{ _mutex };
		assert(frame_idx < FrameBufferCount);
		
		util::vector<u32>& indices{ _deferred_free_indices[frame_idx] };
		if (!indices.empty()) {
			for (auto& index : indices) {
				_size--;
				_free_handles[_size] = index;
			}
			indices.clear();
		}
	}

	DescriptorHandle DescriptorHeap::Alloc() {
		std::lock_guard lock{ _mutex };
		assert(_heap);
		assert(_size < _capacity);

		const u32 index{ _free_handles[_size] };
		const u32 offset{ index * _descriptor_size };
		_size++;

		DescriptorHandle handle;
		handle.cpu.ptr = _cpu_start.ptr + offset;
		if (IsShaderVisible()) handle.gpu.ptr = _gpu_start.ptr + offset;

		handle.index = index;
		DEBUG_OP(handle.container = this);
		return handle;
	}

	void DescriptorHeap::Free(DescriptorHandle& handle) {
		if (!handle.IsValid()) return;
		std::lock_guard lock{ _mutex };
		assert(_heap && _size);
		assert(handle.cpu.ptr >= _cpu_start.ptr);
		assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0);
		assert(handle.index < _capacity);
		const u32 index{ (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size };
		assert(handle.index == index);

		const u32 frame_index{ Core::CurrentFrameIndex() };
		_deferred_free_indices[frame_index].push_back(index);
		Core::SetDeferredReleasesFlag();
		handle = {};
	}
#pragma endregion

/* ------------------------------- D3D12 TEXTURE ----------------------------------------------------------------------------- */
#pragma region D3D12 Texture
	D3D12Texture::D3D12Texture(D3D12TextureInitInfo info) {
		auto* const device{ Core::Device() };
		assert(device);

		D3D12_CLEAR_VALUE* const clear_value{
			(info.desc && (info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
			info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) ?
			&info.clear_value : nullptr
		};

		if (info.resource) {
			assert(!info.heap);
			_resource = info.resource;
		}
		else if (info.heap && info.desc) {
			assert(!info.resource);
			device->CreatePlacedResource(info.heap, info.allocation_info.Offset, info.desc,
				info.initial_state, clear_value, IID_PPV_ARGS(&_resource));
		}
		else if (info.desc) {
			assert(!info.heap && !info.resource);

			DXCall(device->CreateCommittedResource(
				&D3DX::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc, 
				info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
		}
		assert(_resource);

		_srv = Core::SRV_Heap().Alloc();
		device->CreateShaderResourceView(_resource, info.srv_desc, _srv.cpu);
	}
	
	void D3D12Texture::Release() {
		Core::SRV_Heap().Free(_srv);
		Core::DeferredRelease(_resource);
	}
#pragma endregion

/* ------------------------------- D3D12 BUFFER	------------------------------------------------------------------------------ */
#pragma region D3D12 Buffer
	D3D12Buffer::D3D12Buffer(D3D12BufferInitInfo info, bool is_cpu_accessible) {
		assert(!_buffer && info.size && info.alignment);
		_size = (u32)Math::AlignSizeUp(info.size, info.alignment);
		_buffer = D3DX::CreateBuffer(info.data, _size, is_cpu_accessible, info.initial_state, info.flags,
									info.heap, info.allocation_info.Offset);
		_gpu_addr = _buffer->GetGPUVirtualAddress();
		NAME_D3D12_OBJECT_INDEXED(_buffer, _size, L"D3D12 Buffer - size");
	}

	void D3D12Buffer::Release() {
		Core::DeferredRelease(_buffer);
		_gpu_addr = 0;
		_size = 0;
	}

#pragma endregion

/* ------------------------------- CONSTANT BUFFER --------------------------------------------------------------------------- */
#pragma region Constant Buffer
	ConstantBuffer::ConstantBuffer(D3D12BufferInitInfo info) 
		: _buffer{ info, true } {
		NAME_D3D12_OBJECT_INDEXED(Buffer(), Size(), L"Constant Buffer - size");

		D3D12_RANGE range{};
		DXCall(Buffer()->Map(0, &range, (void**)(&_cpu_addr)));
		assert(_cpu_addr);
	}

	u8* const ConstantBuffer::alloc(u32 size) {
		std::lock_guard lock{ _mutex };
		const u32 aligned_size{ (u32)D3DX::ConstantBufferAlignSize(size) };
		assert(_cpu_offset + aligned_size <= _buffer.Size());
		if (_cpu_offset + aligned_size <= _buffer.Size()) {
			u8* const addr{ _cpu_addr + _cpu_offset };
			_cpu_offset += aligned_size;
			return addr;
		}

		return nullptr;
	}
#pragma endregion

/* ------------------------------- STRUCTURED BUFFER ------------------------------------------------------------------------- */
#pragma region Structured Buffer
	StructuredBuffer::StructuredBuffer(const D3D12BufferInitInfo& info) 
		: _buffer{ info, false }, _stride{ info.stride } {
		assert(info.size && info.size == (info.stride * info.element_count));
		assert(info.alignment > 0);
		NAME_D3D12_OBJECT_INDEXED(Buffer(), info.size, L"Structured Buffer - size");

		if (info.create_uav) {
			assert(info.flags && D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			_uav = Core::UAV_Heap().Alloc();
			_uav_shader_visible = Core::SRV_Heap().Alloc();
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Buffer.CounterOffsetInBytes = 0;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			desc.Buffer.NumElements = info.element_count;
			desc.Buffer.StructureByteStride = info.stride;

			Core::Device()->CreateUnorderedAccessView(Buffer(), nullptr, &desc, _uav.cpu);
			Core::Device()->CopyDescriptorsSimple(1, _uav_shader_visible.cpu, _uav.cpu, Core::SRV_Heap().Type());
		}
	}

	void StructuredBuffer::Release() {
		Core::SRV_Heap().Free(_uav_shader_visible);
		Core::UAV_Heap().Free(_uav);
		_buffer.Release();
	}
#pragma endregion
/* ------------------------------- RENDER TEXTURE ---------------------------------------------------------------------------- */
#pragma region Render Texture
	D3D12RenderTexture::D3D12RenderTexture(D3D12TextureInitInfo info) 
		: _texture{ info } {
		
		_mip_count = Resource()->GetDesc().MipLevels;
		assert(_mip_count && _mip_count <= D3D12Texture::max_mips);

		DescriptorHeap& rtv_heap{ Core::RTV_Heap() };
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = info.desc->Format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		auto* const device{ Core::Device() };
		assert(device);

		for (u32 i{ 0 }; i < _mip_count; i++) {
			_rtv[i] = rtv_heap.Alloc();
			device->CreateRenderTargetView(Resource(), &desc, _rtv[i].cpu);
			desc.Texture2D.MipSlice++;
		}
	}


	void D3D12RenderTexture::Release() {
		for (u32 i{ 0 }; i < _mip_count; i++) Core::RTV_Heap().Free(_rtv[i]);
		_texture.Release();
		_mip_count = 0;
	}

#pragma endregion

/* ------------------------------- DEPTH BUFFER ------------------------------------------------------------------------------ */
#pragma region Depth Buffer

	D3D12DepthBuffer::D3D12DepthBuffer(D3D12TextureInitInfo info) {
		assert(info.desc);
		const DXGI_FORMAT dsv_format{ info.desc->Format };

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		if (info.desc->Format == DXGI_FORMAT_D32_FLOAT) {
			info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.PlaneSlice = 0;
		srv_desc.Texture2D.ResourceMinLODClamp = 0.f;

		assert(!info.srv_desc && !info.resource);
		info.srv_desc = &srv_desc;
		_texture = D3D12Texture(info);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		dsv_desc.Format = dsv_format;
		dsv_desc.Texture2D.MipSlice = 0;

		_dsv = Core::DSV_Heap().Alloc();

		auto* const device{ Core::Device() };
		assert(device);
		device->CreateDepthStencilView(Resource(), &dsv_desc, _dsv.cpu);
	}

	void D3D12DepthBuffer::Release() {
		Core::DSV_Heap().Free(_dsv);
		_texture.Release();
	}

#pragma endregion


}

