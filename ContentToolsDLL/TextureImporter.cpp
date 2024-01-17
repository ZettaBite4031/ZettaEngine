#include "ToolsCommon.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"
#include <DirectXTex.h>
#include <dxgi1_6.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace Zetta::Tools {
	bool IsNormalMap(const Image* const image);

	namespace {
		struct ImportError {
			enum ErrorCode : u32 {
				Success = 0,
				Unknown,
				Compress,
				Decompress,
				Load,
				MipmapGeneration,
				MaxSizeExceeded,
				SizeMismatch,
				FormatMismatch,
				FileNotFound,
			};
		};

		struct TextureDimension {
			enum Dimensions : u32 {
				Texture1D,
				Texture2D,
				Texture3D,
				TextureCube
			};
		};

		struct TextureImportSettings {
			char*					sources;		// string of one or more file paths separated by semi-colons ';'
			u32						source_count;	// number of file paths
			u32						dimension;
			u32						mip_levels;
			f32						alpha_threshold;
			u32						prefer_bc7;
			u32						output_format;
			u32						compress;
		};

		struct TextureInfo {
			u32						width;
			u32						height;
			u32						array_size;
			u32						mip_levels;
			u32						format;
			u32						import_error;
			u32						flags;
		};

		struct TextureData {
			constexpr static u32	max_mips{ 14 }; // up to 8k textures
			u8*						subresource_data;
			u32						subresource_size;
			u8*						icon;
			u32						icon_size;
			TextureInfo				info;
			TextureImportSettings	import_settings;
		};

		struct D3D11_Device {
			ComPtr<ID3D11Device>	device;
			std::mutex				hw_compression_mutex;
		};

		std::mutex					device_creation_mutex;
		util::vector<D3D11_Device>  d3d11_devices;

		util::vector<ComPtr<IDXGIAdapter>> GetAdaptersByPerformance() {
			using PFN_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);
			static PFN_CreateDXGIFactory1 create_dxgi_factory1{ nullptr };
			if (!create_dxgi_factory1) {
				HMODULE dxgi_module{ LoadLibrary(L"dxgi.dll") };
				if (!dxgi_module) return {};

				create_dxgi_factory1 = (PFN_CreateDXGIFactory1)((void*)GetProcAddress(dxgi_module, "CreateDXGIFactory1"));
				if (!create_dxgi_factory1) return {};
			}

			ComPtr<IDXGIFactory7> factory;
			util::vector<ComPtr<IDXGIAdapter>> adapters;
			if (SUCCEEDED(create_dxgi_factory1(IID_PPV_ARGS(factory.GetAddressOf())))) {
				constexpr u32 Warp_ID{ 0x1414 }; // Software emulator

				ComPtr<IDXGIAdapter> adapter;
				for (u32 i{ 0 }; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++) {
					if (!adapter) continue;
					DXGI_ADAPTER_DESC desc;
					adapter->GetDesc(&desc);
					if (desc.VendorId != Warp_ID) adapters.emplace_back(adapter);
					adapter.Reset();
				}
			}
			
			return adapters;
		}

		void CreateDevice() {
			if (d3d11_devices.size()) return;

			util::vector<ComPtr<IDXGIAdapter>> adapters{ GetAdaptersByPerformance() };

			static PFN_D3D11_CREATE_DEVICE d3d11_create_device{ nullptr };
			if (!d3d11_create_device) {
				HMODULE d3d11_module{ LoadLibrary(L"d3d11.dll") };
				if (!d3d11_module) return;

				d3d11_create_device = (PFN_D3D11_CREATE_DEVICE)((void*)GetProcAddress(d3d11_module, "D3D11CreateDevice"));
				if (!d3d11_create_device) return;
			}

			u32 create_device_flags{ 0 };
#ifdef _DEBUG
			create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			util::vector<ComPtr<ID3D11Device>> devices(adapters.size(), nullptr);
			constexpr D3D_FEATURE_LEVEL feature_levels[]{ D3D_FEATURE_LEVEL_11_0 };

			for (u32 i{ 0 }; i < adapters.size(); i++) {
				ID3D11Device** device{ &devices[i] };
				D3D_FEATURE_LEVEL feature_level;
				[[maybe_unused]] 
				HRESULT hr{ d3d11_create_device(adapters[i].Get(), adapters[i] ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, 
					nullptr, create_device_flags, feature_levels, _countof(feature_levels), 
					D3D11_SDK_VERSION, device, &feature_level, nullptr) };
				assert(SUCCEEDED(hr));
			}

			for (u32 i{ 0 }; i < devices.size(); i++) {
				// NOTE: Check for valid devices as device creation may fail for adapters that don't support
				//       The requested feaeture level (D3D_FEATURE_LEVEL_11_0).
				if (devices[i]) {
					d3d11_devices.emplace_back();
					d3d11_devices.back().device = devices[i];
				}
			}
		}

		constexpr u32 GetMaxMipCount(u32 width, u32 height, u32 depth) {
			u32 mip_levels{ 1 };
			while (width > 1 || height > 1 || depth > 1) {
				width >>= 1;
				height >>= 1;
				depth >>= 1;
				mip_levels++;
			}
			return mip_levels;
		}

		[[nodiscard]] ScratchImage LoadFromFile(TextureData* const data, const char* file_name) {
			using namespace Zetta::Content;

			assert(FileExists(file_name));
			if (!FileExists(file_name)) {
				data->info.import_error = ImportError::FileNotFound;
				return {};
			}

			data->info.import_error = ImportError::Load;

			WIC_FLAGS wic_flags{ WIC_FLAGS_NONE };
			TGA_FLAGS tga_flags{ TGA_FLAGS_NONE };

			if (data->import_settings.output_format == DXGI_FORMAT_BC4_UNORM ||
				data->import_settings.output_format == DXGI_FORMAT_BC5_UNORM) {
				wic_flags |= WIC_FLAGS_IGNORE_SRGB;
				tga_flags |= TGA_FLAGS_IGNORE_SRGB;
			}

			const std::wstring wfile{ ToWString(file_name) };
			const wchar_t* const file{ wfile.c_str() };
			ScratchImage scratch;

			wic_flags |= WIC_FLAGS_FORCE_RGB;
			HRESULT hr{ LoadFromWICFile(file, wic_flags, nullptr, scratch) };

			// File wasn't WIC format. Attempting TGA.
			if (FAILED(hr)) hr = LoadFromTGAFile(file, tga_flags, nullptr, scratch);

			// File Wasn't TGA or WIC format. Attempting HDR.
			if (FAILED(hr)) {
				hr = LoadFromHDRFile(file, nullptr, scratch);
				if (SUCCEEDED(hr)) data->info.flags |= TextureFlags::IS_HDR;
			}
			// File wasn't any of the above formats. Try DDS
			if (FAILED(hr)) {
				hr = LoadFromDDSFile(file, DDS_FLAGS_FORCE_RGB, nullptr, scratch);
				if (SUCCEEDED(hr)) {
					data->info.import_error = ImportError::Decompress;
					ScratchImage mip_scratch;
					hr = Decompress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), DXGI_FORMAT_UNKNOWN, mip_scratch);
					if (SUCCEEDED(hr)) {
						scratch = std::move(mip_scratch);
					}
				}
			}

			if (SUCCEEDED(hr)) data->info.import_error = ImportError::Success;

			return scratch;
		}

		[[nodiscard]] ScratchImage InitializeFromImages(TextureData* const data, const util::vector<Image>& images) {
			assert(data);
			const TextureImportSettings& settings{ data->import_settings };

			ScratchImage scratch;
			HRESULT hr{ S_OK };
			const u32 array_size{ (u32)images.size() };

			{
				ScratchImage working_scratch{};

				if (settings.dimension == TextureDimension::Texture1D ||
					settings.dimension == TextureDimension::Texture2D) {
					const bool allow_1D{ settings.dimension == TextureDimension::Texture1D };
					assert(array_size >= 1 && images.size() >= 1);
					hr = working_scratch.InitializeArrayFromImages(images.data(), images.size(), allow_1D);
				}
				else if (settings.dimension == TextureDimension::TextureCube) {
					assert(array_size % 6 == 0);
					hr = working_scratch.InitializeCubeFromImages(images.data(), array_size);
				}
				else {
					assert(settings.dimension == TextureDimension::Texture3D);
					hr = working_scratch.Initialize3DFromImages(images.data(), images.size());
				}

				if (FAILED(hr)) {
					data->info.import_error = ImportError::Unknown;
					return{};
				}

				scratch = std::move(working_scratch);
			}

			if (settings.mip_levels != 1) {
				ScratchImage mip_scratch;
				const TexMetadata& metadata{ scratch.GetMetadata() };
				u32 mip_levels{ Math::clamp(settings.mip_levels, (u32)0, GetMaxMipCount((u32)metadata.width, (u32)metadata.height, (u32)metadata.depth)) };

				if (settings.dimension != TextureDimension::Texture3D) {
					hr = GenerateMipMaps(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(),
						TEX_FILTER_DEFAULT, mip_levels, mip_scratch);
				}

				else {
					hr = GenerateMipMaps3D(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(),
						TEX_FILTER_DEFAULT, mip_levels, mip_scratch);
				}

				if (FAILED(hr)) {
					data->info.import_error = ImportError::MipmapGeneration;
					return {};
				}

				scratch = std::move(mip_scratch);
			}

			return scratch;
		}

		void CopyIcon(const Image& bc_image, TextureData* const data) {
			ScratchImage scratch;
			if (FAILED(Decompress(bc_image, DXGI_FORMAT_UNKNOWN, scratch))) return;
			const Image& image{ scratch.GetImages()[0] };

			// 4 x u32 for width, height, rowPitch and SlicePitch
			data->icon_size = (u32)(sizeof(u32) * 4 + image.slicePitch);
			data->icon = (u8* const)CoTaskMemRealloc(data->icon, data->icon_size);
			assert(data->icon);

			util::BlobStreamWriter blob{ data->icon, data->icon_size };
			blob.write((u32)image.width);
			blob.write((u32)image.height);
			blob.write((u32)image.rowPitch);
			blob.write((u32)image.slicePitch);
			blob.write(image.pixels, image.slicePitch);
		}

		void CopySubresources(const ScratchImage& scratch, TextureData* const data) {
			const TexMetadata& metadata{ scratch.GetMetadata() };
			const Image* const images{ scratch.GetImages() };
			const u32 image_count{ (u32)scratch.GetImageCount() };
			assert(images && metadata.mipLevels && metadata.mipLevels <= TextureData::max_mips);

			u64 subresource_size{ 0 };
			for (u32 i{ 0 }; i < image_count; i++) {
				// 4 x u32 for width, height, rowPitch and SlicePitch
				subresource_size += sizeof(u32) * 4 + images[i].slicePitch;
			}
			if (subresource_size > ~(u32)0) {
				// Supports up to 4GB per resource. If someone hits this... dear god
				data->info.import_error = ImportError::MaxSizeExceeded;
			}

			data->subresource_size = (u32)subresource_size;
			data->subresource_data = (u8* const)CoTaskMemRealloc(data->subresource_data, subresource_size);
			assert(data->subresource_data);

			util::BlobStreamWriter blob{ data->subresource_data, data->subresource_size };
			for (u32 i{ 0 }; i < image_count; i++) {
				const Image& image{ images[i] };
				blob.write((u32)image.width);
				blob.write((u32)image.height);
				blob.write((u32)image.rowPitch);
				blob.write((u32)image.slicePitch);
				blob.write(image.pixels, image.slicePitch);
			}
		}

		constexpr void SetOrClearFlag(u32& flags, u32 flag, bool set) {
			if (set) flags |= flag;
			else flags &= ~flag;
		}

		void TextureInfoFromMetadata(const TexMetadata& metadata, TextureInfo& info) {
			using namespace Zetta::Content;

			const DXGI_FORMAT format{ metadata.format };
			info.width = (u32)metadata.width;
			info.height = (u32)metadata.height;
			info.array_size = metadata.IsVolumemap() ? (u32)metadata.depth : (u32)metadata.arraySize;
			info.mip_levels = (u32)metadata.mipLevels;
			SetOrClearFlag(info.flags, TextureFlags::HAS_ALPHA, HasAlpha(format));
			SetOrClearFlag(info.flags, TextureFlags::IS_HDR, format == DXGI_FORMAT_BC6H_UF16 || format == DXGI_FORMAT_BC6H_SF16);
			SetOrClearFlag(info.flags, TextureFlags::IS_PREMULTIPLIED_ALPHA, metadata.IsPMAlpha());
			SetOrClearFlag(info.flags, TextureFlags::IS_CUBE_MAP, metadata.IsCubemap());
			SetOrClearFlag(info.flags, TextureFlags::IS_VOLUME_MAP, metadata.IsVolumemap());
		}

		DXGI_FORMAT DetermineOutputFormat(TextureData *const data, ScratchImage& scratch, const Image* const image) {
			assert(data && data->import_settings.compress);
			using namespace Zetta::Content;
			const DXGI_FORMAT image_format{ image->format };
			DXGI_FORMAT output_format{ (DXGI_FORMAT)data->import_settings.output_format };

			if (output_format != DXGI_FORMAT_UNKNOWN) goto _done;
			if ((data->info.flags & TextureFlags::IS_HDR) || image_format == DXGI_FORMAT_BC6H_UF16 || image_format == DXGI_FORMAT_BC6H_SF16) output_format = DXGI_FORMAT_BC6H_UF16;
			else if (image_format == DXGI_FORMAT_R8_UNORM || image_format == DXGI_FORMAT_BC4_UNORM || image_format == DXGI_FORMAT_BC4_SNORM) output_format = DXGI_FORMAT_BC4_UNORM;
			else if (IsNormalMap(image) || image_format == DXGI_FORMAT_BC5_UNORM || image_format == DXGI_FORMAT_BC5_SNORM) {
				data->info.flags |= TextureFlags::IS_IMPORTED_AS_NORMAL_MAP;
				output_format = DXGI_FORMAT_BC5_UNORM;
				if (IsSRGB(image_format)) scratch.OverrideFormat(MakeTypelessUNORM(MakeTypeless(image_format)));
			}
			else output_format = data->import_settings.prefer_bc7 ? DXGI_FORMAT_BC7_UNORM : 
				scratch.IsAlphaAllOpaque() ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC3_UNORM;

		_done:
			assert(IsCompressed(output_format));
			if (HasAlpha(output_format)) data->info.flags |= TextureFlags::HAS_ALPHA;

			return IsSRGB(image_format) ? MakeSRGB(output_format) : output_format;
		}



		bool CanUseGPU(DXGI_FORMAT format) {
			switch (format) {
			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
			{
				std::lock_guard lock{ device_creation_mutex };
				static bool try_once = false;
				if (!try_once) {
					try_once = true;
					CreateDevice();
				}
				return d3d11_devices.size() > 0;
			}
			}
			return false;
		}

		[[nodiscard]] ScratchImage CompressImage(TextureData* const data, ScratchImage& scratch) {
			assert(data && data->import_settings.compress && scratch.GetImages());

			const Image* const image{ scratch.GetImage(0, 0, 0) };
			if (!image) {
				data->info.import_error = ImportError::Unknown;
				return {};
			}

			const DXGI_FORMAT output_format{ DetermineOutputFormat(data, scratch, image) };
			HRESULT hr{ S_OK };
			ScratchImage bc_scratch;
			if (CanUseGPU(output_format)) {
				bool wait{ true };
				while (wait) {
					for (u32 i{ 0 }; i < d3d11_devices.size(); i++) {
						if (d3d11_devices[i].hw_compression_mutex.try_lock()) {
							hr = Compress(d3d11_devices[i].device.Get(), scratch.GetImages(), scratch.GetImageCount(),
										  scratch.GetMetadata(), output_format, TEX_COMPRESS_DEFAULT, 1.0f, bc_scratch);
							d3d11_devices[i].hw_compression_mutex.unlock();
							wait = false;
							break;
						}
					}
					if (wait) std::this_thread::sleep_for(std::chrono::milliseconds(250));
				}
			}
			else hr = Compress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(),
					output_format, TEX_COMPRESS_PARALLEL, data->import_settings.alpha_threshold, bc_scratch);

			if (FAILED(hr)) {
				data->info.import_error = ImportError::Compress;
				return {};
			}

			return bc_scratch;
		}

		[[nodiscard]] util::vector<Image> SubresourceDataToImages(TextureData* const data) {
			assert(data && data->subresource_data && data->subresource_size);
			assert(data->info.mip_levels && data->info.mip_levels <= TextureData::max_mips);
			assert(data->info.array_size);

			const TextureInfo& info{ data->info };
			u32 image_count{ info.array_size };

			if (info.flags & Content::TextureFlags::IS_VOLUME_MAP) {
				u32 depth_per_mip_level{ info.array_size };
				for (u32 i{ 1 }; i < info.mip_levels; i++) {
					depth_per_mip_level = std::max(depth_per_mip_level >> 1, (u32)1);
					image_count += depth_per_mip_level;
				}
			}
			else {
				image_count *= info.mip_levels;
			}

			util::BlobStreamReader blob{ data->subresource_data };
			util::vector<Image> images(image_count);

			for (u32 i{ 0 }; i < image_count; i++) {
				Image image{};
				image.width = blob.read<u32>();
				image.height = blob.read<u32>();
				image.format = (DXGI_FORMAT)info.format;
				image.rowPitch = blob.read<u32>();
				image.slicePitch = blob.read<u32>();
				image.pixels = (u8*)blob.Position();

				blob.skip(image.slicePitch);

				images[i] = image;
			}

			return images;
		}
	}

	void ShutdownTextureTools() {
		d3d11_devices.clear();
	}

	EDITOR_INTERFACE void Decompress(TextureData *const data) {
		using namespace Zetta::Content;
		assert(data->import_settings.compress);
		TextureInfo& info{ data->info };
		const DXGI_FORMAT format{ (DXGI_FORMAT)info.format };
		assert(IsCompressed(format));
		util::vector<Image> images = SubresourceDataToImages(data);
		const bool is_3D{ (info.flags & TextureFlags::IS_VOLUME_MAP) != 0 };

		TexMetadata metadata{};
		metadata.width = info.width;
		metadata.height = info.height;
		metadata.depth = is_3D ? info.array_size : 1;
		metadata.arraySize = is_3D ? 1 : info.array_size;
		metadata.mipLevels = info.mip_levels;
		metadata.miscFlags = info.flags & TextureFlags::IS_CUBE_MAP ? TEX_MISC_TEXTURECUBE : 0;
		metadata.miscFlags2 = info.flags & TextureFlags::IS_PREMULTIPLIED_ALPHA ? TEX_ALPHA_MODE_PREMULTIPLIED :
			info.flags & TextureFlags::HAS_ALPHA ? TEX_ALPHA_MODE_STRAIGHT : TEX_ALPHA_MODE_OPAQUE;
		metadata.format = format;
		metadata.dimension = is_3D ? TEX_DIMENSION_TEXTURE3D : TEX_DIMENSION_TEXTURE2D;

		ScratchImage scratch;
		HRESULT hr{ Decompress(images.data(), (size_t)images.size(), metadata, DXGI_FORMAT_UNKNOWN, scratch) };
		if (SUCCEEDED(hr)) {
			CopySubresources(scratch, data);
			TextureInfoFromMetadata(scratch.GetMetadata(), data->info);
		}
		else {
			info.import_error = ImportError::Decompress;
		}
	}

	EDITOR_INTERFACE void Import(TextureData *const data) {
		const TextureImportSettings& settings{ data->import_settings };
		assert(settings.sources && settings.source_count);

		util::vector<ScratchImage> scratch_images;
		util::vector<Image> images;

		u32 width{ 0 };
		u32 height{ 0 };
		DXGI_FORMAT format{};
		util::vector<std::string> files = split(settings.sources, ';');
		assert(files.size() == settings.source_count);

		for (u32 i{ 0 }; i < settings.source_count; i++) {
			scratch_images.emplace_back(LoadFromFile(data, files[i].c_str()));
			if (data->info.import_error) return;

			const ScratchImage& scratch{ scratch_images.back() };
			const TexMetadata& metadata{ scratch.GetMetadata() };

			if (i == 0) {
				width = (u32)metadata.width;
				height = (u32)metadata.height;
				format = metadata.format;
			}

			if (width != metadata.width || height != metadata.height) {
				data->info.import_error = ImportError::SizeMismatch;
				return;
			}

			if (format != metadata.format) {
				data->info.import_error = ImportError::FormatMismatch;
				return;
			}

			const u32 array_size{ (u32)metadata.arraySize };
			const u32 depth{ (u32)metadata.depth };

			for (u32 array_index{ 0 }; array_index < array_size; array_index++) {
				for (u32 depth_index{ 0 }; depth_index < depth; depth_index++) {
					const Image* image{ scratch.GetImage(0, array_index, depth_index) };
					assert(image);
					if (!image) {
						data->info.import_error = ImportError::Unknown;
						return;
					}

					if (width != image->width || height != image->height) {
						data->info.import_error = ImportError::SizeMismatch;
						return;
					}

					images.emplace_back(*image);
				}
			}
		}

		ScratchImage scratch{ InitializeFromImages(data, images) };
		if (data->info.import_error) return;

		if (settings.compress) {
			// NOTE: Copy the first uncompressed image for the editor to generate an icon.
			//		 Only do this for compressed imports. if not compressed, the editor
			//		 Will pick the first image from the returned subresources.

			ScratchImage bc_scratch{ /* TODO: compress image */};
			if (data->info.import_error) return;

			assert(bc_scratch.GetImages());
			CopyIcon(bc_scratch.GetImages()[0], data);

			scratch = std::move(bc_scratch);
		}
		CopySubresources(scratch, data);
		TextureInfoFromMetadata(scratch.GetMetadata(), data->info);
	}
}