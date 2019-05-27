.PHONY: default clean xo.so xo-jack
.SILENT: default clean xo.so xo-jack
default:
	ninja -v
xo.so:
	ninja -v xo.so
xo-jacl:
	ninja -v xo-jack
clean:
	ninja -v -t clean
