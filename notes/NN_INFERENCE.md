# Running a Trained Neural Network in raylib-miniscript

How to take a network trained somewhere else — PyTorch, in the worked example —
and run its **forward pass** in MiniScript on the intrinsic `Matrix` class.

Worked example: the AlphaZero Connect-4 net from `ML-work-2026/rl_connect4`
(1.19M parameters, 4 residual blocks of 128 channels, policy + value heads).
Ported at 8.0 ms per position with **no C++ changes**. The result is
`assets/connect4.ms` plus `assets/lib/{AZNet,C4,MCTS}.ms`, and the exporter is
`export_msmx.py` on the training side.

Inference only. Training would need gradients and an optimizer; nothing here
requires either.

---

## The shape of the job

```
   PyTorch checkpoint
        |
        |  export script, in Python, on the training side
        |    - fold BatchNorm into the preceding layer
        |    - rearrange conv weights for im2col
        |    - fix flatten ordering
        |    - verify against the source framework
        v
   one .msmx blob  (a sequence of MSMX-headered matrices)
        |
        |  readRawData loop
        v
   MiniScript forward pass  (a sequence of gemm calls)
```

Two rules govern everything below:

1. **Every transformation that can happen at export time, happens at export
   time.** BatchNorm folding, weight transposition, index permutation — none of
   it belongs in the per-frame path. The runtime should be nothing but gemms.
2. **Verify numerically before you trust behavior.** A network with a subtly
   wrong weight layout does not crash. It plays badly, and you will blame the
   search.

---

## Step 1 — Inventory the network

Dump the state dict and write down every tensor and its shape:

```python
sd = torch.load("checkpoints/az.pt", map_location="cpu")["net"]
for k, v in sd.items():
    print(f"{k:32s} {tuple(v.shape)}")
```

You are looking for:

- **Layer types.** Dense and 1x1 convolutions are trivial. NxN convolutions
  need im2col (Step 4). Anything recurrent or attention-based needs more
  thought than this document gives.
- **Normalization layers.** Every one is a folding opportunity (Step 3).
- **Activation functions.** Check they exist on `Matrix` — see the table in
  Step 6.
- **Flatten points.** Every `.flatten()` / `.view()` / `.reshape()` between a
  spatial layer and a dense one is a place where index order can silently
  disagree. Mark them; Step 4 explains why.
- **`bias=False` on convs.** This is the norm when a BatchNorm follows, and it
  makes folding exact rather than approximate.

Count the multiply-accumulates while you are here — Step 8 turns that into a
time budget, and it is much better to learn the thing is too slow now than
after writing the port.

---

## Step 2 — Choose the 2D layout

`Matrix` is deliberately 2D (see `MATRIX_DESIGN.md`, "Why 2D only"), so a
convolutional activation of shape `(C, H, W)` has to be flattened. Use
**spatial-position-major: `(H*W) rows x C columns`**, one row per cell, channels
across.

For the Connect-4 board that is 42 rows x 128 columns. This is the right choice
for three reasons:

- A 1x1 convolution becomes `h.gemm(W, b)` with no gather at all.
- An NxN convolution becomes one im2col plus one gemm (Step 4).
- Bias is a 1 x C row vector, which `gemm`'s `addend` broadcasts down every row
  for free — a whole dense layer in one kernel pass.

Batching, if you ever need it, extends the same layout: stack samples as more
rows. For tree search you evaluate one position at a time, so batch size is 1
and the row count is just the cell count.

---

## Step 3 — Fold BatchNorm into the preceding layer

BatchNorm at inference is a fixed affine map, so it can be absorbed into the
convolution or dense layer before it. Do this and the runtime never mentions
normalization.

For `y = gamma * (x - mean) / sqrt(var + eps) + beta` following a conv with
weight `W` and no bias:

```python
scale = gamma / np.sqrt(var + eps)
W_folded = W * scale[:, None, None, None]     # scale each output channel
b_folded = beta - mean * scale
```

Then the layer is exactly `conv(W_folded) + b_folded`.

Details that matter:

- **Use `running_mean` / `running_var`, not batch statistics.** Call
  `net.eval()` before touching anything.
- **`eps` must match the framework's.** PyTorch defaults to `1e-5`. Getting
  this wrong produces a small, plausible-looking, entirely wrong network.
- **If the conv had a bias**, fold it too: `b_folded = (bias - mean) * scale + beta`.
- **Ignore `num_batches_tracked`.** It is bookkeeping, not a parameter.

The same trick works for any fixed affine normalization. LayerNorm does *not*
fold this way, because its statistics depend on the input.

---

## Step 4 — Arrange convolution weights for im2col

An NxN convolution over the layout from Step 2 is one gather plus one product:

```
cols  = cells x (K*Cin)      each cell's KxK neighborhood, zero-padded
W     = (K*Cin) x Cout
out   = cols.gemm(W, bias)   one gemm, bias broadcast
```

where `K = N*N`. The gather ("im2col") is cheap in script — for a fixed offset
the copy is contiguous within a board row, so it is a handful of
`getSub`/`setSub` block copies rather than per-element writes:

```
AZNet.im2col = function(src, dest, ch)
	rows = self.ROWS
	cols = self.COLS
	dest.fill 0
	k = 0
	for dr in [-1, 0, 1]
		for dc in [-1, 0, 1]
			// destination cell (r, c) reads source cell (r+dr, c+dc)
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
end function
```

That is 48 block copies for a 6x7 board and costs 0.4 ms against the ~1.7 ms
gemm it feeds — which is why `MATRIX_DESIGN.md`'s contingency of adding an
`im2col` primitive in C++ was not needed at this size. On a much larger grid,
revisit.

**The weight matrix must be laid out in the same offset order the gather
uses.** Row index is `k*Cin + ci`, with `k` enumerating the `(dr, dc)` offsets:

```python
# PyTorch conv weight is (Cout, Cin, kh, kw); padding=1 means
# out[oc, r, c] = sum W[oc, ci, kh, kw] * x[ci, r+kh-1, c+kw-1]
W_mat = w.transpose(2, 3, 1, 0).reshape(kh * kw * cin, cout)
```

The `(kh, kw)` order here and the `(dr, dc)` loop order in the script are the
same thing written twice. **They must not drift apart**, and nothing will tell
you if they do — put a comment on both sides saying so.

### The flatten-order trap

This is the single most likely place to get a silently wrong network.

PyTorch's `flatten(1)` on a `(C, H, W)` tensor is **channel-major**: index
`ch*H*W + cell`. The layout from Step 2 flattens **cell-major**: `cell*C + ch`.
When a dense layer consumes the output of a spatial layer, its input columns
are therefore in the wrong order.

Fix it at export time by permuting the weight's columns, never at runtime:

```python
cells = ROWS * COLS
perm = np.array([ch * cells + cell for cell in range(cells) for ch in range(2)])
W_fc = pfc[:, perm].T          # now indexed cell-major, ready to gemm
```

Note this only bites when `C > 1`. The value head in the worked example passes
through a 1-channel conv, where both orders coincide — so the policy head was
wrong and the value head was right, which is exactly the kind of partial
symptom that wastes an afternoon. Step 7 catches it in seconds instead.

---

## Step 5 — Write the blob

Serialize every weight matrix, in the order the forward pass will consume them,
into one file. Each matrix carries a 16-byte header:

| Bytes | Field |
|---|---|
| 0..3 | magic `MSMX` |
| 4..5 | uint16 version (currently 1) |
| 6..7 | uint16 dtype code |
| 8..11 | int32 rows |
| 12..15 | int32 columns |

followed by `rows*columns` elements, row-major, little-endian. Dtype codes:
`float64`=1, `float32`=2, `int8`=3, `uint8`=4, `int16`=5, `uint16`=6,
`int32`=7, `uint32`=8, `int64`=9.

```python
buf += struct.pack("<4sHHii", b"MSMX", 1, 2, *m.shape)
buf += np.ascontiguousarray(m, dtype="<f4").tobytes()
```

**Use `float32`.** `Matrix` computes in double internally, but weights do not
need the precision and the file halves in size — 4.75 MB rather than 9.5 MB for
the worked example.

Reading it back is a loop, because `readRawData` returns the position just past
what it read:

```
rd = file.loadRaw("assets/az_c4.msmx")
pos = 0
while pos < rd.len
	m = Matrix.ofSize(1, 1)
	pos = m.readRawData(rd, "auto", pos)     // "auto" = the header gives shape
	weights.push m
end while
```

`Matrix.fromRawData` is the wrong tool for a sequential blob: it returns a
matrix, not a position. `readRawData` with `dtype="auto"` reshapes the receiver
from the header, so a `Matrix.ofSize(1, 1)` placeholder is all you need.

Check the count after loading. A blob that is one matrix short will otherwise
fail deep inside the forward pass with an incomprehensible dimension error.

---

## Step 6 — Write the forward pass

The body is a straight sequence of gemms. Preallocate every activation buffer
once, so evaluating a position does no large allocation:

```
// Stem: 3x3 conv 2 -> CH, then relu.
self.im2col b.obs, b.stemCols, 2
b.stemCols.gemm w[0], w[1], b.h
b.h.relu

// A residual block: the second conv has no relu of its own; the relu
// happens after the skip connection is added.
self.im2col b.h, b.cols, self.CH
b.cols.gemm w[i], w[i + 1], b.y
b.y.relu
self.im2col b.y, b.cols, self.CH
b.cols.gemm w[i + 2], w[i + 3], b.y
b.h.add b.y
b.h.relu
```

Points worth internalizing:

- **`A.gemm(B, addend, out)` is the whole layer.** `addend` broadcasts a 1 x n
  row, so weights-times-input-plus-bias is one kernel pass, and `out` is
  written in place — no allocation per layer.
- **`reshape` is O(1) and row-major.** Flattening a `cells x C` activation for a
  dense head is `m.reshape 1, cells*C`, free. Reshape it back afterwards if the
  buffer is reused. It is *not* transpose.
- **Import `matrixUtil`.** `relu`, `add`, `plus`, `times`, `clone` and friends
  are script wrappers over `gemm`, not intrinsics. Without the import you get
  `Key Not Found: 'relu'`.
- **Return plain MiniScript values** from the evaluate function (`getRow`,
  `getElem`), not the internal buffers, which the next call will overwrite.

Activation availability:

| Function | How |
|---|---|
| `relu` | `m.relu` (wrapper for `clamp(0, null)`) |
| `sigmoid`, `tanh`, `softmax` | intrinsic, in place |
| `clamp`, `abs`, `sqrt`, `pow`, `round` | intrinsic, in place |
| anything else | `m.apply(@f)` — correct, but a script call per element |

Every in-place mutator has a copying `-ed` counterpart (`relued`, `tanhed`,
`softmaxed`). In a hot path use the in-place form.

**Mask before you softmax.** If some outputs are invalid — illegal moves, say —
apply the mask to the logits first. Softmaxing and then zeroing afterwards
leaves probability mass assigned to impossible options.

---

## Step 7 — Verify against the source framework

Do not skip this, and do not do it by playing a game and squinting.

**On the export side**, re-implement the forward pass in numpy *in the exact
form the MiniScript will run it* — same layout, same folded weights, same
flatten order — read it back out of the blob you just wrote, and compare
against the framework itself. Refuse to write the file if it disagrees:

```
folded-numpy vs PyTorch: max |dlogit| = 9.331e-07, max |dvalue| = 6.428e-08
```

This catches folding errors, transposition errors, and flatten-order errors
before a line of MiniScript exists, and it tells you which of the three, because
you can bisect it in Python.

**On the MiniScript side**, have the exporter also emit a handful of real
positions with the outputs the framework produced for them, and assert against
them in a test script (`assets/aznet_test.ms`):

```
checked 8 positions
  max |logit difference| = 0.000001
  max |value difference| = 3.4458203113096886E-07
  8.12 ms per position
PASS -- the MiniScript net matches PyTorch
```

**Expect agreement to about 1e-6, and no better.** The weights are float32 while
the arithmetic is float64, so that is the storage format's resolution, not a
bug. Set the tolerance there — say `1e-4` on logits — so the test fails on real
errors and not on rounding. Anything off by more than that is a genuine
discrepancy, and in practice it will be off by a *lot*, not a little: layout
bugs produce garbage, not drift.

Use realistic positions, not zeros or random noise. An empty board exercises
almost none of the network.

---

## Step 8 — Budget the time, and pay it in instalments

Estimate before you build: multiply-accumulates divided by roughly
**3 GMAC/s** (a dense, conservative figure for this kernel on desktop hardware)
gives a per-evaluation time. Measure end-to-end afterwards.

**Do not budget by summing micro-benchmarks.** In the worked example, isolated
benchmarks of the individual gemm shapes predicted ~15 ms per position; the real
forward pass runs in 8.0 ms. The reason is worth knowing:

> The `NN` matmul kernel skips zero entries of the **left** operand
> (`MatrixCore.cpp`: `if (aik == 0.0) continue;`). An im2col matrix is 85% zeros
> in this network (the relu'd activation feeding it is itself 72% zeros, and the
> zero padding at the board edges adds more), so much of the nominal work never
> executes. Measured: 3.3 GMAC/s on dense
> data, 4.9 GMAC/s with a relu'd left operand, 6.2 GMAC/s effective in the real
> net.

Two practical consequences: **put the sparse operand on the left** in a product
you control, and always measure the assembled thing.

If a single evaluation costs more than a frame — 8 ms already does at 60 fps,
and a tree search multiplies it by the simulation count — **do not run it inside
one frame**. Make the work resumable and spend a fixed budget per frame:

```
MCTS.step = function(budget = 0.012)
	t0 = time
	while not self.finished
		self.simulate
		if budget != null and time - t0 >= budget then break
	end while
	return self.finished
end function
```

The caller keeps drawing, shows a progress bar, and plays the move when `step`
returns true. In the worked example a 100-simulation search takes 0.85 s of wall
clock and the window stays at 60 fps throughout. This also makes search depth a
live setting rather than an architectural decision.

---

## Quirks and traps, collected

- **`import "matrixUtil"`** or the convenience methods are missing.
- **BatchNorm `eps`** must match the source framework (PyTorch: `1e-5`).
- **`net.eval()`** before exporting, so running statistics are used.
- **Flatten order** is channel-major in PyTorch, cell-major here. Permute the
  consuming dense layer's columns at export time. Only bites when `C > 1`.
- **im2col offset order** must match the weight row order. Comment both.
- **`readRawData`, not `fromRawData`**, for sequential loading; only the former
  returns a position.
- **`reshape` is not transpose**, and it is row-major and O(1).
- **Reused buffers are overwritten** by the next evaluation — copy anything you
  keep.
- **float32 storage** caps agreement at ~1e-6. Do not chase it further.
- **Matrix equality**: `m1 == m2` compares handles, not contents. Use
  `m.equals`.
- **Errors are values.** A dimension mismatch returns an error value that
  propagates through chained calls; it is neither `null` nor `0`. Check it when
  loading.
- **A time budget uses `time`**, not `rl.GetTime`, if you want the search to be
  testable without a window.

---

## Worked example, by the numbers

| | |
|---|---|
| Network | 4 residual blocks, 128 channels, policy + value heads |
| Parameters | 1,190,379 |
| Exported | 28 matrices, 4.75 MB float32 |
| MACs per evaluation | ~49.7M nominal |
| Forward pass | 8.0 ms (200 reps) |
| Agreement with PyTorch | 1e-6 logits, 3.4e-7 value |
| Search at 25 sims | 0.22 s/move |
| Search at 100 sims | 0.85 s/move |
| C++ changes required | none |

---

## Checklist

1. [ ] Dump the state dict; list layers, shapes, activations, flatten points.
2. [ ] Count MACs; divide by ~3 GMAC/s; decide it is feasible.
3. [ ] Choose the 2D layout (`cells x channels` for conv nets).
4. [ ] Fold every BatchNorm into the layer before it.
5. [ ] Rearrange conv weights for im2col; permute flatten-consuming dense layers.
6. [ ] Write the blob, float32, in consumption order.
7. [ ] Re-implement the forward pass in numpy; assert it matches the framework.
8. [ ] Emit reference positions alongside the weights.
9. [ ] Write the MiniScript forward pass; preallocate all buffers.
10. [ ] Assert against the reference positions; expect ~1e-6.
11. [ ] Measure the assembled pass; make it resumable if it exceeds a frame.
