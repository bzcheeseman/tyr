TYRC_INSTALLED := $(shell which tyr 2> /dev/null)

find_tyrc:
ifndef TYRC_INSTALLED
	@echo "Reading TYRC_PATH from the environment, if there's an error \
	like \"/tyr: no such program\" run \"export TYRC_PATH=/path/to/tyr/executable\""
TYRC = ${TYRC_PATH}/tyr
else
TYRC = tyr
endif

UNAME_S := $(shell uname -s)

LDFLAGS :=
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib
endif

CPPFLAGS :=
ifeq ($(UNAME_S),Darwin)
	CPPFLAGS += -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1 -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/10.0.0/include -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk/usr/include
endif

PYTHON_BIN :=
ifeq ($(UNAME_S),Darwin)
	PYTHON_BIN += ../venv/bin/python
else
	PYTHON_BIN += python3
endif

help: find_tyrc
	$(TYRC) -help

path.bc: find_tyrc
	$(TYRC) path.tyr -emit-llvm -bind-lang=c -out-dir=./c
	$(TYRC) path.tyr -emit-llvm -link-rt -bind-lang=c -out-dir=./x-lang

test.out: path.bc
	clang++ -O3 $(LDFLAGS) $(CPPFLAGS) -flto ./c/path.bc ./c/integration_test.cpp -o ./c/test.out

test_c: test.out
	@echo Starting test
	./c/test.out

path.py: find_tyrc
	$(TYRC) path.tyr -bind-lang=python -out-dir=./python
	$(TYRC) path.tyr -bind-lang=python -link-rt -out-dir=./x-lang

test_python: path.py
	@echo Starting test
	$(PYTHON_BIN) ./python/integration_test.py

copy_tyr_header:
	mkdir -p ./tyr/rt
	cp ../lib/compiler/runtime/*.h ./tyr/rt/ 2>/dev/null || :

test_x_lang: path.bc path.py copy_tyr_header
	clang++ -O3 $(LDFLAGS) $(CPPFLAGS) -I. -flto ./x-lang/path.bc ./x-lang/integration_test.cpp -o ./x-lang/test.out
	@echo Starting test
	./x-lang/test.out
	$(PYTHON_BIN) ./x-lang/integration_test.py