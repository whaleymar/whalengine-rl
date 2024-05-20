.PHONY: all clean release

all:
	cmake . -DCMAKE_BUILD_TYPE=Debug -B build && $(MAKE) -C build -j

release:
	cmake . -DCMAKE_BUILD_TYPE=Release -B build && $(MAKE) -C build -j

clean:
	rm -r build || true

