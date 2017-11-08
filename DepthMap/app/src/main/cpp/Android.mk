LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../../../userPaths.mk

CVROOT := ${PROJECT_ROOT}/Modules/OpenCV-android-sdk/sdk/native/jni

include $(CLEAR_VARS)

OPENCV_INSTALL_MODULES := on
OPENCV_LIB_TYPE:=STATIC
include $(CVROOT)/OpenCV.mk

LOCAL_MODULE    := libdepth_map
LOCAL_SHARED_LIBRARIES := tango_client_api tango_support
LOCAL_CFLAGS += -std=c++11 -frtti -fexceptions -fopenmp -w
LOCAL_SRC_FILES := jni_interface.cpp \
                   Depth_map.cpp \
                   YUVDrawable.cpp \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/drawable_object.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/shaders.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/transform.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/util.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/video_overlay.cc

LOCAL_C_INCLUDES := $(PROJECT_ROOT)/Modules/tango_gl/include \
                    $(PROJECT_ROOT)/Modules/third_party/glm \
                    $(CVROOT)/include

LOCAL_LDLIBS    := -llog -lGLESv3 -L$(SYSROOT)/usr/lib -lz
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT)/Modules)
$(call import-module,tango_client_api)
$(call import-module,tango_support)