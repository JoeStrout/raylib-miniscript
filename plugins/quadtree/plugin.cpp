//
//  plugins/quadtree/plugin.cpp
//
//  Example plugin: a point quadtree, for fast "what is near here?" queries.
//
//      t = quadtree.create(0, 0, 1000, 1000)    // bounds: x, y, width, height
//      quadtree.insert t, id, x, y           // id is any number you like
//      hits = quadtree.query(t, x, y, w, h)  // ids of points inside the rectangle
//      quadtree.count(t)
//      quadtree.clear t
//

#include "Plugin.h"
#include "RaylibTypes.h"   // NewNativeHandle / NativeHandlePtr
#include "macros.h"
#include <vector>

using namespace MiniScript;

namespace {

struct Pt { double id, x, y; };

struct Node {
	double x, y, w, h;
	int child[4];            // -1 until split
	std::vector<Pt> pts;
};

struct Quadtree {
	std::vector<Node> nodes;
	int count = 0;
	static const int kCapacity = 8;
	static const int kMaxDepth = 10;

	void Reset(double x, double y, double w, double h) {
		nodes.clear();
		count = 0;
		nodes.push_back(Node{x, y, w, h, {-1, -1, -1, -1}, {}});
	}

	static bool Inside(const Node& n, double px, double py) {
		return px >= n.x && px <= n.x + n.w && py >= n.y && py <= n.y + n.h;
	}

	int ChildFor(const Node& n, double px, double py) const {
		return (px >= n.x + n.w / 2 ? 1 : 0) + (py >= n.y + n.h / 2 ? 2 : 0);
	}

	void Split(int ni) {
		double hw = nodes[ni].w / 2, hh = nodes[ni].h / 2;
		double x = nodes[ni].x, y = nodes[ni].y;
		for (int c = 0; c < 4; c++) {
			int idx = (int)nodes.size();
			nodes.push_back(Node{x + (c & 1) * hw, y + (c >> 1) * hh, hw, hh, {-1, -1, -1, -1}, {}});
			nodes[ni].child[c] = idx;   // after push_back: the vector may have moved
		}
		std::vector<Pt> old;
		old.swap(nodes[ni].pts);
		for (const Pt& p : old) Place(nodes[ni].child[ChildFor(nodes[ni], p.x, p.y)], p, 1000);
	}

	void Place(int ni, const Pt& p, int depth) {
		while (nodes[ni].child[0] >= 0) ni = nodes[ni].child[ChildFor(nodes[ni], p.x, p.y)];
		nodes[ni].pts.push_back(p);
		if ((int)nodes[ni].pts.size() > kCapacity && depth < kMaxDepth) Split(ni);
	}

	bool Insert(const Pt& p) {
		if (!Inside(nodes[0], p.x, p.y)) return false;
		// Depth is only used to stop splitting; recompute by walking down.
		int ni = 0, depth = 0;
		while (nodes[ni].child[0] >= 0) { ni = nodes[ni].child[ChildFor(nodes[ni], p.x, p.y)]; depth++; }
		nodes[ni].pts.push_back(p);
		if ((int)nodes[ni].pts.size() > kCapacity && depth < kMaxDepth) Split(ni);
		count++;
		return true;
	}

	void Query(int ni, double qx, double qy, double qw, double qh, ValueList& out) const {
		const Node& n = nodes[ni];
		if (qx > n.x + n.w || qx + qw < n.x || qy > n.y + n.h || qy + qh < n.y) return;
		for (const Pt& p : n.pts) {
			if (p.x >= qx && p.x <= qx + qw && p.y >= qy && p.y <= qy + qh) out.Add(Value(p.id));
		}
		if (n.child[0] >= 0) for (int c = 0; c < 4; c++) Query(n.child[c], qx, qy, qw, qh, out);
	}
};

Quadtree* TreeArg(Context context, const char* fn, Value& err) {
	Quadtree* t = NativeHandlePtr<Quadtree>(context.GetVar("tree"));
	if (t == nullptr) err = ErrorTypes::RuntimeError(String("quadtree.") + fn + ": tree must be a quadtree");
	return t;
}

void Fill(ValueDict& m) {
	Intrinsic f;

	// Create a new quadtree covering the rectangle x, y, width, height; returns the tree
	f = Intrinsic::Create("");
	f.AddParam("x", Value(0));
	f.AddParam("y", Value(0));
	f.AddParam("width", Value(1));
	f.AddParam("height", Value(1));
	f.set_Code(INTRINSIC_LAMBDA {
		Quadtree t;
		t.Reset(context.GetVar("x").DoubleValue(), context.GetVar("y").DoubleValue(),
			context.GetVar("width").DoubleValue(), context.GetVar("height").DoubleValue());
		return IntrinsicResult(NewNativeHandle<Quadtree>(t));
	});
	m.SetValue(String("create"), f.GetFunc());

	// Add a point with the given id (any number) to the tree; returns 1 if the point was stored, 0 if outside the tree's bounds
	f = Intrinsic::Create("");
	f.AddParam("tree");
	f.AddParam("id");
	f.AddParam("x");
	f.AddParam("y");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		Quadtree* t = TreeArg(context, "insert", err);
		if (t == nullptr) return IntrinsicResult(err);
		Pt p{context.GetVar("id").DoubleValue(), context.GetVar("x").DoubleValue(), context.GetVar("y").DoubleValue()};
		return IntrinsicResult(Value(t->Insert(p) ? 1 : 0));
	});
	m.SetValue(String("insert"), f.GetFunc());

	// Return a list of the ids of all points inside the rectangle x, y, width, height
	f = Intrinsic::Create("");
	f.AddParam("tree");
	f.AddParam("x");
	f.AddParam("y");
	f.AddParam("width");
	f.AddParam("height");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		Quadtree* t = TreeArg(context, "query", err);
		if (t == nullptr) return IntrinsicResult(err);
		ValueList out;
		t->Query(0, context.GetVar("x").DoubleValue(), context.GetVar("y").DoubleValue(),
			context.GetVar("width").DoubleValue(), context.GetVar("height").DoubleValue(), out);
		return IntrinsicResult(DynamicList(out));
	});
	m.SetValue(String("query"), f.GetFunc());

	// Return the number of points stored in the tree
	f = Intrinsic::Create("");
	f.AddParam("tree");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		Quadtree* t = TreeArg(context, "count", err);
		if (t == nullptr) return IntrinsicResult(err);
		return IntrinsicResult(Value(t->count));
	});
	m.SetValue(String("count"), f.GetFunc());

	// Remove all points from the tree, keeping its bounds
	f = Intrinsic::Create("");
	f.AddParam("tree");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		Quadtree* t = TreeArg(context, "clear", err);
		if (t == nullptr) return IntrinsicResult(err);
		Node root = t->nodes[0];
		t->Reset(root.x, root.y, root.w, root.h);
		return IntrinsicResult::Null;
	});
	m.SetValue(String("clear"), f.GetFunc());
}

bool Init() {
	return PluginAddModule<&Fill>("quadtree");
}

} // namespace

MS_PLUGIN(quadtree, "1.0", Init, nullptr, nullptr, nullptr)
