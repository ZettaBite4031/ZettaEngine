#include "ToolsCommon.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"
#include <DirectXTex.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace Zetta::Tools {
	namespace {
		constexpr f32 INV_255{ 1.f / 255.f };
		constexpr f32 MIN_AVG_LENGTH_THRESHOLD{ 0.7f };
		constexpr f32 MAX_AVG_LENGTH_THRESHOLD{ 1.1f };
		constexpr f32 MIN_AVG_Z_THRESHOLD{ 0.8f };
		constexpr f32 VECTOR_LENGTH_SQ_REJECTION_THRESHOLD{ MIN_AVG_LENGTH_THRESHOLD * MIN_AVG_LENGTH_THRESHOLD };
		constexpr f32 REJECTION_RATIO_THRESHOLD{ 0.33f };

		struct Color {
			f32 r, g, b, a;
			bool IsTransparent() const { return a < 0.001f; }
			bool IsBlack() const { return r < 0.001f && g < 0.001f && b < 0.001f; }

			Color operator+(Color c) {
				r += c.r; g += c.g; b += c.b; a += c.a;
				return *this;
			}
			Color operator+=(Color c) { return (*this) + c; }

			Color operator*(f32 s) {
				r *= s; g *= s; b *= s;
				return *this;
			}

			Color operator*=(f32 s) { return (*this) * s; }
			Color operator/=(f32 s) { return (*this) * (1.f/s); }
		};

		using sampler = Color(*)(const u8 *const);

		Color SamplePixelRGB(const u8* const pixel) {
			Color c{ (f32)pixel[0], (f32)pixel[1], (f32)pixel[2], (f32)pixel[3] };
			return c * INV_255;
		}

		Color SamplePixelBGR(const u8* const pixel) {
			Color c{ (f32)pixel[2], (f32)pixel[1], (f32)pixel[1], (f32)pixel[3] };
			return c * INV_255;
		}

		s32 EvaluateColor(Color c) {
			if (c.IsBlack() || c.IsTransparent()) return 0;

			Math::v3 v{ c.r * 2.f - 1.f, c.g * 2.f - 1.f, c.b * 2.f - 1.f };
			const f32 v_length_sq{ v.x * v.x + v.y * v.y + v.z * v.z };

			return (v.z < 0.f || v_length_sq < VECTOR_LENGTH_SQ_REJECTION_THRESHOLD) ? -1 : 1;
		}

		bool EvaluateImage(const Image* const image, sampler sample) {
			constexpr u32 sample_count{ 4096 };
			const size_t image_size{ image->slicePitch };
			const size_t sample_interval{ std::max(image_size / sample_count, (size_t)4) };
			const u32 min_sample_count{ std::max((u32)(image_size / sample_interval) >> 2, (u32)1) };
			const u8* const pixels{ image->pixels };

			u32 accepted_samples{ 0 };
			u32 rejected_samples{ 0 };
			Color average_color{};

			size_t offset{ sample_interval };
			while (offset < image_size) {
				const Color c{ sample(&pixels[offset]) };
				const s32 result{ EvaluateColor(c) };
				if (result < 0) {
					rejected_samples++;
				}
				else if (result > 0) {
					accepted_samples++;
					average_color += c;
				}
				offset += sample_interval;
			}

			if (accepted_samples >= min_sample_count) {
				const f32 rejection_ratio{ (f32)rejected_samples / (f32)accepted_samples };
				if (rejection_ratio > REJECTION_RATIO_THRESHOLD) return false;
				average_color /= (f32)accepted_samples;
				Math::v3 v{ average_color.r * 2.f - 1.f, average_color.g * 2.f - 1.f, average_color.b * 2.f - 1.f };
				const f32 avg_length{ sqrt(v.x * v.x + v.y * v.y + v.z * v.z) };
				const f32 avg_normalized_z{ v.z / avg_length };

				return
					avg_length >= MIN_AVG_LENGTH_THRESHOLD &&
					avg_length <= MAX_AVG_LENGTH_THRESHOLD &&
					avg_normalized_z >= MIN_AVG_Z_THRESHOLD;
			}

			return false;
		}
	}

	bool IsNormalMap(const Image* const image) {
		const DXGI_FORMAT image_format{ image->format };
		if (BitsPerPixel(image_format) != 32 || BitsPerColor(image_format) != 8) return false;

		return EvaluateImage(image, IsBGR(image_format) ? SamplePixelBGR : SamplePixelRGB);
	}
}