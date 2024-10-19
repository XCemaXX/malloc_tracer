# malloc_tracer

## Overview
`malloc_tracer` is a dynamic library for tracing memory allocations in applications. It can be used to track malloc, calloc, realloc, and free operations, enabling deep analysis of memory usage patterns. 

## How to Build
1. **Clone the repository** and navigate to the project directory.
2. **Build the library** using `cmake`:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cmake --install build --prefix ./output
   ``` 
3. **Extra flags**:
    ```
    -DTURN_ON_MALLOC_COUNTERS=ON # count allocs
    -DDEBUG=ON # print allocs events
    ```
4. **Example Build**. You can also build with the hello_world example:
    ```
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_HELLO_WORLD=ON
    ```  

## How to use
1. **Preload the library** to track allocations in your application:
    ```
    LD_PRELOAD=/full/path/to/libmalloc_tracer.so ./example_app
    ```
2. **Capture a memory dump** of the running application:
    ```
    gcore -o app.dump $(pgrep example_app)
    ```

## Memory Dump Analysis

### Manual Way
1. Use [CHAP](https://github.com/vmware/chap) to list memory allocations:
    ```
    ./chap app.dump
    > list used
    ```
2. Use gdb to inspect the allocations:
   ```
   gdb example_app app.dump
   ```

## Example Walkthrough
1. **Build example** and download and build [chap](https://github.com/vmware/chap):
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DTURN_ON_MALLOC_COUNTERS=ON -DBUILD_HELLO_WORLD=ON
   cmake --build build
   cmake --install build --prefix ./output
   ```
2. **Run the Example**:
   ```
   cd output
   LD_PRELOAD=$(pwd)/libmalloc_tracer.so ./hello_world
   ```
   Example output: `Pid 1089364 0 big100Mb is 0x7f498e528010; addr10Mb is 0x7f498db27010`
3. **Create a Dump**:
   ```
   sudo gcore -o app.dump $(pgrep hello_world) # make dump
   sudo chmod 0777 app.dump
   ```
4. **Analyze the Dump with CHAP**:
   ```
   ./chap app.dump
   > list used
   ```
   Example output:
   ```
   Used allocation at 55f387eb2000 of size 28 # size=40
   Used allocation at 7f498db27010 of size a00ff0
   ...
   10 allocations use 0x6e147a0 (115,427,232) bytes.
   ```
5. **Inspect Allocations in `gdb`**:
   ```
   gdb hello_world app.dump
   ```
6. **Check Allocation Totals** (only if TURN_ON_MALLOC_COUNTERS=ON):
   ```
   print TOTAL_ALLOCS
   > $1 = std::atomic<long> = { 5 }
   print TOTAL_ALLOCATED_BYTES
   > $2 = std::atomic<long> = { 115417128 }
   ```
7. **Inspect Specific Memory Locations**:
   ```
   x/s 0x7f498db27010
   > 0x7f498db27010: "big10Mb. A lot of data"
   x/40x 0x55f387eb2000
   > 0x55f387eb2000: 0x19    0x31    0x39    0x72    0x74    0x79    0x75    0x69 # 0x19, '1', '9', 'r', 't', 'y', 'u', 'i', 
   > 0x55f387eb2008: 0x31    0x39    0x19    0x00    0x00    0x00    0x00    0x00 # '1', '9', 0x19
   > 0x55f387eb2010: 0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
   > 0x55f387eb2018: 0x62    0x13    0x1f    0x87    0xf3    0x55    0x00    0x00 # address 0x55f3871f1362
   > 0x55f387eb2020: 0x13    0x00    0x00    0x00    0x00    0x00    0x00    0x00 # 0x13=19 - size of struct my_struct19
   info symbol 0x55f3871f1362
   > main + 258 in section .text of hello_world
   ```
## Additional Notes
Ensure you have the correct permissions when using gcore and gdb for memory analysis.  
The malloc_tracer library should be built in release mode with the `TURN_ON_MALLOC_COUNTERS` flag enabled for tracking allocation statistics.