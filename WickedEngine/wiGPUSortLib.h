#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

namespace wi::gpusortlib
{
	// Perform radix sort on a GPU dataset
	//	maxCount				-	Maximum size of the dataset. GPU count can be smaller (see: counterBuffer_read param)
	//	keyBuffer_uint_RW		-	Buffer containing uint32_t values to sort by (ReadWrite UNORDERED_ACCESS state)
	//	counterBuffer_RO		-	Buffer containing count of values to sort (Read Only SHADER_RESOURCE state)
	//	counterReadOffset		-	Byte offset into the counter buffer to read the uint32_t count value
	//	payloadBuffer_uint_RW	-	The payload buffer of uint32_t values which to sort [optional] (ReadWrite UNORDERED_ACCESS state)
	void Sort(
		uint32_t maxCount,
		const wi::graphics::GPUBuffer& keyBuffer_uint_RW,
		const wi::graphics::GPUBuffer& counterBuffer_RO,
		uint32_t counterReadOffset,
		const wi::graphics::GPUBuffer& payloadBuffer_uint_RW,
		wi::graphics::CommandList cmd
	);

	void Initialize();
};
