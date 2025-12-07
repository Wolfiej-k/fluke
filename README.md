# Fluke

Flat memory isolation using the [Crab](https://github.com/seahorn/crab) abstract
interpreter. Bounds-check, optimize, and load untrusted processes into a shared
virtual address space. See `Makefile` to get started.

### Quick Start
Build the loader:
```
make loader
```

Build Clam:
```
make clam
```

Compile and run `.c` files in `/programs`:
```
make run
```
