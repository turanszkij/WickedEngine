#include "wiRectPacker.h"

#include <algorithm>

using namespace std;

namespace wiRectPacker
{

	bool area(rect_xywh* a, rect_xywh* b) {
		return a->area() > b->area();
	}

	bool perimeter(rect_xywh* a, rect_xywh* b) {
		return a->perimeter() > b->perimeter();
	}

	bool max_side(rect_xywh* a, rect_xywh* b) {
		return std::max(a->w, a->h) > std::max(b->w, b->h);
	}

	bool max_width(rect_xywh* a, rect_xywh* b) {
		return a->w > b->w;
	}

	bool max_height(rect_xywh* a, rect_xywh* b) {
		return a->h > b->h;
	}


	// just add another comparing function name to cmpf to perform another packing attempt
	// more functions == slower but probably more efficient cases covered and hence less area wasted

	bool(*cmpf[])(rect_xywh*, rect_xywh*) = {
		area,
		perimeter,
		max_side,
		max_width,
		max_height
	};

	// if you find the algorithm running too slow you may double this factor to increase speed but also decrease efficiency
	// 1 == most efficient, slowest
	// efficiency may be still satisfying at 64 or even 256 with nice speedup

	int discard_step = 128;

	/*

	For every sorting function, algorithm will perform packing attempts beginning with a bin with width and height equal to max_side,
	and decreasing its dimensions if it finds out that rectangles did actually fit, increasing otherwise.
	Although, it's doing that in sort of binary search manner, so for every comparing function it will perform at most log2(max_side) packing attempts looking for the smallest possible bin size.
	discard_step = 128 means that the algorithm will break of the searching loop if the rectangles fit but "it may be possible to fit them in a bin smaller by 128"
	the bigger the value, the sooner the algorithm will finish but the rectangles will be packed less tightly.
	use discard_step = 1 for maximum tightness.

	the algorithm was based on http://www.blackpawn.com/texts/lightmaps/default.html
	the algorithm reuses the node tree so it doesn't reallocate them between searching attempts

	*/

	/*************************************************************************** CHAOS BEGINS HERE */

	struct node {
		struct pnode {
			node* pn;
			bool fill;

			pnode() : fill(false), pn(0) {}
			void set(int l, int t, int r, int b) {
				if (!pn) pn = new node(rect_ltrb(l, t, r, b));
				else {
					(*pn).rc = rect_ltrb(l, t, r, b);
					(*pn).id = false;
				}
				fill = true;
			}
		};

		pnode c[2];
		rect_ltrb rc;
		bool id;
		node(rect_ltrb rc = rect_ltrb()) : id(false), rc(rc) {}

		void reset(const rect_wh& r) {
			id = false;
			rc = rect_ltrb(0, 0, r.w, r.h);
			delcheck();
		}

		node* insert(rect_xywh& img) {
			if (c[0].pn && c[0].fill) {
				node* newn;
				if (newn = c[0].pn->insert(img)) return newn;
				return    c[1].pn->insert(img);
			}

			if (id) return 0;
			int f = img.fits(rect_xywh(rc));

			switch (f) {
			case 0: return 0;
			case 1: break;
			case 2: id = true; return this;
			}

			int iw = img.w, ih = img.h;

			if (rc.w() - iw > rc.h() - ih) {
				c[0].set(rc.l, rc.t, rc.l + iw, rc.b);
				c[1].set(rc.l + iw, rc.t, rc.r, rc.b);
			}
			else {
				c[0].set(rc.l, rc.t, rc.r, rc.t + ih);
				c[1].set(rc.l, rc.t + ih, rc.r, rc.b);
			}

			return c[0].pn->insert(img);
		}

		void delcheck() {
			if (c[0].pn) { c[0].fill = false; c[0].pn->delcheck(); }
			if (c[1].pn) { c[1].fill = false; c[1].pn->delcheck(); }
		}

		~node() {
			delete c[0].pn;
			delete c[1].pn;
		}
	};

	rect_wh _rect2D(rect_xywh* const * v, int n, int max_s, std::vector<rect_xywh*>& succ, std::vector<rect_xywh*>& unsucc) {
		node root;

		const int funcs = (sizeof(cmpf) / sizeof(bool(*)(rect_xywh*, rect_xywh*)));

		rect_xywh** order[funcs];

		for (int f = 0; f < funcs; ++f) {
			order[f] = new rect_xywh*[n];
			memcpy(order[f], v, sizeof(rect_xywh*) * n);
			sort(order[f], order[f] + n, cmpf[f]);
		}

		rect_wh min_bin = rect_wh(max_s, max_s);
		int min_func = -1, best_func = 0, best_area = 0, _area = 0, step, fit, i;

		bool fail = false;

		for (int f = 0; f < funcs; ++f) {
			v = order[f];
			step = min_bin.w / 2;
			root.reset(min_bin);

			while (true) {
				if (root.rc.w() > min_bin.w) {
					if (min_func > -1) break;
					_area = 0;

					root.reset(min_bin);
					for (i = 0; i < n; ++i)
						if (root.insert(*v[i]))
							_area += v[i]->area();

					fail = true;
					break;
				}

				fit = -1;

				for (i = 0; i < n; ++i)
					if (!root.insert(*v[i])) {
						fit = 1;
						break;
					}

				if (fit == -1 && step <= discard_step)
					break;

				root.reset(rect_wh(root.rc.w() + fit*step, root.rc.h() + fit*step));

				step /= 2;
				if (!step)
					step = 1;
			}

			if (!fail && (min_bin.area() >= root.rc.area())) {
				min_bin = rect_wh(root.rc);
				min_func = f;
			}

			else if (fail && (_area > best_area)) {
				best_area = _area;
				best_func = f;
			}
			fail = false;
		}

		v = order[min_func == -1 ? best_func : min_func];

		int clip_x = 0, clip_y = 0;
		node* ret;

		root.reset(min_bin);

		for (i = 0; i < n; ++i) {
			if (ret = root.insert(*v[i])) {
				v[i]->x = ret->rc.l;
				v[i]->y = ret->rc.t;

				clip_x = std::max(clip_x, ret->rc.r);
				clip_y = std::max(clip_y, ret->rc.b);

				succ.push_back(v[i]);
			}
			else {
				unsucc.push_back(v[i]);
			}
		}

		for (int f = 0; f < funcs; ++f)
			delete[] order[f];

		return rect_wh(clip_x, clip_y);
	}


	bool pack(rect_xywh* const * v, int n, int max_s, std::vector<bin>& bins) {
		rect_wh _rect(max_s, max_s);

		for (int i = 0; i < n; ++i)
			if (!v[i]->fits(_rect)) return false;

		std::vector<rect_xywh*> vec[2], *p[2] = { vec, vec + 1 };
		vec[0].resize(n);
		vec[1].clear();
		memcpy(&vec[0][0], v, sizeof(rect_xywh*)*n);

		bin* b = 0;

		while (true) {
			bins.push_back(bin());
			b = &bins[bins.size() - 1];

			b->size = _rect2D(&((*p[0])[0]), static_cast<int>(p[0]->size()), max_s, b->rects, *p[1]);
			b->rects.shrink_to_fit();
			p[0]->clear();

			if (!p[1]->size()) break;

			std::swap(p[0], p[1]);
		}

		return true;
	}


	rect_wh::rect_wh(const rect_ltrb& rr) : w(rr.w()), h(rr.h()) {}
	rect_wh::rect_wh(const rect_xywh& rr) : w(rr.w), h(rr.h) {}
	rect_wh::rect_wh(int w, int h) : w(w), h(h) {}

	int rect_wh::fits(const rect_wh& r) const {
		if (w == r.w && h == r.h) return 2;
		if (w <= r.w && h <= r.h) return 1;
		return 0;
	}

	rect_ltrb::rect_ltrb() : l(0), t(0), r(0), b(0) {}
	rect_ltrb::rect_ltrb(int l, int t, int r, int b) : l(l), t(t), r(r), b(b) {}

	int rect_ltrb::w() const {
		return r - l;
	}

	int rect_ltrb::h() const {
		return b - t;
	}

	int rect_ltrb::area() const {
		return w()*h();
	}

	int rect_ltrb::perimeter() const {
		return 2 * w() + 2 * h();
	}

	void rect_ltrb::w(int ww) {
		r = l + ww;
	}

	void rect_ltrb::h(int hh) {
		b = t + hh;
	}

	rect_xywh::rect_xywh() : x(0), y(0) {}
	rect_xywh::rect_xywh(const rect_ltrb& rc) : x(rc.l), y(rc.t) { b(rc.b); r(rc.r); }
	rect_xywh::rect_xywh(int x, int y, int w, int h) : x(x), y(y), rect_wh(w, h) {}

	rect_xywh::operator rect_ltrb() {
		rect_ltrb rr(x, y, 0, 0);
		rr.w(w); rr.h(h);
		return rr;
	}

	int rect_xywh::r() const {
		return x + w;
	};

	int rect_xywh::b() const {
		return y + h;
	}

	void rect_xywh::r(int right) {
		w = right - x;
	}

	void rect_xywh::b(int bottom) {
		h = bottom - y;
	}

	int rect_wh::area() {
		return w*h;
	}

	int rect_wh::perimeter() {
		return 2 * w + 2 * h;
	}

}
