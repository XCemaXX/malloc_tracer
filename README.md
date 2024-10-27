# malloc_tracer

## Overview
`malloc_tracer` is a dynamic library that enables in-depth analysis of memory allocation patterns in applications by tracking `malloc`, `calloc`, `realloc`, and `free` operations.

## How to Build
1. **Clone the repository** and navigate to the project directory.
2. **Build the library** using `cmake`:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cmake --install build --prefix ./output
   ```
3. **Extra flags** (optional):
   ```
   -DTURN_ON_MALLOC_COUNTERS=ON # count allocs
   -DDEBUG=ON # print allocs events
   ```
4. **Example Build**. You can also build with the hello_world example:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_HELLO_WORLD=ON
   ```

## How to Use
1. **Preload the library** to track allocations in your application:
   ```
   LD_PRELOAD=/full/path/to/libmalloc_tracer.so ./example_app
   ```
2. **Capture a memory dump** of the running application:
   ```
   gcore -o app.dump $(pgrep example_app)
   ```

## Memory Dump Analysis

### Recommended Method with GDB Plugin
1. Use [CHAP](https://github.com/vmware/chap) to write to a file:
   ```
   ./chap app.dump
   > redirect on
   > list used
   ```
   Allocations will be saved to `app.dump.list_used`.
2. Load the dump into `gdb` with a Python plugin for automated analysis:
   ```
   gdb ./example_app ./app.dump -ex "python chap_file='app.dump.list_allocations'" -x /abs/path/to/gdb_plugin/gdb_malloc_tracer
   ```
3. Run the `heap_total` command in `gdb`.  

### Hardcore Method with Vanilla GDB
1. Use [CHAP](https://github.com/vmware/chap) to list memory allocations:
   ```
   ./chap app.dump
   > list used
   ```
2. Use `gdb` to inspect the allocations:
   ```
   gdb example_app app.dump
   ```

## Example Walkthrough
1. **Build the example** and download/build [chap](https://github.com/vmware/chap):
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
   Example output:  
   `Pid 2137496 0 big100Mb is 0x7f419fe6d010; addr10Mb is 0x7f419f46c010`
3. **Create a Dump**:
   ```
   sudo gcore -o app.dump $(pgrep hello_world) # make dump
   sudo chmod 0777 app.dump
   mv app.dump.2137496 app.dump
   ```

### Memory Dump Analysis
#### Recommended Method with GDB Plugin
4. **Analyze the Dump with CHAP**:
   ```
   ./chap app.dump
   > redirect on
   > list allocations
   ```
   File `app.dump.list_allocations` will be created.  
5. **Open `gdb` with plugin**:
   ```
   gdb ./hello_world ./app.dump -ex "python chap_file='app.dump.list_allocations'" -x /abs/path/to/gdb_plugin/gdb_malloc_tracer
   # same as
   gdb ./hello_world ./app.dump -x /abs/path/to/gdb_plugin/gdb_malloc_tracer
   (gdb) set_chap app.dump.list_allocations
   ```
6. **Check Allocations**:
   Get total memory allocation sizes:
   ```
   heap_total
   > ### Total memory allocations Sizes: User=110.07MB, Mem=110.08MB, Chunk=110.08MB; Count=71; PerAllocMean=1625612.21b
   > Lib: "/output/hello_world": Sizes: User=110.0MB, Mem=110.01MB, Chunk=110.01MB; Count=66; PerAllocMean=1747647.56b
   > Lib: "/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.30": Sizes: User=71.0KB, Mem=71.01KB, Chunk=71.02KB; Count=1; PerAllocMean=72704.00b
   > Lib: "/lib/x86_64-linux-gnu/libc.so.6": Sizes: User=1.0KB, Mem=1.01KB, Chunk=1.02KB; Count=1; PerAllocMean=1024.00b
   > Lib: "Unknown lib": Sizes: User=0B, Mem=1.62KB, Chunk=1.66KB; Count=3; PerAllocMean=0.00b
   ```
   Inspect the elf:
   ```
   heap_total "/output/hello_world"
   > ### Memory allocations for library "/output/hello_world". Total Sizes: User=110.0MB, Mem=110.01MB, Chunk=110.01MB; Count=66; PerAllocMean=1747647.56b
   > Func: "main": Sizes: User=110.0MB, Mem=110.01MB, Chunk=110.01MB; Count=5; PerAllocMean=23068707.80b
   > Func: "make_vectors(int)": Sizes: User=1.17KB, Mem=1.18KB, Chunk=2.13KB; Count=61; PerAllocMean=19.67b
   ```
   Inspect the function:
   ```
   heap_total "/output/hello_world" "make_vectors(int)"
   > ###  Memory allocations for function "make_vectors(int)" in library "/output/hello_world": Sizes: User=1.17KB, Mem=1.18KB, Chunk=2.13KB; Count=61; PerAllocMean=19.67b
   > RetAddr: 0x55bf30f74325, FuncOffset+165, Sizes: User=720.0B, Mem=720.0B, Chunk=1.17KB; Count=30; PerAllocMean=24.00b
   > RetAddr: 0x55bf30f742c8, FuncOffset+72, Sizes: User=240.0B, Mem=248.0B, Chunk=264.0B; Count=1; PerAllocMean=240.00b
   > RetAddr: 0x55bf30f7430d, FuncOffset+141, Sizes: User=240.0B, Mem=240.0B, Chunk=720.0B; Count=30; PerAllocMean=8.00b

   heap_addrs "/output/hello_world" "main"
   > ###  Memory allocations for function "main" in library "/output/hello_world": Sizes: User=110.0MB, Mem=110.01MB, Chunk=110.01MB; Count=5; PerAllocMean=23068707.80b
   > RetAddr: 0x55bf30f74696, FuncOffset+374, Sizes: User=100.0MB, Mem=100.0MB, Chunk=100.0MB; Count=1; PerAllocMean=104857600.00b
   >   Data addresses: 0x7f419fe6d010 # <---- will inspect on next step
   > ...
   > RetAddr: 0x55bf30f74622, FuncOffset+258, Sizes: User=19.0B, Mem=24.0B, Chunk=40.0B; Count=1; PerAllocMean=19.00b
   >   Data addresses: 0x55bf31269000 # <---- will inspect on next step
   ```
7. **Inspect Specific Memory Locations**:
   ```
   hexdump 0x7f419fe6d010 -s 2
   > 0x00007f419fe6d010 -> 0x00007f419fe6d02f big100Mb. A lot of data.........|
   > 0x00007f419fe6d030 -> 0x00007f419fe6d04f ................................|

   heap_one 0x55bf31269000
   > Data address: 0x55bf31269000
   > RetAddr: 0x55bf30f74622, Sizes: User=19.0B, Mem=24.0B, Chunk=40.0B; Count=1; PerAllocMean=19.00b
   > Lib: /output/hello_world
   > Func: main+258
   > 0x000055bf31269000 -> 0x000055bf3126901f 19 31 39 72 74 79 75 69 31 39 19 00 00 00 00 00 00 00 00 00 00 00 00 00 22 46 f7 30 bf 55 00 00
                                              |.19rtyui19.............."F.0.U..|
   > 0x000055bf31269020 -> 0x000055bf3126903f 13 00 00 00 00 00 00 00 c1 03 00 00 00 00 00 00 69 12 f3 5b 05 00 00 00 8e e0 05 12 8b d6 f7 3e
                                              |................i..[...........>|
   ```
   Last one is `struct my_struct19` with size 0x13=19. Return address "22 46 f7 30 bf 55" ~ 0x55bf30f74622.

#### Hardcore Method with Vanilla GDB
4. **Analyze the Dump with CHAP**:
   ```
   ./chap app.dump
   > list used
   ```
   Example output:
   ```
   Used allocation at 55bf31268f50 of size 38 # size=56
   Used allocation at 7f419f46c010 of size a00ff0
   ...
   71 allocations use 0x6e15028 (115,429,416) bytes.
   ```
5. **Inspect Allocations in `gdb`**:
   ```
   gdb hello_world app.dump
   ```
6. **Check Allocation Totals** (only if `TURN_ON_MALLOC_COUNTERS=ON`):
   ```
   print TOTAL_ALLOCS
   > $1 = std::atomic<long> = { 66 }
   print TOTAL_ALLOCATED_BYTES
   > $2 = std::atomic<long> = { 115418328 }
   ```
7. **Inspect Specific Memory Locations**:
   ```
   x/s 0x7f419f46c010
   > 0x7f498db27010: "big10Mb. A lot of data"
   x/40x 0x55bf31268f50
   > 0x55bf31268f50: 0x50    0x00    0x00    0x00    0x00    0x00    0x00    0x00 # 0x50 (start of my_struct40)
   > 0x55bf31268f58: 0x68    0x8f    0x26    0x31    0xbf    0x55    0x00    0x00 # pointer to bytes in std::string. Check 4th line (0x55bf31268f68)
   > 0x55bf31268f60: 0x07    0x00    0x00    0x00    0x00    0x00    0x00    0x00
   > 0x55bf31268f68: 0x31    0x32    0x33    0x34    0x35    0x36    0x37    0x00 # "1234567"
   > 0x55bf31268f70: 0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
   > 0x55bf31268f78: 0xc9    0x45    0xf7    0x30    0xbf    0x55    0x00    0x00 # address there (0x55bf30f745c9) allocation is called
   > 0x55bf31268f80: 0x28    0x00    0x00    0x00    0x00    0x00    0x00    0x00 # 0x28=40 - size of struct my_struct40
   info symbol 0x55bf30f745c9
   main + 169 in section .text of hello_world
   ```

## Additional Notes
Ensure you have the correct permissions when using gcore and gdb for memory analysis.  
The malloc_tracer library should be built in release mode with the `TURN_ON_MALLOC_COUNTERS` flag enabled for tracking allocation statistics.

