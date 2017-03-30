// (C) 1992-2014 Altera Corporation. All rights reserved.                         
// Your use of Altera Corporation's design tools, logic functions and other       
// software and tools, and its AMPP partner logic functions, and any output       
// files any of the foregoing (including device programming or simulation         
// files), and any associated documentation or information are expressly subject  
// to the terms and conditions of the Altera Program License Subscription         
// Agreement, Altera MegaCore Function License Agreement, or other applicable     
// license agreement, including, without limitation, that your use is for the     
// sole purpose of programming logic devices manufactured by Altera and sold by   
// Altera or its authorized distributors.  Please refer to the applicable         
// agreement for further details.                                                 
    

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ACL specific includes
#include "CL/opencl.h"
#include "aclutil.h"
#include "timer.h"

int memspeed (
    cl_platform_id platform,
    cl_device_id device,
    cl_context context,
    cl_command_queue queue);
int reorder (int DEFAULT_ARRAYSIZE, int DEFAULT_SUBARRAYSIZE, int DEFAULT_ITERATIONS,
    cl_platform_id in_platform,
    cl_device_id in_device,
    cl_context in_context,
    cl_command_queue in_queue);
int hostspeed(
    cl_platform_id platform,
    cl_device_id device,
    cl_context context,
    cl_command_queue queue);
int rwtest(
    cl_platform_id platform,
    cl_device_id device,
    cl_context context,
    cl_command_queue queue);
void ocl_notify(
    const char *errinfo, 
    const void *private_info, 
    size_t cb, 
    void *user_data);


// ACL runtime configuration
static cl_platform_id platform;
static cl_device_id device;
static cl_context context;
static cl_command_queue queue;
static cl_int status;

// Set to false to temporarily disable printing of error notification callbacks
bool g_enable_notifications = true;
void ocl_notify(
    const char *errinfo, 
    const void *private_info, 
    size_t cb, 
    void *user_data) {
  if(g_enable_notifications) {
    printf("  OpenCL Notification Callback:");
    printf(" %s\n", errinfo);
  }
}

static void dump_error(const char *str, cl_int status) {
  printf("%s\n", str);
  printf("Error code: %d\n", status);
}


static void dump_init_error() {
  printf("Failed to initialize the device.  Please check the following:\n");
  printf("  1. The card is visible to your host operating system\n");
  printf("  2. There is a valid OpenCL design currently configured on the card\n");
  printf("  3. You've installed all necessary drivers\n");
  printf("  4. You've linked the host program with the correct libraries for your specific card\n");
};

// free the resources allocated during initialization
static void freeResources() {
  if(queue) 
    clReleaseCommandQueue(queue);
  if(context) 
    clReleaseContext(context);
}

int main(int argc, char *argv[]) {
  cl_uint num_platforms;
  cl_uint num_devices;

  int test_to_run = 0; // This means run all tests

  // Don't buffer stdout
  setbuf(stdout, NULL);

  if (argc == 2 )
  {
    test_to_run = atoi(argv[1]);
    printf("Running only test %d\n",test_to_run);
  }

  // get the platform ID
  status = clGetPlatformIDs(1, &platform, &num_platforms);
  if(status != CL_SUCCESS) {
    dump_error("Failed clGetPlatformIDs.", status);
    dump_init_error();
    freeResources();
    return 1;
  }
  if(num_platforms != 1) {
    printf("Found %d platforms!\n", num_platforms);
    dump_init_error();
    freeResources();
    return 1;
  }

  // get the device ID
  status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &num_devices);
  if(status != CL_SUCCESS) {
    dump_error("Failed clGetDeviceIDs.", status);
    dump_init_error();
    freeResources();
    return 1;
  }
  if(num_devices != 1) {
    printf("Warning: Found %d OpenCL Devices, using the first one!\n", num_devices);
  }

  // create a context
  context = clCreateContext(0, 1, &device, &ocl_notify, NULL, &status);
  if(status != CL_SUCCESS) {
    dump_error("Failed clCreateContext.", status);
    freeResources();
    return 1;
  }

  // create a command queue
  queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
  if(status != CL_SUCCESS) {
    dump_error("Failed clCreateCommandQueue.", status);
    freeResources();
    return 1;
  }

  int ret = 0;

  if ( test_to_run == 0 || test_to_run == 1)
  {
    printf("\n");
    printf("*****************************************************************\n");
    printf("*********************** Host Speed Test *************************\n");
    printf("*****************************************************************\n");
    printf("\n");

    ret |= hostspeed(platform,device,context,queue);

    printf("\n");
    printf("*****************************************************************\n");
    printf("********************* Host Read Write Test **********************\n");
    printf("*****************************************************************\n");
    printf("\n");

    ret |= rwtest(platform,device,context,queue);
  }

  if ( test_to_run == 0 || test_to_run == 2)
  {
    printf("\n");
    printf("*****************************************************************\n");
    printf("*****************  Kernel-to-Memory Bandwidth   *****************\n");
    printf("*****************************************************************\n");
    printf("\n");

    ret |= memspeed(platform,device,context,queue);
  }

  if ( test_to_run == 0 || test_to_run == 3)
  {
    printf("\n");
    printf("*****************************************************************\n");
    printf("*********************** Cache Snoop Test ************************\n");
    printf("*****************************************************************\n");
    printf("\n");

    int r=reorder(4096,4096,4000,platform,device,context,queue);
    if (r==0)
      printf("\nSNOOP TEST PASSED\n");
    ret |= r;
  }


  // free the resources allocated
  freeResources();

  return ret;
}

