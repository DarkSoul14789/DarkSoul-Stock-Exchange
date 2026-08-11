# Order Book Matching Engine

A limit order book matching engine in C++17, with concurrent order ingestion
and throughput/latency benchmarking.

## Build & test

```bash
rmdir build
mkdir build && cd build
cmake .. 
cmake --build . --config Debug
ctest -C Debug --output-on-failure
```

