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

static const size_t total_bytes_used = 1024 * 1024 * 32;

static const size_t V = 16;
static const size_t wgSize = 1024 * 32;
static const size_t vectorSize = total_bytes_used / sizeof(unsigned) / 2;

static const size_t NUM_DIMMS = 8;
static const size_t NUM_KERNELS = 4;
static const char *kernel_name[] = {"kclk",
                            "mem_stream",
                            "mem_writestream",
                            "mem_burstcoalesced"};

float bw[NUM_KERNELS][NUM_DIMMS];

// ACL runtime configuration
static cl_platform_id platform;
static cl_device_id device;
static cl_context context;
static cl_command_queue queue;
static cl_kernel kernel[NUM_KERNELS];
static cl_program program;
static cl_int status;

static cl_mem ddatain, ddataout;

// input and output vectors
static unsigned *hdatain, *hdataout;

static void initializeVector(unsigned* vector, int size) {
  for (int i = 0; i < size; ++i) {
    vector[i] = rand();
  }
}

static void dump_error(const char *str, cl_int status) {
  printf("%s\n", str);
  printf("Error code: %d\n", status);
}

// free the resources allocated during initialization
static void freeResources() {
  for (int k = 0; k < NUM_KERNELS; k++)
    if(kernel[k]) 
      clReleaseKernel(kernel[k]);  
  if(ddatain) 
    clReleaseMemObject(ddatain);
  if(ddataout) 
    clReleaseMemObject(ddataout);
  if(hdatain) 
    acl_aligned_free(hdatain);
  if(hdataout) 
    acl_aligned_free(hdataout);
}

int memspeed (
    cl_platform_id in_platform,
    cl_device_id in_device,
    cl_context in_context,
    cl_command_queue in_queue
    ) {

  platform = in_platform;
  device = in_device;
  context = in_context;
  queue = in_queue;

  unsigned char* aocx; size_t aocx_len = 0;
  aocx = load_file("boardtest.aocx",&aocx_len); 
  if (aocx == NULL) 
  {
    printf("Error: Failed to find boardtest.aocx\n");
    exit(-1);
  }

  // create the program
  cl_int kernel_status;
  program = clCreateProgramWithBinary(context, 1, &device,
      &aocx_len, (const unsigned char **)&aocx, &kernel_status, &status);
  if(status != CL_SUCCESS) {
    dump_error("Failed clCreateProgramWithBinary.", status);
    freeResources();
    return 1;
  }

  // build the program
  status = clBuildProgram(program, 0, NULL, "",
      NULL, NULL);
  if(status != CL_SUCCESS) {
    dump_error("Failed clBuildProgram.",
        status);
    freeResources();
    return 1;
  }

  free(aocx);

  printf("Performing kernel transfers of %d MBs\n",sizeof(unsigned) * vectorSize * 2 / 1024 /1024);
  printf("  Note: This test assumes each memory bank is no smaller than\n");
  printf("  %d MBs and the design was compiled with --sw-dimm-partition\n\n",sizeof(unsigned) * vectorSize * 2 / 1024 / 1024);

  // allocate and initialize the input vectors
  hdatain = (unsigned *)acl_aligned_malloc(sizeof(unsigned) * vectorSize);
  hdataout = (unsigned *)acl_aligned_malloc(sizeof(unsigned) * vectorSize);

  initializeVector(hdatain, vectorSize);
  initializeVector(hdataout, vectorSize);

  for ( int k = 0; k < NUM_KERNELS; k++)
  {

    //printf("Creating kernel %d (%s)\n",k,kernel_name[k]);

    // create the kernel
    kernel[k] = clCreateKernel(program, kernel_name[k], &status);
    if(status != CL_SUCCESS) {
      dump_error("Failed clCreateKernel.", status);
      freeResources();
      return 1;
    }

    printf("Launching kernel %s ...\n",kernel_name[k]);


    for (int b = 0; b < NUM_DIMMS; b++)
    {
      cl_int memflags=0;

      switch(b)
      {
        case 0: memflags |= CL_MEM_BANK_1_ALTERA; break;
        case 1: memflags |= CL_MEM_BANK_2_ALTERA; break;
        case 2: memflags |= CL_MEM_BANK_3_ALTERA; break;
        case 3: memflags |= CL_MEM_BANK_4_ALTERA; break;
        case 4: memflags |= CL_MEM_BANK_5_ALTERA; break;
        case 5: memflags |= CL_MEM_BANK_6_ALTERA; break;
        case 6: memflags |= CL_MEM_BANK_7_ALTERA; break;
                //case 7: There is no CL_MEM_BANK_8_ALTERA!!!!
        default: memflags |= CL_MEM_BANK_1_ALTERA; break;
      }

      // create the input buffer
      ddatain = clCreateBuffer(context, CL_MEM_READ_WRITE | memflags, sizeof(unsigned) * vectorSize, NULL, &status);
      if(status != CL_SUCCESS) {
        dump_error("Failed clCreateBuffer.", status);
        freeResources();
        return 1;
      }

      // create the input buffer
      ddataout = clCreateBuffer(context, CL_MEM_READ_WRITE | memflags, sizeof(unsigned) * vectorSize, NULL, &status);
      if(status != CL_SUCCESS) {
        dump_error("Failed clCreateBuffer.", status);
        freeResources();
        return 1;
      }

      // set the arguments
      status = clSetKernelArg(kernel[k], 0, sizeof(cl_mem), (void*)&ddatain);
      if(status != CL_SUCCESS) {
        dump_error("Failed set arg 0.", status);
        return 1;
      }
      status = clSetKernelArg(kernel[k], 1, sizeof(cl_mem), (void*)&ddataout);
      if(status != CL_SUCCESS) {
        dump_error("Failed Set arg 1.", status);
        freeResources();
        return 1;
      }
      unsigned int arg=1;
      status = clSetKernelArg(kernel[k], 2, sizeof(unsigned int), &arg);
      unsigned int arg2=0;
      status |= clSetKernelArg(kernel[k], 3, sizeof(unsigned int), &arg2);
      if(status != CL_SUCCESS) {
        dump_error("Failed Set arg 2 and/or 3.", status);
        freeResources();
        return 1;
      }

      status = clEnqueueWriteBuffer(queue, ddatain, CL_FALSE, 0, sizeof(unsigned) * vectorSize, hdatain, 0, NULL, NULL);
      if(status != CL_SUCCESS) {
        dump_error("Failed to enqueue buffer kernelX.", status);
        freeResources();
        return 1;
      }

      status = clEnqueueWriteBuffer(queue, ddataout, CL_FALSE, 0, sizeof(unsigned) * vectorSize, hdataout, 0, NULL, NULL);
      if(status != CL_SUCCESS) {
        dump_error("Failed to enqueue buffer kernelY.", status);
        freeResources();
        return 1;
      }

      clFinish(queue);

      // launch kernel
      //size_t gsize = vectorSize / V;
      size_t gsize = vectorSize;
      size_t lsize = wgSize;
      Timer t;

      t.start();
      status = clEnqueueNDRangeKernel(queue, kernel[k], 1, NULL, &gsize, &lsize, 0, NULL, NULL);
      if (status != CL_SUCCESS) {
        dump_error("Failed to launch kernel.", status);
        freeResources();
        return 1;
      }
      clFinish(queue);
      t.stop();
      float time = t.get_time_s();
      bw[k][b] = gsize  / (time * 1000000.0f) * sizeof(unsigned int) * 2;

      if (strcmp(kernel_name[k],"kclk") != 0)
      {

        // read the input
        status = clEnqueueReadBuffer(queue, ddatain, CL_TRUE, 0, sizeof(unsigned) * vectorSize, hdatain, 0, NULL, NULL);
        if(status != CL_SUCCESS) {
          dump_error("Failed to enqueue buffer read.", status);
          freeResources();
          return 1;
        }

        // read the output
        status = clEnqueueReadBuffer(queue, ddataout, CL_TRUE, 0, sizeof(unsigned) * vectorSize, hdataout, 0, NULL, NULL);
        if(status != CL_SUCCESS) {
          dump_error("Failed to enqueue buffer read.", status);
          freeResources();
          return 1;
        }

        // verify the output
        for(int i = 0; i < vectorSize; i++) {
          if(hdatain[i] != hdataout[i]) {
            printf("Verification failed %d: %08x != %08x xor=%08x\n",
                i, hdatain[i], hdataout[i], hdatain[i]^hdataout[i]);
            return 1;
          }
        }
      }

      if(ddatain) 
        clReleaseMemObject(ddatain);
      if(ddataout) 
        clReleaseMemObject(ddataout);
    }
  }

  // Ignore last kernel - the kclk kernel is only there to make sure the
  // kernel even can saturate memory bandwidth

  printf("\nSummarizing bandwidth in MB/s/bank for banks 1 to %d\n",NUM_DIMMS);
  float avg_bw=0.0;
  for ( int k = 1; k < NUM_KERNELS; k++)
  {
    for ( int b = 0; b < NUM_DIMMS; b++)
    {
      printf(" %.0f ",bw[k][b]);
      avg_bw += bw[k][b];
    }
    printf(" %s\n",kernel_name[k]);
  }
  avg_bw /= NUM_DIMMS * (NUM_KERNELS - 1);
  printf("\n");

  printf("  Kernel mem bandwidth assuming ideal memory: %.0f MB/s\n",bw[0][NUM_DIMMS-1]);
  printf("              * If this is lower than your board's peak memory\n");
  printf("              * bandwidth then your kernel's clock isn't fast enough\n");
  printf("              * to saturate memory\n");
  //printf("              *   approx. fmax = %.2f\n",bw[0][NUM_DIMMS-1]/sizeof(unsigned int)/2.0/V);

  printf("\n");
  printf("\nKERNEL-TO-MEMORY BANDWIDTH = %.0f MB/s/bank\n",avg_bw);

  // free the resources allocated
  freeResources();

  return 0;
}

