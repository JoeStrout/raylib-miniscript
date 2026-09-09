// AZNet.ms
//
// Inference for the AlphaZero Connect-4 network trained in PyTorch
// (ML-work-2026/rl_connect4).  A 1.19M-parameter residual conv net -- a 2->128
// stem, four 128-channel residual blocks, and separate policy and value heads
// -- evaluated entirely through the intrinsic Matrix class, with no scripted
// loop anywhere in the arithmetic.
//
// Weights come from `assets/az_c4.msmx`, written by `export_msmx.py`.  The
// exporter has already folded every BatchNorm into the convolution ahead of it
// (all the convs were trained with bias=False, so a folded layer is exactly
// `conv(W') + b'`), which is why nothing here mentions BatchNorm.
//
//
// LAYOUT
//
// The Matrix class is deliberately 2D, so the (C, H, W) activation of a conv
// net has to be flattened.  We use **cells x channels**: the 6x7 board becomes
// 42 rows, one per cell, indexed `cell = row*COLS + col` with row 0 the bottom,
// and the channels are the columns.
//
// That makes a 3x3 convolution one im2col followed by one gemm:
//
//      cols  = 42 x (9*Cin)      the 3x3 neighborhood of every cell, zero-padded
//      W     = (9*Cin) x Cout
//      out   = cols * W + b      one gemm, bias broadcast by gemm's addend
//
// and a 1x1 convolution is just `h * W + b` with no gather at all.  The im2col
// is 48 block copies (9 offsets x 6 board rows, less those clipped at the top
// and bottom edges) via getSub/setSub, costing about 0.4 ms against the gemm it
// feeds -- so a scripted gather is not worth a C++ primitive here.
//
// Weight rows are ordered `k*Cin + ci`, where k enumerates the nine (dr, dc)
// offsets in the order the loops below walk them.  The exporter matches that
// order; the two must not drift apart.
//
// The forward pass reuses preallocated buffers throughout, so evaluating a
// position allocates only the small per-block getSub temporaries.

import "matrixUtil"

ROWS = 6
COLS = 7
CELLS = 42          // ROWS * COLS
CH = 128            // trunk channels
BLOCKS = 4          // residual blocks

w = null            // list of weight matrices, in file order
buf = null          // preallocated activation buffers


// Load the weight blob.  Returns the net, or an error value.
load = function(path = "assets/az_c4.msmx")
	rd = file.loadRaw(path)
	if rd == null then return err("aznet: cannot read " + path)

	net = new self
	net.w = []
	pos = 0
	while pos < rd.len
		m = Matrix.ofSize(1, 1)
		pos = m.readRawData(rd, "auto", pos)
		if pos isa error then return pos
		net.w.push m
	end while

	expected = 2 + 4 * self.BLOCKS + 10
	if net.w.len != expected then
		return err("aznet: expected " + expected + " matrices in " + path +
		  ", got " + net.w.len)
	end if

	net.allocBuffers
	return net
end function


// Preallocate every activation buffer the forward pass needs, so that
// evaluating a position does no large allocation.
allocBuffers = function
	cells = self.CELLS
	ch = self.CH
	self.buf = {}
	self.buf.obs = Matrix.ofSize(cells, 2)             // input planes
	self.buf.stemCols = Matrix.ofSize(cells, 9 * 2)    // im2col of the input
	self.buf.h = Matrix.ofSize(cells, ch)              // trunk activation
	self.buf.y = Matrix.ofSize(cells, ch)              // residual branch
	self.buf.cols = Matrix.ofSize(cells, 9 * ch)       // im2col of the trunk
	self.buf.p = Matrix.ofSize(cells, 2)               // policy head conv
	self.buf.v = Matrix.ofSize(cells, 1)               // value head conv
	self.buf.vh = Matrix.ofSize(1, 64)                 // value head hidden
	self.buf.logits = Matrix.ofSize(1, self.COLS)
	self.buf.value = Matrix.ofSize(1, 1)
end function


// Gather each cell's 3x3 neighborhood into `dest` (cells x 9*ch), zero-padded
// at the board edges.  Offsets are enumerated (dr, dc) over -1, 0, 1 -- the
// same order `export_msmx.py` laid the weight rows out in.
//
// For a fixed offset the copy is contiguous within a board row, so each offset
// costs at most ROWS block copies rather than CELLS element writes.
im2col = function(src, dest, ch)
	rows = self.ROWS
	cols = self.COLS
	dest.fill 0
	k = 0
	for dr in [-1, 0, 1]
		for dc in [-1, 0, 1]
			// Destination cell (r, c) reads source cell (r+dr, c+dc).  Clip
			// the column span to the board; rows are clipped per iteration.
			if dc < 0 then
				c0 = -dc
				n = cols + dc
			else
				c0 = 0
				n = cols - dc
			end if
			for r in range(0, rows - 1, 1)
				sr = r + dr
				if sr < 0 or sr >= rows then continue
				block = src.getSub(sr * cols + c0 + dc, 0, n, ch)
				dest.setSub r * cols + c0, k * ch, block
			end for
			k = k + 1
		end for
	end for
	return dest
end function


// Evaluate one position.
//
//   own, opp:  42-element lists of 0/1, from the mover's point of view,
//              indexed cell = row*COLS + col (row 0 = bottom).
//
// Returns {"logits": list of COLS raw policy logits, "value": number in
// (-1, 1)}, the value being the expected outcome for the player to move.
// The logits are unnormalized and unmasked -- the caller applies the legal
// move mask, because masking before the softmax is what keeps an illegal
// column from taking probability away from the legal ones.
evaluate = function(own, opp)
	b = self.buf
	w = self.w

	b.obs.setColumn 0, own
	b.obs.setColumn 1, opp

	// Stem: 3x3 conv 2 -> CH, then relu.
	self.im2col b.obs, b.stemCols, 2
	b.stemCols.gemm w[0], w[1], b.h
	b.h.relu

	// Residual trunk.  Second conv of each block has no relu of its own; the
	// relu happens after the skip connection is added.
	i = 2
	for blk in range(1, self.BLOCKS)
		self.im2col b.h, b.cols, self.CH
		b.cols.gemm w[i], w[i + 1], b.y
		b.y.relu
		self.im2col b.y, b.cols, self.CH
		b.cols.gemm w[i + 2], w[i + 3], b.y
		b.h.add b.y
		b.h.relu
		i = i + 4
	end for

	// Policy head: 1x1 conv CH -> 2, relu, flatten, dense to COLS.
	b.h.gemm w[i], w[i + 1], b.p
	b.p.relu
	b.p.reshape 1, 2 * self.CELLS       // O(1); cell-major, as the exporter expects
	b.p.gemm w[i + 2], w[i + 3], b.logits
	b.p.reshape self.CELLS, 2           // restore for the next call

	// Value head: 1x1 conv CH -> 1, relu, flatten, dense 42 -> 64 -> 1, tanh.
	b.h.gemm w[i + 4], w[i + 5], b.v
	b.v.relu
	b.v.reshape 1, self.CELLS
	b.v.gemm w[i + 6], w[i + 7], b.vh
	b.vh.relu
	b.vh.gemm w[i + 8], w[i + 9], b.value
	b.value.tanh
	b.v.reshape self.CELLS, 1

	return {"logits": b.logits.getRow(0), "value": b.value.getElem(0, 0)}
end function
