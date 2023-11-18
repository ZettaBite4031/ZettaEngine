#pragma once
#include "D3D12CommonHeaders.h"
#include <mutex>

namespace Zetta::Graphics::D3D12{
	class DescriptorHeap;

	struct DescriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE	cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE	gpu{};
		u32	index{ u32_invalid_id };

		[[nodiscard]] constexpr bool IsValid() const { return cpu.ptr != 0; }
		[[nodiscard]] constexpr bool IsShaderVisible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
	private:
		friend class DescriptorHeap;
		DescriptorHeap* container{ nullptr };
#endif
	};

	class DescriptorHeap {
	public:
		explicit DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type{ type } { }
		DISABLE_COPY_AND_MOVE(DescriptorHeap);
		~DescriptorHeap() { assert(!_heap); }

		bool Initialize(u32 capacity, bool is_shader_visible);
		void ProcessDeferredFree(u32 frame_idx);
		void Release();

		[[nodiscard]] DescriptorHandle Alloc();
		void Free(DescriptorHandle& handle);

		[[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE Type() const { return _type; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE CPU_Start() const { return _cpu_start; }
		[[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE GPU_Start() const { return _gpu_start; }
		[[nodiscard]] constexpr ID3D12DescriptorHeap* const Heap() const { return _heap; }
		[[nodiscard]] constexpr u32 Capacity() const { return _capacity; }
		[[nodiscard]] constexpr u32 DescriptorSize() const { return _descriptor_size; }
		[[nodiscard]] constexpr bool IsShaderVisible() const { return _gpu_start.ptr != 0; }

	private:
		ID3D12DescriptorHeap*			_heap;
		D3D12_CPU_DESCRIPTOR_HANDLE		_cpu_start{};
		D3D12_GPU_DESCRIPTOR_HANDLE		_gpu_start{};
		std::unique_ptr<u32[]>			_free_handles{};
		util::vector<u32>				_deferred_free_indices[FrameBufferCount]{};
		std::mutex						_mutex{};
		u32								_capacity{ 0 };
		u32								_size{ 0 };
		u32								_descriptor_size{};
		const D3D12_DESCRIPTOR_HEAP_TYPE _type{};
	};

	struct D3D12TextureInitInfo {
		ID3D12Heap1* heap{ nullptr };
		ID3D12Resource* resource{ nullptr };
		D3D12_SHADER_RESOURCE_VIEW_DESC* srv_desc{ nullptr };
		D3D12_RESOURCE_DESC* desc{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1 allocation_info{};
		D3D12_RESOURCE_STATES initial_state{};
		D3D12_CLEAR_VALUE clear_value{};
	};

	class D3D12Texture {
	public:
		constexpr static u32 max_mips{ 14 };

		D3D12Texture() = default;
		explicit D3D12Texture(D3D12TextureInitInfo info);
		DISABLE_COPY(D3D12Texture);

		constexpr D3D12Texture(D3D12Texture&& o)
			: _resource{ o._resource }, _srv{ o._srv }
		{
			o.Reset();
		}

		constexpr D3D12Texture& operator=(D3D12Texture&& o) {
			assert(this != &o);
			if (this != &o) {
				Release();
				Move(o);
			}
			return *this;
		}

		~D3D12Texture() { Release(); }

		void Release();

		[[nodiscard]] constexpr ID3D12Resource* const Resource() const { return _resource; }
		[[nodiscard]] constexpr DescriptorHandle SRV() const { return _srv; }

	private:
		constexpr void Move(D3D12Texture& o) {
			_resource = o._resource;
			_srv = o._srv;
			o.Reset();
		}
		constexpr void Reset() {
			_resource = nullptr;
			_srv = {};
		}


		ID3D12Resource* _resource{ nullptr };
		DescriptorHandle _srv;
	};

	class D3D12RenderTexture {
	public:
		D3D12RenderTexture() = default;
		explicit D3D12RenderTexture(D3D12TextureInitInfo info);
		DISABLE_COPY(D3D12RenderTexture);
		constexpr D3D12RenderTexture(D3D12RenderTexture&& o)
			: _texture{ std::move(o._texture) }, _mip_count{ o._mip_count } {
			for (u32 i{ 0 }; i < _mip_count; i++) _rtv[i] = o._rtv[i];
			o.Reset();
		}

		constexpr D3D12RenderTexture& operator=(D3D12RenderTexture&& o) {
			assert(this != &o);
			if (this != &o) {
				Release();
				Move(o);
			}
			return *this;
		}

		~D3D12RenderTexture() { Release(); }

		void Release();
		[[nodiscard]] constexpr u32 Mipcount() const { return _mip_count; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE RTV(u32 i) const { assert(i < _mip_count); return _rtv[i].cpu; }
		[[nodiscard]] constexpr DescriptorHandle SRV() const { return _texture.SRV(); }
		[[nodiscard]] constexpr ID3D12Resource* const Resource() const { return _texture.Resource(); }

	private:
		constexpr void Move(D3D12RenderTexture& o) {
			_texture = std::move(o._texture);
			_mip_count = o._mip_count;
			for (u32 i{ 0 }; i < _mip_count; i++) _rtv[i] = o._rtv[i];
			o.Reset();
		}

		constexpr void Reset() {
			for (u32 i{ 0 }; i < _mip_count; i++) _rtv[i] = {};
			_mip_count = 0;
		}

		D3D12Texture _texture{};
		DescriptorHandle _rtv[D3D12Texture::max_mips]{};
		u32 _mip_count{ 0 };
	};

	class D3D12DepthBuffer {
	public:
		D3D12DepthBuffer() = default;
		explicit D3D12DepthBuffer(D3D12TextureInitInfo info);
		DISABLE_COPY(D3D12DepthBuffer);
		constexpr D3D12DepthBuffer(D3D12DepthBuffer&& o)
			: _texture{ std::move(o._texture) }, _dsv{ o._dsv } {
			o._dsv = {};
		}

		constexpr D3D12DepthBuffer& operator=(D3D12DepthBuffer&& o) {
			assert(this != &o);
			if (this != &o) {
				_texture = std::move(o._texture);
				_dsv = o._dsv;
				o._dsv = {};
			}
			return *this;
		}

		~D3D12DepthBuffer() { Release(); }

		void Release();
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE DSV() const { return _dsv.cpu; }
		[[nodiscard]] constexpr DescriptorHandle SRV() const { return _texture.SRV(); }
		[[nodiscard]] constexpr ID3D12Resource* const Resource() const { return _texture.Resource(); }

	private:
		D3D12Texture _texture{};
		DescriptorHandle _dsv{};
	};
}
