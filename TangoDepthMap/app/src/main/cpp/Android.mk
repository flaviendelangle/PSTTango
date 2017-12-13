LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../../../userPaths.mk

CVROOT := ${PROJECT_ROOT}/Modules/OpenCV-android-sdk/sdk/native/jni

OPENCV_INSTALL_MODULES := on
OPENCV_LIB_TYPE:=STATIC
include $(CVROOT)/OpenCV.mk

LOCAL_MODULE    := libdepth_map

LOCAL_SHARED_LIBRARIES := tango_client_api tango_support
LOCAL_CFLAGS += -std=c++11 -frtti -fexceptions -fopenmp -w

LOCAL_C_INCLUDES := $(PROJECT_ROOT)/Modules/tango_gl/include/ \
                    $(PROJECT_ROOT)/Modules/third_party/glm/ \
                    $(CVROOT)/include/


LOCAL_SRC_FILES := camera_texture_drawable.cc \
                   color_image.cc \
                   my_color_image.cc\
                   depth_image.cc \
                   jni_interface.cc \
                   tango_depth_map_app.cc \
                   scene.cc \
                   util.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/bounding_box.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/camera.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/conversions.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/cube.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/drawable_object.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/frustum.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/grid.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/line.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/mesh.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/shaders.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/trace.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/transform.cc \
                   $(PROJECT_ROOT)/Modules/tango_gl/src/util.cc

LOCAL_LDLIBS    += -llog -lGLESv2 -L$(SYSROOT)/usr/lib -lz
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT)/Modules)
$(call import-module,tango_client_api)
$(call import-module,tango_support)
