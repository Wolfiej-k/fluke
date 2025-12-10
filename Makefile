CLANG=clang-14
CLANGXX=clang++-14
OPT=opt-14
LINK=llvm-link-14
CFLAGS=-fPIC -Wall -Wextra
LDFLAGS=-shared -Wl,-z,now

CLAM=clam/py/clam.py
CLAM_FLAGS=--crab-track=mem --crab-dom=term-dis-int --crab-check=assert --crab-inter

PASS_NAME=bounds-check
PASS_PLUGIN=bounds_check.so
PASS_SRC=BoundsCheck.cpp

PATCH_NAME=patch-entry
PATCH_PLUGIN=patch_entry.so
PATCH_SRC=PatchEntry.cpp

DIR=programs
SRCS=$(wildcard $(DIR)/*.c)

TARGET_EXEC=$(SRCS:%.c=%_exec)
TARGET_LIB=$(SRCS:%.c=%_lib.so)
TARGET_CLAM=$(SRCS:%.c=%_clam.so)

RUNTIME=runtime.c
STUBS=stubs.c

LOADER=./loader/target/release/fixed_loader

.PHONY: all clean clean-all run pass loader clam
.SECONDARY:

all: pass $(TARGET_EXEC) $(TARGET_LIB) $(TARGET_CLAM)

%: $(DIR)/%.c
	@$(MAKE) $(DIR)/$*_exec $(DIR)/$*_lib.so $(DIR)/$*_clam.so

# Build the LLVM plugins
pass: $(PASS_PLUGIN) $(PATCH_PLUGIN)

$(PASS_PLUGIN): $(PASS_SRC)
	$(CLANGXX) -O3 $(CFLAGS) $(LDFLAGS) \
	-I$(shell llvm-config-14 --includedir) \
	-o $@ $< \
	$(shell llvm-config-14 --cxxflags --ldflags --system-libs --libs core irreader passes)

$(PATCH_PLUGIN): $(PATCH_SRC)
	$(CLANGXX) -O3 $(CFLAGS) $(LDFLAGS) \
	-I$(shell llvm-config-14 --includedir) \
	-o $@ $< \
	$(shell llvm-config-14 --cxxflags --ldflags --system-libs --libs core irreader passes)

# Compile runtime to bitcode
$(RUNTIME:.c=.bc): $(RUNTIME)
	$(CLANG) -O3 $(CFLAGS) -emit-llvm -c $< -o $@

# Compile crab stubs to bitcode
$(STUBS:.c=.bc): $(STUBS)
	$(CLANG) -O3 $(CFLAGS) -emit-llvm -c $< -o $@

# Compile to executable without bounds checks
$(DIR)/%_exec: $(DIR)/%.c
	$(CLANG) -O3 $(CFLAGS) $^ -o $@ -lm

# Compile to LLVM IR
$(DIR)/%.ll: $(DIR)/%.c
	$(CLANG) -O3 $(CFLAGS) -Xclang -disable-O0-optnone -S -emit-llvm $< -o $@

# Run bounds check pass
$(DIR)/%_checked.ll: $(DIR)/%.ll $(PASS_PLUGIN)
	$(OPT) -load-pass-plugin=./$(PASS_PLUGIN) -passes=$(PASS_NAME) $< -S -o $@

# Convert to LLVM bitcode
$(DIR)/%_checked.bc: $(DIR)/%_checked.ll
	$(CLANG) $(CFLAGS) -emit-llvm -c $< -o $@

# Link with fluke runtime
$(DIR)/%_linked.bc: $(DIR)/%_checked.bc $(RUNTIME:.c=.bc)
	$(LINK) $(RUNTIME:.c=.bc) $< -o $@

# Inline bounds check functions
$(DIR)/%_inlined.bc: $(DIR)/%_linked.bc
	$(OPT) -passes=always-inline $< -o $@

# Replace crab intrinsics with stubs
$(DIR)/%_lib_stubbed.bc: $(DIR)/%_inlined.bc $(STUBS:.c=.bc)
	$(LINK) $^ -o $@

# Run entry patch pass
$(DIR)/%_lib_patched.bc: $(DIR)/%_lib_stubbed.bc $(PATCH_PLUGIN)
	$(OPT) -load-pass-plugin=./$(PATCH_PLUGIN) -passes=$(PATCH_NAME) $< -o $@

# Run another optimizer pass
$(DIR)/%_lib_optimized.bc: $(DIR)/%_lib_patched.bc $(PATCH_PLUGIN)
	$(OPT) -O3 $< -o $@

# Generate shared object with bounds checks
$(DIR)/%_lib.so: $(DIR)/%_lib_optimized.bc
	$(CLANG) -O3 $(CFLAGS) $(LDFLAGS) $< -o $@

# Run clam on inlined bitcode
$(DIR)/%_clam.bc: $(DIR)/%_inlined.bc
	$(CLAM) $(CLAM_FLAGS) $< -o $@ > $@.log 2>&1
	@./print_failures.sh $@.log

# Replace crab intrinsics with stubs
$(DIR)/%_clam_stubbed.bc: $(DIR)/%_clam.bc $(STUBS:.c=.bc)
	$(LINK) $< $(STUBS:.c=.bc) -o $@

# Run entry patch pass
$(DIR)/%_clam_patched.bc: $(DIR)/%_clam_stubbed.bc $(PATCH_PLUGIN)
	VERIFIED_IDS="$$(python3 extract_safe_ids.py $(DIR)/$*_clam.bc.log)" \
	$(OPT) -load-pass-plugin=./$(PATCH_PLUGIN) -passes=$(PATCH_NAME) \
	$< -o $@

# Run another optimizer pass
$(DIR)/%_clam_optimized.bc: $(DIR)/%_clam_patched.bc $(PATCH_PLUGIN)
	$(OPT) -O3 $< -o $@

# Generate shared object with unsafe bounds checks
$(DIR)/%_clam.so: $(DIR)/%_clam_optimized.bc
	$(CLANG) -O3 $(CFLAGS) $(LDFLAGS) $< -o $@

# Helper scripts
loader:
	cd loader && cargo build --release

clam:
	./build_clam.sh

run: all
	@echo "--- Running Executables ---"
	@for prog in $(TARGET_EXEC); do \
		echo "Running ./$$prog"; \
		./$$prog; \
	done
	
	@echo "\n--- Running Libraries ---"
	@for prog in $(TARGET_LIB); do \
		echo "Loading $$prog"; \
		$(LOADER) $$prog; \
	done
	
	@echo "\n--- Running Optimized ---"
	@for prog in $(TARGET_CLAM); do \
		echo "Loading $$prog"; \
		$(LOADER) $$prog; \
	done

clean:
	rm -f $(DIR)/*_exec $(DIR)/*.so $(DIR)/*.ll $(DIR)/*.bc $(DIR)/*.bc.log *.csv

clean-all: clean
	rm -f $(RUNTIME:.c=.bc) $(STUBS:.c=.bc) *.so *.o
