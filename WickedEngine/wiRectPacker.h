#pragma once
#include "CommonInclude.h"

#include <vector>

// NOTE: 
// This is based on the rectpack2D library hosted here: https://github.com/TeamHypersomnia/rectpack2D

/* of your interest:

1. rect_xywh - structure representing your rectangle object
members:
int x, y, w, h;

2. bin - structure representing resultant bin object
3. bool pack(rect_xywh* const * v, int n, int max_side, std::std::vector<bin>& bins) - actual packing function
Arguments:
input/output: v - pointer to array of pointers to your rectangles (const here means that the pointers will point to the same rectangles after the call)
input: n - rectangles count

input: max_side - maximum bins' side - algorithm works with square bins (in the end it may trim them to rectangular form).
for the algorithm to finish faster, pass a reasonable value (unreasonable would be passing 1 000 000 000 for packing 4 50x50 rectangles).
output: bins - vector to which the function will push_back() created bins, each of them containing vector to pointers of rectangles from "v" belonging to that particular bin.
Every bin also keeps information about its width and height of course, none of the dimensions is bigger than max_side.

returns true on success, false if one of the rectangles' dimension was bigger than max_side

You want to your rectangles representing your textures/glyph objects with GL_MAX_TEXTURE_SIZE as max_side,
then for each bin iterate through its rectangles, typecast each one to your own structure (or manually add userdata) and then memcpy its pixel contents (rotated by 90 degrees if "flipped" rect_xywh's member is true)
to the array representing your texture atlas to the place specified by the rectangle, then finally upload it with glTexImage2D.

Algorithm doesn't create any new rectangles.
You just pass an array of pointers - rectangles' x/y/w/h are modified in place.
There is a vector of pointers for every resultant bin to let you know which ones belong to the particular bin.

For description how to tune the algorithm and how it actually works see the .cpp file.


*/

namespace wiRectPacker
{

	struct rect_ltrb;
	struct rect_xywh;

	struct rect_wh {
		rect_wh(const rect_ltrb&);
		rect_wh(const rect_xywh&);
		rect_wh(int w = 0, int h = 0);
		int w, h, area(), perimeter(),
			fits(const rect_wh& bigger) const; // 0 - no, 1 - yes, 2 - perfectly
	};

	// rectangle implementing left/top/right/bottom behaviour

	struct rect_ltrb {
		rect_ltrb();
		rect_ltrb(int left, int top, int right, int bottom);
		int l, t, r, b, w() const, h() const, area() const, perimeter() const;
		void w(int), h(int);
	};

	struct rect_xywh : public rect_wh {
		rect_xywh();
		rect_xywh(const rect_ltrb&);
		rect_xywh(int x, int y, int width, int height);
		operator rect_ltrb();

		int x, y, r() const, b() const;
		void r(int), b(int);
	};


	struct bin {
		rect_wh size;
		std::vector<rect_xywh*> rects;
	};

	bool pack(rect_xywh* const * v, int n, int max_side, std::vector<bin>& bins);

}
