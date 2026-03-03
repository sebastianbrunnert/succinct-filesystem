# This file is part of the Succinct Filesystem project.
#
# Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
# SPDX-License-Identifier: GPL-2.0-only

.PHONY: all build test clean configure link

all: test

configure:
	cmake -B build

build: configure
	cmake --build build

test: build
	ctest --test-dir build --output-on-failure

link: build
ifeq ($(shell uname -s),Linux)
	fusermount -u tmp || true
	rm -rf tmp || true
	mkdir -p tmp
	./build/succinct_filesystem $(CURDIR)/tmp.img tmp
else
	@echo "FUSE filesystem can only be linked on Linux systems"
endif

clean:
	rm -rf build