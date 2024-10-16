.PHONY: all clean release

all:
	cmake . -DCMAKE_BUILD_TYPE=Debug -B build -G Ninja && cd build && cmake --build . -j && cd ..

release:
	cmake . -DCMAKE_BUILD_TYPE=Release -B build -G Ninja && cd build && cmake --build . -j && cd ..

webdebug:
	EMCC_AUTODEBUG=1 emcmake cmake . -DCMAKE_BUILD_TYPE=Debug -DEMSCRIPTEN=true -B build && emmake $(MAKE) -C build -j

clean:
	(rm -r build || true) && (rm engined || true) && (rm engine || true) && (rm engined.js || true) && (rm engined.wasm || true) && (rm engined.data || true) && (rm engined.html || true)

