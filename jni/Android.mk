LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH :=$(call my-dir)


#================================xwffmpeg static lib===============================================

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)

PATH_TO_FFMPEG := $(LOCAL_PATH)/thirdlib/ffmpeg



LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libavcodec
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libavdevice
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libavfilter
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libavformat
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libavutil
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libpostproc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libswresample
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ffmpeg/libswscale


LOCAL_LDLIBS := -llog -lz
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavcodec.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavdevice.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavfilter.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavformat.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavresample.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libavutil.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libpostproc.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libswresample.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libswscale.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/libx264.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/aac/libfdk-aac.a 
LOCAL_LDLIBS += $(PATH_TO_FFMPEG)/lame/libmp3lame.a

LOCAL_SRC_FILES := src/mtxw_ffmpeg_decoder.c src/mtxw_ffmpeg_encoder.c

LOCAL_MODULE := xwffmpeg

include $(BUILD_SHARED_LIBRARY)


#================================g729 static lib===============================================

include $(CLEAR_VARS)


LOCAL_PATH:= $(MY_LOCAL_PATH)

MY_CPP_LIST := $(wildcard $(LOCAL_PATH)/src/g729/*.c*)

LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/g729

LOCAL_CFLAGS = -O3

LOCAL_ARM_MODE := arm

LOCAL_MODULE    := mtxwg729

include $(BUILD_STATIC_LIBRARY)

#================================amr static lib===============================================

include $(CLEAR_VARS)

LOCAL_PATH:= $(MY_LOCAL_PATH)

#LOCAL_MODULE    := mtxwamr

MY_CPP_LIST := $(wildcard $(LOCAL_PATH)/src/amr/*.cpp)

LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/amr

LOCAL_CFLAGS = -O3

LOCAL_ARM_MODE := arm

LOCAL_MODULE    := mtxwamr

include $(BUILD_STATIC_LIBRARY)


#================================mediatransxw shared lib===============================================
include $(CLEAR_VARS)

LOCAL_PATH:= $(MY_LOCAL_PATH)

LOCAL_LDLIBS := -llog 
LOCAL_LDLIBS += -pthread
LOCAL_STATIC_LIBRARIES := mtxwg729 mtxwamr


LOCAL_SHARED_LIBRARIES := SpeechEngineEnhance xwffmpeg

LOCAL_MODULE    := mediatransxw

LOCAL_SRC_FILES := src/com_mediatransxw_MediaTransXW.cpp \
                   src/mtxw_amrrtp.cpp \
                   src/mtxw_audioinstance.cpp \
                   src/mtxw_comm.cpp \
                   src/mtxw_controlblock.cpp \
                   src/mtxw_g729rtp.cpp \
                   src/mtxw_h264rtp.cpp \
                   src/mtxw_instance.cpp \
                   src/mtxw_rtp.cpp \
                   src/mtxw_videoinstance.cpp \
                   src/mtxw_jitter.cpp \
				   src/Crc8.cpp \
				   src/Galois.cpp \
				   src/rtpfec.cpp \
                   src/mtxw_SequenceParameterSet.cpp 
                   
LOCAL_C_INCLUDES :=  \
					 $(LOCAL_PATH)/include 
					 
LOCAL_CFLAGS   += -std=c++11
		

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_PATH:= $(MY_LOCAL_PATH)

LOCAL_LDLIBS := -llog 
LOCAL_LDLIBS += -pthread

LOCAL_STATIC_LIBRARIES := mtxwg729 mtxwamr xwffmpeg

LOCAL_SHARED_LIBRARIES := xwffmpeg

LOCAL_MODULE    := mediatransxw_nase

LOCAL_SRC_FILES := src/com_mediatransxw_MediaTransXW.cpp \
                   src/mtxw_amrrtp.cpp \
                   src/mtxw_audioinstance.cpp \
                   src/mtxw_comm.cpp \
                   src/mtxw_controlblock.cpp \
                   src/mtxw_g729rtp.cpp \
                   src/mtxw_h264rtp.cpp \
                   src/mtxw_instance.cpp \
                   src/mtxw_rtp.cpp \
                   src/mtxw_videoinstance.cpp \
                   src/mtxw_jitter.cpp \
				   src/Crc8.cpp \
				   src/Galois.cpp \
				   src/rtpfec.cpp \
                   src/mtxw_SequenceParameterSet.cpp 
                   
LOCAL_C_INCLUDES :=  \
					 $(LOCAL_PATH)/include 
					 
					 
LOCAL_CFLAGS   += -std=c++11 -DMTXW_NASE
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/prebuilt/Android.mk

#================================lame shared lib===============================================
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)

include $(LOCAL_PATH)/lame/Android.mk

#================================pcmmix shared lib==============================================
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)

include $(LOCAL_PATH)/pcmmix/Android.mk