#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Upload {
	class D3D12UploadContext {
	public:
		DISABLE_COPY_AND_MOVE(D3D12UploadContext);
		D3D12UploadContext(u32 aligned_size);
		~D3D12UploadContext() { assert(_frame_index == u32_invalid_id); }
		
		void EndUpload();

		[[nodiscard]] constexpr ID3D12GraphicsCommandList* const CommandList() const { return _cmd_list; }
		[[nodiscard]] constexpr ID3D12Resource* const UploadBuffer() const { return _upload_buffer; }
		[[nodiscard]] constexpr void* const CPUAddress() const { return _cpu_addr; }

	private:
#ifdef _DEBUG
		D3D12UploadContext() = default;
#endif
		ID3D12GraphicsCommandList* _cmd_list{ nullptr };
		ID3D12Resource* _upload_buffer{ nullptr };
		void* _cpu_addr{ nullptr };
		u32 _frame_index{ u32_invalid_id };
	};

	bool Initialize();
	void Shutdown();
}
