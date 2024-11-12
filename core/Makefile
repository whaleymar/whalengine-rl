.PHONY: all clean release

all:
	cmake . -DCMAKE_BUILD_TYPE=Debug -B build -G Ninja && cd build && cmake --build . -j && cd ..

release:
	cmake . -DCMAKE_BUILD_TYPE=Release -B build -G Ninja && cd build && cmake --build . -j && cd ..

webrelease:
	EMCC_AUTODEBUG=0 emcmake cmake . -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN=true -B build && emmake $(MAKE) -C build -j

webdebug:
	EMCC_AUTODEBUG=1 emcmake cmake . -DCMAKE_BUILD_TYPE=Debug -DEMSCRIPTEN=true -B build && emmake $(MAKE) -C build -j

singlethread:
	cmake . -DCMAKE_BUILD_TYPE=Debug -B build -G Ninja && cd build && cmake --build . && cd ..

clean:
	(rm -r build || true) && (rm engined engine engined.js engined.wasm engined.data engined.html engine.js engine.wasm engine.data engine.html || true)


