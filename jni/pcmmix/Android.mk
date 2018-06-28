LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_LDLIBS := -llog 

LOCAL_MODULE    := pcmmix

LOCAL_SRC_FILES := PcmMix.cpp \
                   com_mediatransxw_PcmMix.cpp	

include $(BUILD_SHARED_LIBRARY)