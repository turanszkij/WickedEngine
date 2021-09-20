# Makefile for FAudio#
# Written by Ethan "flibitijibibo" Lee

build: clean
	mkdir bin
	cp app.config bin/FAudio-CS.dll.config
	mcs /unsafe -debug -out:bin/FAudio-CS.dll -target:library FAudio.cs

clean:
	rm -rf bin
