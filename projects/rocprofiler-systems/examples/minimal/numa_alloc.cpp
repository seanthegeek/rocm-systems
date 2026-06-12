// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT

#include <numa.h>

// Generates a numa_alloc and numa_free region with the same "size" trace-arg of 4096
int
main()
{
    void* ptr = numa_alloc(4096);
    numa_free(ptr, 4096);
    return 0;
}
