
  var Module = typeof Module != 'undefined' ? Module : {};

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'docs/game.data';
      var REMOTE_PACKAGE_BASE = 'game.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

      var fetchPromise;
      var fetched = Module['getPreloadedPackage']?.(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);

      if (!fetched) {
        // Note that we don't use await here because we want to execute the
        // the rest of this function immediately.
        fetchPromise = fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "assets", true, true);
Module['FS_createPath']("/assets", "lib", true, true);
Module['FS_createPath']("/assets", "shaders", true, true);
Module['FS_createPath']("/assets", "sounds", true, true);

      for (var file of metadata['files']) {
        var name = file['filename']
        Module['addRunDependency'](`fp ${name}`);
      }

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
          Module['FS_createDataFile'](name, null, data, true, true, true);
          Module['removeRunDependency'](`fp ${name}`);
          }
          Module['removeRunDependency']('datafile_docs/game.data');
      }
      Module['addRunDependency']('datafile_docs/game.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

      Module['preloadResults'][PACKAGE_NAME] = {fromCache: false};
      if (!fetched) {
        fetched = await fetchPromise;
      }
      processPackageData(fetched);

    }
    if (Module['calledRun']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/assets/AZNet.ms", "start": 0, "end": 6434}, {"filename": "/assets/Asteroids-2X.png", "start": 6434, "end": 199765}, {"filename": "/assets/C4.ms", "start": 199765, "end": 201937}, {"filename": "/assets/MCTS.ms", "start": 201937, "end": 208181}, {"filename": "/assets/Merkin.ttf", "start": 208181, "end": 294477}, {"filename": "/assets/Sprite.ms", "start": 294477, "end": 295215}, {"filename": "/assets/Wumpus.png", "start": 295215, "end": 311854}, {"filename": "/assets/angryBalls.ms", "start": 311854, "end": 325810}, {"filename": "/assets/asteroids.ms", "start": 325810, "end": 336568}, {"filename": "/assets/az_c4.msmx", "start": 336568, "end": 5084628}, {"filename": "/assets/basic.ms", "start": 5084628, "end": 5086775}, {"filename": "/assets/breakout.ms", "start": 5086775, "end": 5095779}, {"filename": "/assets/callback_smoke.ms", "start": 5095779, "end": 5099136}, {"filename": "/assets/connect4.ms", "start": 5099136, "end": 5108488}, {"filename": "/assets/digits.png", "start": 5108488, "end": 5112969}, {"filename": "/assets/diskdrop.ms", "start": 5112969, "end": 5115788}, {"filename": "/assets/fireworks.ms", "start": 5115788, "end": 5126547}, {"filename": "/assets/highdpi_demo.ms", "start": 5126547, "end": 5131134}, {"filename": "/assets/lib/grfon.ms", "start": 5131134, "end": 5145095}, {"filename": "/assets/lib/importUtil.ms", "start": 5145095, "end": 5146192}, {"filename": "/assets/lib/json.ms", "start": 5146192, "end": 5162274}, {"filename": "/assets/lib/listUtil.ms", "start": 5162274, "end": 5178479}, {"filename": "/assets/lib/mapUtil.ms", "start": 5178479, "end": 5184081}, {"filename": "/assets/lib/mathUtil.ms", "start": 5184081, "end": 5216756}, {"filename": "/assets/lib/matrixUtil.ms", "start": 5216756, "end": 5246559}, {"filename": "/assets/lib/pathUtil.ms", "start": 5246559, "end": 5256351}, {"filename": "/assets/lib/stringUtil.ms", "start": 5256351, "end": 5275501}, {"filename": "/assets/lib/tsv.ms", "start": 5275501, "end": 5281276}, {"filename": "/assets/lifeDemo.ms", "start": 5281276, "end": 5288742}, {"filename": "/assets/main.ms", "start": 5288742, "end": 5295891}, {"filename": "/assets/matrix_numpy_test.ms", "start": 5295891, "end": 5431290}, {"filename": "/assets/matrix_test.ms", "start": 5431290, "end": 5501962}, {"filename": "/assets/metaballDemo.ms", "start": 5501962, "end": 5504844}, {"filename": "/assets/physics.ms", "start": 5504844, "end": 5536444}, {"filename": "/assets/physicsBench.ms", "start": 5536444, "end": 5541762}, {"filename": "/assets/physicsDemo.ms", "start": 5541762, "end": 5547203}, {"filename": "/assets/physicsFallback.ms", "start": 5547203, "end": 5573746}, {"filename": "/assets/raymath_smoke.ms", "start": 5573746, "end": 5578785}, {"filename": "/assets/rmodels_smoke.ms", "start": 5578785, "end": 5584557}, {"filename": "/assets/rtext_smoke.ms", "start": 5584557, "end": 5585906}, {"filename": "/assets/run_smoke.ms", "start": 5585906, "end": 5586701}, {"filename": "/assets/run_smoke2.ms", "start": 5586701, "end": 5587809}, {"filename": "/assets/runfunction_test.ms", "start": 5587809, "end": 5593311}, {"filename": "/assets/sandDemo.ms", "start": 5593311, "end": 5600061}, {"filename": "/assets/shaderDemo.ms", "start": 5600061, "end": 5602794}, {"filename": "/assets/shaders/fireShader.fs", "start": 5602794, "end": 5604690}, {"filename": "/assets/shaders/lambert.fs", "start": 5604690, "end": 5605141}, {"filename": "/assets/shaders/lambert.vs", "start": 5605141, "end": 5605553}, {"filename": "/assets/shaders/metaballs.fs", "start": 5605553, "end": 5607371}, {"filename": "/assets/shaders/plasmaShader.fs", "start": 5607371, "end": 5608333}, {"filename": "/assets/shaders/sandDisplay.fs", "start": 5608333, "end": 5608901}, {"filename": "/assets/shaders/sandSim.fs", "start": 5608901, "end": 5612895}, {"filename": "/assets/shaders/sdf.fs", "start": 5612895, "end": 5613620}, {"filename": "/assets/sounds.ms", "start": 5613620, "end": 5616965}, {"filename": "/assets/sounds/SoundBounce.wav", "start": 5616965, "end": 5634361}, {"filename": "/assets/sounds/SoundCoin.wav", "start": 5634361, "end": 5659103}, {"filename": "/assets/sounds/SoundFallDull.wav", "start": 5659103, "end": 5742055}, {"filename": "/assets/text_font_sdf.ms", "start": 5742055, "end": 5747891}, {"filename": "/assets/wooble.wav", "start": 5747891, "end": 5942297}], "remote_package_size": 5942297});

  })();
