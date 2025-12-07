# Fluke

Flat memory isolation using the [Crab](https://github.com/seahorn/crab) abstract
interpreter. Bounds-check, optimize, and load untrusted processes into a shared
virtual address space. See the `Makefile` to get started.

### Quick Start
```
git clone --recurse-submodules https://github.com/Wolfiej-k/fluke.git
make loader
make clam
make run
```
