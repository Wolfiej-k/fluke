CLANG=clang-14
CLANGXX=clang++-14
OPT=opt-14
LINK=llvm-link-14
CLAM=../clam/py/clam.py

CFLAGS=-fPIC -Wall -Wextra
LDFLAGS=-shared -Wl,-z,now
CLAM_FLAGS=--crab-track=mem --crab-check=assert --crab-opt=dce --crab-promote-assume --inline

PASS_NAME=bounds-check
PASS_PLUGIN=bounds_check.so
PASS_SRC=BoundsCheck.cpp

DIR=programs
SRCS=$(wildcard $(DIR)/*.c)

TARGET_EXEC=$(SRCS:%.c=%_exec)
TARGET_LIB=$(SRCS:%.c=%_lib.so)
TARGET_CLAM=$(SRCS:%.c=%_clam.so)

RUNTIME=runtime.c
STUBS=stubs.c

LOADER=../fixed_loader/target/release/fixed_loader

.PHONY: all clean run pass

all: pass $(TARGET_EXEC) $(TARGET_LIB) $(TARGET_CLAM)

# Build the LLVM pass plugin
pass: $(PASS_PLUGIN)

$(PASS_PLUGIN): $(PASS_SRC)
	$(CLANGXX) -O3 $(CFLAGS) $(LDFLAGS) \
	-I$(shell llvm-config-14 --includedir) \
	-o $@ $< \
	$(shell llvm-config-14 --cxxflags --ldflags --system-libs --libs core irreader passes)

# Compile runtime to bitcode
$(RUNTIME:.c=.bc): $(RUNTIME)
	$(CLANG) $(CFLAGS) -emit-llvm -c $< -o $@

# Compile crab stubs to bitcode
$(STUBS:.c=.bc): $(STUBS)
	$(CLANG) $(CFLAGS) -emit-llvm -c $< -o $@

# Compile to executable without bounds checks
$(DIR)/%_exec: $(DIR)/%.c
	$(CLANG) -O3 $(CFLAGS) $^ -o $@ -lm

# Compile to LLVM IR
$(DIR)/%.ll: $(DIR)/%.c
	$(CLANG) $(CFLAGS) -S -emit-llvm $< -o $@

# Run bounds check pass
$(DIR)/%_checked.ll: $(DIR)/%.ll $(PASS_PLUGIN)
	$(OPT) -load-pass-plugin=./$(PASS_PLUGIN) -passes=$(PASS_NAME) $< -S -o $@

# Convert to LLVM bitcode
$(DIR)/%_checked.bc: $(DIR)/%_checked.ll
	$(CLANG) $(CFLAGS) -emit-llvm -c $< -o $@

# Link with fluke runtime
$(DIR)/%_linked.bc: $(DIR)/%_checked.bc $(RUNTIME:.c=.bc)
	$(LINK) $(RUNTIME:.c=.bc) $< -o $@

# Generate shared object with bounds checks
$(DIR)/%_lib.so: $(DIR)/%_linked.bc
	$(CLANG) -O3 $(CFLAGS) $(LDFLAGS) $< -o $@

# Run clam on linked bitcode
$(DIR)/%_clam.bc: $(DIR)/%_linked.bc
	$(CLAM) $(CLAM_FLAGS) $< -o $@ > $@.log

# Replace crab intrinsics with stubs
$(DIR)/%_stubbed.bc: $(DIR)/%_clam.bc $(STUBS:.c=.bc)
	$(LINK) $< $(STUBS:.c=.bc) -o $@

# Run LLVM optimizer to remove checks
$(DIR)/%_optimized.bc: $(DIR)/%_clam.bc
	$(OPT) -O3 $< -o $@

# Generate shared object with unsafe bounds checks
$(DIR)/%_clam.so: $(DIR)/%_optimized.bc
	$(CLANG) $(CFLAGS) $(LDFLAGS) $< -o $@

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
	
	@echo "\n--- Running Optimized Libraries ---"
	@for prog in $(TARGET_CLAM); do \
		echo "Loading $$prog"; \
		$(LOADER) $$prog; \
	done

clean:
	rm -f $(DIR)/*_exec $(DIR)/*.so $(DIR)/*.ll $(DIR)/*.bc *.o $(RUNTIME:.c=.bc) *.so
