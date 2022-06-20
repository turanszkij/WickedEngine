#pragma once
#include "CommonInclude.h"
#include "wiVector.h"

#include "Utility/stb_rect_pack.h"

namespace wi::rectpacker
{
	using Rect = stbrp_rect;

	// Convenience state wrapper around stb_rect_pack
	//	This can be used to automatically grow backing memory for the packer,
	//	and also supports iteratively finding a minimal containing size when packing
	struct State
	{
		stbrp_context context = {};
		wi::vector<stbrp_node> nodes;
		wi::vector<Rect> rects;
		int width = 0;
		int height = 0;

		// Clears the rectangles to empty, but doesn't reset the containing width/height result
		void clear()
		{
			rects.clear();
		}

		// Add a new rect and also grow the minimum containing size
		void add_rect(const Rect& rect)
		{
			rects.push_back(rect);
			width = std::max(width, rect.w);
			height = std::max(height, rect.h);
		}

		// Performs packing of the rect array
		//	The rectangle offsets will be filled after this
		//	The width, height for minimal containing area will be filled after this if the packing is successful
		//	max_width : if the packing is unsuccessful above this, it will result in a failure
		//	returns true for success, false for failure
		bool pack(int max_width)
		{
			while (width <= max_width && height <= max_width)
			{
				if (nodes.size() < width)
				{
					nodes.resize(width);
				}
				stbrp_init_target(&context, width, height, nodes.data(), int(nodes.size()));
				if (stbrp_pack_rects(&context, rects.data(), int(rects.size())))
					return true;
				if (height < width)
				{
					height *= 2;
				}
				else
				{
					width *= 2;
				}
			}
			width = 0;
			height = 0;
			return false;
		}
	};
}
