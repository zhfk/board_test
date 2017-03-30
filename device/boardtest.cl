// Read/write test - Make sure host and kernels can access global memory
// $Header: $
#define V 16
#define REQD_WG_SIZE (1024 * 32)

// Pass arguments arg=1 and arg2=0 to get the intended lsu with ideal access
// pattern

__kernel void 
__attribute((reqd_work_group_size(REQD_WG_SIZE,1,1)))
__attribute((num_vector_lanes(V)))
mem_stream (__global uint *src, __global uint *dst, uint arg, uint arg2)
{
  int gid = get_global_id(0);
  dst[gid]=src[gid];
}

__kernel void 
__attribute((reqd_work_group_size(REQD_WG_SIZE,1,1)))
__attribute((num_vector_lanes(V)))
mem_writestream (__global uint *src, __global uint *dst, uint arg, uint arg2)
{
  int gid = get_global_id(0);
  dst[gid]=gid;
  src[gid]=gid;
}

__kernel void 
__attribute((reqd_work_group_size(REQD_WG_SIZE,1,1)))
__attribute((num_vector_lanes(V)))
mem_burstcoalesced (__global uint *src, __global uint *dst, uint arg, uint arg2)
{
  int gid = get_global_id(0);
  dst[gid+arg2*arg]=src[gid+arg2*arg];
}

__kernel void 
__attribute((reqd_work_group_size(REQD_WG_SIZE,1,1)))
__attribute((num_vector_lanes(V)))
kclk (__global uint *src, __global uint *dst, uint arg, uint arg2)
{
}

__kernel void reorder_const (
    __global uint *dst,
    __global const uint *index,
    __constant uint *src)
{
    uint gID = get_global_id(0);
    uint ndx = index[gID];
    dst[gID] = src[ndx];
}
