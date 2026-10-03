// plugins/video/web/video.js -- the browser backend of the video plugin.
//
// Linked into the web build with --pre-js (see plugins/video/plugin.cmake).  It defines
// the Module.vpxVideo* functions that RVideo.cpp calls: each video is a hidden
// <video> element, and CopyFrameRGBA draws its current frame to a canvas and copies
// the pixels into the engine's memory.  The browser does the decoding, so any
// format it plays (WebM/VP8, VP9, MP4/H.264 ...) works; the file is fetched from
// the page's server by URL, so it must be deployed with the web build.
//
// Browsers refuse to start un-muted playback until the user has interacted with the
// page; when play() is refused we retry muted, so the picture still plays.

(function () {
	var videos = {};
	var nextId = 1;

	function get(id) { return videos[id] || null; }

	Module['vpxVideoLoad'] = function (url) {
		return new Promise(function (resolve, reject) {
			var el = document.createElement('video');
			el.preload = 'auto';
			el.playsInline = true;
			el.setAttribute('playsinline', '');
			el.crossOrigin = 'anonymous';
			// Attached (but invisible): some browsers will not play detached elements.
			el.style.cssText = 'position:fixed;left:0;top:0;width:1px;height:1px;opacity:0;pointer-events:none;';
			document.body.appendChild(el);
			el.onloadedmetadata = function () {
				var id = nextId++;
				var canvas = document.createElement('canvas');
				canvas.width = el.videoWidth;
				canvas.height = el.videoHeight;
				videos[id] = {
					el: el,
					canvas: canvas,
					ctx: canvas.getContext('2d', { willReadFrequently: true })
				};
				resolve({ id: id, width: el.videoWidth, height: el.videoHeight, duration: el.duration });
			};
			el.onerror = function () {
				if (el.parentNode) el.parentNode.removeChild(el);
				reject(new Error('could not load video ' + url));
			};
			el.src = url;
			el.load();
		});
	};

	Module['vpxVideoDestroy'] = function (id) {
		var v = get(id);
		if (!v) return;
		v.el.pause();
		v.el.removeAttribute('src');
		v.el.load();
		if (v.el.parentNode) v.el.parentNode.removeChild(v.el);
		delete videos[id];
	};

	Module['vpxVideoPlay'] = function (id) {
		var v = get(id);
		if (!v) return;
		var p = v.el.play();
		if (p && p.catch) {
			p.catch(function () {
				v.el.muted = true;      // autoplay policy: retry without sound
				v.el.play().catch(function (e) { console.warn('video play refused:', e); });
			});
		}
	};

	Module['vpxVideoPause'] = function (id) { var v = get(id); if (v) v.el.pause(); };

	Module['vpxVideoStop'] = function (id) {
		var v = get(id);
		if (!v) return;
		v.el.pause();
		v.el.currentTime = 0;
	};

	Module['vpxVideoSeek'] = function (id, timeSec) { var v = get(id); if (v) v.el.currentTime = timeSec; };

	Module['vpxVideoIsPlaying'] = function (id) {
		var v = get(id);
		return v && !v.el.paused && !v.el.ended ? 1 : 0;
	};

	Module['vpxVideoIsFinished'] = function (id) {
		var v = get(id);
		return !v || (v.el.ended && !v.el.loop) ? 1 : 0;
	};

	Module['vpxVideoGetTimePlayed'] = function (id) { var v = get(id); return v ? v.el.currentTime : 0; };

	Module['vpxVideoGetTimeLength'] = function (id) {
		var v = get(id);
		return v && isFinite(v.el.duration) ? v.el.duration : 0;
	};

	Module['vpxVideoSetLooping'] = function (id, enabled) { var v = get(id); if (v) v.el.loop = !!enabled; };
	Module['vpxVideoGetLooping'] = function (id) { var v = get(id); return v && v.el.loop ? 1 : 0; };

	Module['vpxVideoSetPlaybackRate'] = function (id, rate) { var v = get(id); if (v) v.el.playbackRate = rate; };
	Module['vpxVideoGetPlaybackRate'] = function (id) { var v = get(id); return v ? v.el.playbackRate : 1; };

	// Copy the current frame as RGBA into the engine's memory (heapU8 is its byte view).
	// Returns the number of bytes copied, or 0 if there is no frame to show yet.
	Module['vpxVideoCopyFrameRGBA'] = function (id, dstPtr, maxBytes, heapU8) {
		var v = get(id);
		if (!v || v.el.readyState < 2 /* HAVE_CURRENT_DATA */) return 0;
		var w = v.canvas.width, h = v.canvas.height;
		v.ctx.drawImage(v.el, 0, 0, w, h);
		var data = v.ctx.getImageData(0, 0, w, h).data;
		var n = Math.min(data.length, maxBytes);
		heapU8.set(data.subarray(0, n), dstPtr);
		return n;
	};
})();
