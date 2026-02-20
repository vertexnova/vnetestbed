# VneTestbed Examples

This directory contains examples demonstrating the VneTestbed API.

## Building Examples

From the project root:

```bash
cmake -B build -DVNE_TESTBED_EXAMPLES=ON
cmake --build build
```

Alternatively, `-DVNE_TESTBED_DEV=ON` enables both tests and examples.

Executables are placed in `build/bin/examples/`.

## Available Examples

### 01_hello_testbed — Getting Started

Minimal usage: call `vne::testbed_ns::hello()` and `get_version()`.

**Run:** `./build/bin/examples/example_01_hello_testbed`

## Quick Reference

| Example              | Focus          | Key Concepts              |
|----------------------|----------------|---------------------------|
| 01_hello_testbed     | Getting started| hello(), get_version()   |
