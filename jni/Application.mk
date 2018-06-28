#APP_PROJECT_PATH := $(call my-dir)
APP_ABI := armeabi armeabi-v7a

APP_MODULES	 +=xwffmpeg
APP_MODULES	 += mtxwamr
APP_MODULES	 += mtxwg729
APP_MODULES	 += SpeechEngineEnhance
APP_MODULES	 += mediatransxw
APP_MODULES	 += mediatransxw_nase
APP_MODULES  += lame
APP_MODULES  += pcmmix

APP_OPTIM        := release 
APP_CFLAGS       += -O3

APP_PLATFORM:=android-14


#APP_STL := stlport_shared 
APP_STL := stlport_static

#STLPORT_FORCE_REBUILD := true

