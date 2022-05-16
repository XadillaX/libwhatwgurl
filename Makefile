PYTHON ?= python3
BUILDTYPE ?= Release
GYP=./node_modules/node-gyp/gyp/gyp_main.py
CPPLINT=./node_modules/.bin/cpplint
CLANG_FORMAT=./node_modules/.bin/clang-format
MAIN_GYP_FILE=./libwhatwgurl.gyp

out/$(BUILDTYPE)/libwhatwgurl.a: out/$(BUILDTYPE)/Makefile
	make -C out BUILDTYPE=$(BUILDTYPE) libwhatwgurl

# example: out/$(BUILDTYPE)/example
#
# out/$(BUILDTYPE)/example: out/$(BUILDTYPE)/Makefile
# 	make -C out BUILDTYPE=$(BUILDTYPE) example

compile_commands.json:
	./node_modules/.bin/node-gyp -- configure -f=gyp.generator.compile_commands_json.py
	mv Release/compile_commands.json compile_commands.json
	rm -rf Release
	rm -rf Debug

out/$(BUILDTYPE)/Makefile: $(MAIN_GYP_FILE)
	$(PYTHON) $(GYP) \
		--depth=. \
		--generator-output=./out \
		-Goutput_dir=. \
		-fmake \
		$(MAIN_GYP_FILE)
	$(PYTHON) $(GYP) \
		--depth=. \
		--generator-output=./out \
		-Goutput_dir=./out \
		-fcompile_commands_json \
		$(MAIN_GYP_FILE)

CPP_FILES=$(shell find src include binding -type f -name '*.cc' -or -name '*.h')
lint-cpp: $(CPP_FILES)
	$(CPPLINT) $(CPP_FILES)
clang-format: $(CPP_FILES)
	$(CLANG_FORMAT) -i --style=file $(CPP_FILES)

.PHONY: out/$(BUILDTYPE)/libwhatwgurl.a lint-cpp clang-format compile_commands.json
