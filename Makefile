
# This is a GNU Makefile.

# It can be used to compile an OpenCL program with
# the Altera SDK for OpenCL.
# See README.txt for more information.


# You must configure ALTERAOCLSDKROOT to point the root directory of the Altera SDK for OpenCL
# software installation.
# See doc/getting_started.txt for more information on installing and
# configuring the Altera SDK for OpenCL.


TARGET = boardtest_host

# Where is the Altera SDK for OpenCL software?
ifeq ($(wildcard $(ALTERAOCLSDKROOT)),)
$(error Set ALTERAOCLSDKROOT to the root directory of the Altera SDK for OpenCL software installation)
endif
ifeq ($(wildcard $(ALTERAOCLSDKROOT)/host/include/CL/opencl.h),)
$(error Set ALTERAOCLSDKROOT to the root directory of the Altera SDK for OpenCL software installation.)
endif

SRCS = main.cpp \
		memspeed.cpp \
		reorder.cpp  \
		reorder_ocl.cpp \
		hostspeed.cpp \
		hostspeed_ocl.cpp \
		aclutil.cpp \
		timer.cpp \
		rwtest.cpp
SRCS_FILES = $(foreach F, $(SRCS), host/src/$(F))


# arm cross compiler
CROSS-COMPILE = arm-linux-gnueabihf-

# OpenCL compile and link flags.
AOCL_COMPILE_CONFIG=$(shell aocl compile-config --arm) -Ihost/inc
AOCL_LINK_CONFIG=$(shell aocl link-config --arm) -lrt -std=gnu++11

# Make it all!
all : Makefile
	$(CROSS-COMPILE)g++ -fPIC $(SRCS_FILES) -o $(TARGET) -DLINUX $(AOCL_COMPILE_CONFIG) $(AOCL_LINK_CONFIG)

# Standard make targets
clean :
	@rm -f $(OBJS) $(TARGET)
