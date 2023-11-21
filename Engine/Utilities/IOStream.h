#pragma once

#include "CommonHeaders.h"

namespace Zetta::util {
	class BlobStreamReader {
	public:
		DISABLE_COPY_AND_MOVE(BlobStreamReader);
		explicit BlobStreamReader(const u8* buffer)
			: _buffer{ buffer }, _position{ buffer } {
			assert(buffer);
		}

		template<typename T>
		[[nodiscard]] T read() {
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");
			T v{ *((T*)_position) };
			_position += sizeof(T);
			return v;
		}

		void read(u8* buffer, size_t len) {
			memcpy(buffer, _position, len);
			_position += len;
		}

		void skip(size_t offset) {
			_position += offset;
		}

		[[nodiscard]] constexpr const u8* const BufferStart() const { return _buffer; }
		[[nodiscard]] constexpr const u8* const Position() const { return _position; }
		[[nodiscard]] constexpr size_t Offset() const { return _position - _buffer; }

	private:
		const u8* const _buffer;
		const u8* _position;
	};

	class BlobStreamWriter {
	public:
		DISABLE_COPY_AND_MOVE(BlobStreamWriter);
		explicit BlobStreamWriter(u8* buffer, size_t buffer_size)
			: _buffer{ buffer }, _position{ buffer }, _size{ buffer_size } {
			assert(buffer && buffer_size);
		}

		template<typename T>
		void write(T v) {
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");
			assert(&_position[sizeof(T)] <= &_buffer[_size]);
			*((T*)_position) = v;
			_position += sizeof(T);
		}

		void write(const char* buffer, size_t len) {
			assert(&_position[len] <= &_buffer[_size]);
			memcpy(_position, buffer, len);
			_position += len;
		}

		void write(const u8* buffer, size_t len) {
			assert(&_position[len] <= &_buffer[_size]);
			memcpy(_position, buffer, len);
			_position += len;
		}

		void skip(size_t offset) {
			assert(&_position[offset] <= &_buffer[_size]);
			_position += offset;
		}

		[[nodiscard]] constexpr const u8* const BufferStart() const { return _buffer; }
		[[nodiscard]] constexpr const u8* const BufferEnd() const { return &_buffer[_size]; }
		[[nodiscard]] constexpr const u8* const Position() const { return _position; }
		[[nodiscard]] constexpr size_t Offset() const { return _position - _buffer; }

	private:
		u8* _buffer;
		u8* _position;
		size_t _size;
	};
}