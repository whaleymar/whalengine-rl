.PHONY: all clean release

all:
	cmake . -DCMAKE_BUILD_TYPE=Debug -B build && $(MAKE) -C build -j

release:
	cmake . -DCMAKE_BUILD_TYPE=Release -B build && $(MAKE) -C build -j

WEB:
	emcmake cmake . -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN=true -B build && emmake $(MAKE) -C build -j

clean:
	rm -r build || true

