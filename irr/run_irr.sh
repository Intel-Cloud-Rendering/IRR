#!/bin/bash

external_root=/home/jhou15/workspace/src/ssg/android-sc/external
irr_root=/home/jhou15/workspace/src/ssg/android-sc/external/qemu-irr
render_root=/home/jhou15/workspace/src/ssg/android-sc/external/qemu-s
instance_root=/home/jhou15/workspace/src/ssg/android-sc/external/qemu-c
vtune_root=/opt/intel/vtune_amplifier_2018.1.0.535340
ffmpeg_root=/home/jhou15/workspace/src/vcd_gfx-gfxstreamer/source/dcgm-ffmpeg_qsv

# default make intel_remote_renderer
make_irr(){
  echo "make ./objs/intel_remote_renderer"
  make ./objs/intel_remote_renderer
}

# make object by input
make_obj(){
  make $obj
}

# copy related solib to irr/libs
copy_solib(){
    echo "cp -rf $irr_root/objs/build/intermediates64/lib64OpenglRender/lib64OpenglRender.so $irr_root/irr/libs/"
    cp -rf $irr_root/objs/build/intermediates64/lib64OpenglRender/lib64OpenglRender.so $irr_root/irr/libs/
}

# set executable environment
set_env(){
#    export DISPLAY=:0
    export LD_LIBRARY_PATH=$irr_root/objs/build/intermediates64/lib64OpenglRender:$irr_root/objs/build/intermediates64/lib64GLES_CM_translator:$irr_root/objs/build/intermediates64/lib64GLES_V2_translator:$irr_root/objs/build/intermediates64/lib64EGL_translator:/usr/local/lib
#    export ANDROID_EGL_LIB=libEGL
#    export ANDROID_GLESv1_LIB=libGLESv1_CM
#    export ANDROID_GLESv2_LIB=libGLESv2
    export render_server_port=23432
#    export RENDERER_FRAME_DUMP_DIR=$irr_root/tmp
    export SHOW_FPS_STATS=true
    export irr_lcd_width=720
    export irr_lcd_height=1280

    # list out exported env variables
    echo "DISPLAY=$DISPLAY"
    echo $LD_LIBRARY_PATH
#    echo $ANDROID_EGL_LIB
#    echo $ANDROID_GLESv1_LIB
#    echo $ANDROID_GLESv2_LIB
    echo "render_server_port=$render_server_port"
    echo "RENDERER_FRAME_DUMP_DIR=$RENDERER_FRAME_DUMP_DIR"
    echo "SHOW_FPS_STATS=$SHOW_FPS_STATS"
    echo "intel remote renderer lcd width=$irr_lcd_width"
    echo "intel remote renderer lcd height=$irr_lcd_height"
    glxinfo|egrep "version|render"
}

set_env_render(){
    export LD_LIBRARY_PATH=$render_root/objs/lib64/qt/lib:$render_root/objs/lib64/gles_angle11:$render_root/objs/lib64/gles_angle9:$render_root/objs/lib64/gles_angle:$render_root/objs/lib64/gles_swiftshader:$render_root/objs/lib64:.:/usr/lib/x86_64-linux-gnu/libstdc++
    export ANDROID_SDK_ROOT=/home/jhou15/Android/Sdk
    export render_client=0
    export render_server_port=23432
    export render_server=1
    ln -sf /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /home/jhou15/Android/Sdk/emulator/lib64/libstdc++/libstdc++.so.6
    echo $LD_LIBRARY_PATH
    echo "ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
    echo "render_client=$render_client"
    echo "render_server_port=$render_server_port"
    echo "render_server=$render_server"
}

run_render(){
    $render_root/objs/emulator -avd RENDER -use-system-libs
}

rebuild_debug(){
    ./android/rebuild.sh --debug --no-strip --symbols --no-tests
}

set_env_instance(){
    export LD_LIBRARY_PATH=$instance_root/objs/lib64/qt/lib:$instance_root/objs/lib64/gles_angle11:$instance_root/objs/lib64/gles_angle9:$instance_root/objs/lib64/gles_angle:$instance_root/objs/lib64/gles_swiftshader:$instance_root/objs/lib64:.:/usr/lib/x86_64-linux-gnu/libstdc++
    export ANDROID_SDK_ROOT=/home/jhou15/Android/Sdk
    export render_client=1
    export render_server_port=23432
    export render_server=0
    export render_server_hostname=127.0.0.1
    ln -sf /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /home/jhou15/Android/Sdk/emulator/lib64/libstdc++/libstdc++.so.6
    echo $LD_LIBRARY_PATH
    echo "ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
    echo "render_client=$render_client"
    echo "render_server_port=$render_server_port"
    echo "render_server=$render_server"
}

run_instance(){
    $instance_root/objs/emulator -avd INSTANCE -use-system-libs
}

# local install
local_install(){
    copy_solib
}

# print irr private building variables
print_irr_var(){
  make print-IRR_PRIVATE_WHOLE_STATIC_LIBRARIES 
  make print-IRR_PRIVATE_STATIC_LIBRARIES 
  make print-IRR_PRIVATE_CFLAGS 
  make print-IRR_PRIVATE_C_INCLUDES
  make print-IRR_PRIVATE_LDFLAGS 
  make print-IRR_PRIVATE_LDLIBS 
  make print-IRR_PRIVATE_SYMBOL_FILE 
  make print-IRR_PRIVATE_SRC_FILES 
  make print-IRR_PRIVATE_INSTALL_DIR 
}

# gdb irr 
dbg_irr(){
  set_env;
  ulimit -c unlimited
#  gdb -ex start --args "./objs/intel_remote_renderer"
  gdb -x ./irr/gdbinit.txt 
}

# run irr 
run_irr(){
  set_env;
  ulimit -c unlimited
  ./objs/intel_remote_renderer -streaming -url udp://localhost:12345 -fr 25 -b 2M
}

# memory check
memory_check(){
  set_env;
#  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose \
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
      --log-file=$external_root/tmp/vg-mc-out.txt \
      $irr_root/objs/intel_remote_renderer -streaming -url udp://localhost:12345 -fr 25 -b 2M
}

# profiling
amp(){
  . $vtune_root/amplxe-vars.sh 
  amplxe-cl -collect ./qemu-irr/objs/intel_remote_renderer -streaming -url udp://localhost:12345 -fr 20 -b 2M
}

ampg(){
  . $vtune_root/amplxe-vars.sh 
  amplxe-gui
}

ffplay(){
  ./ffplay udp://localhost:12345
}

usage(){
  echo "./irr_run,sh h or -h for help and usage"
  echo "./irr_run,sh m or -m for make intel_remote_renderer" 
  echo "./irr_run,sh mo or -o for make defined objects with path, like ./objs/intel_remote_renderer" 
  echo "./irr_run,sh i or -i for local installation" 
  echo "./irr_run,sh p or -p for printing out compiling options" 
  echo "./irr_run,sh d or -d for running intel_remote_renderer with gdb" 
  echo "./irr_run,sh r or -r for running intel_remote_renderer" 
  echo ". ./irr_run,sh e or -e for setting up environments" 
}

irr_path(){
  pushd $PWD
  cd $irr_root
  echo "running in $PWD"
}
irr_root
render_path(){
  pushd $PWD
  cd $render_root
  echo "running in $PWD"
}

external_path(){
  pushd $PWD
  cd $external_root
  echo "running in $PWD"
}

instance_path(){
  pushd $PWD
  cd $instance_root
  echo "running in $PWD"
}

ffmpeg_path(){
  pushd $PWD
  cd $ffmpeg_root
  echo "running in $PWD"
}

pop_path(){
  popd
}


# if no command options 
if [ $# = 0 ]
then
    option="m"
elif [ -n $1 ] #only accept valid string
then
# otherwise make first arg as a option
    option=$1
fi


if [ -n $2 ]
then
    obj=$2
else
    obj=""
fi

# use case statement to make decision for option
case $option in
    "m"|-m)
        irr_path;make_irr;pop_path;;
    "mo"|-o) 
        irr_path;make_obj;pop_path;;
    "i"|-i)
        irr_path;local_install;pop_path;;
    "p"|-p)
        irr_path;print_irr_var;pop_path;;
    "d"|-d)
        irr_path;dbg_irr;pop_path;;
    "r"|-r)
        irr_path;run_irr;pop_path;;
    "e"|-e)
        irr_path;set_env;pop_path;;
    "h"|-h)
        irr_path;usage;pop_path;;
    "env_instance"|-eI)
        instance_path;set_env_instance;pop_path;;
    "env_render"|-eR)
        render_path;set_env_render;pop_path;;
    "run_instance"|-rI)
        instance_path;run_instance;pop_path;;
    "run_render"|-rR)
        render_path;run_render;pop_path;;
    "go_instance"|-rI)
        instance_path;set_env_instance;run_instance;pop_path;;
    "go_render"|-rR)
        render_path;set_env_render;run_render;pop_path;;
    "rebuild_instance"|-bI)
        instance_path;rebuild_debug;pop_path;;
    "rebuild_render"|-bR)
        render_path;rebuild_debug;pop_path;;
    "rebuild_irr"|-bIRR)
        irr_path;rebuild_debug;pop_path;;
    "amp"|-amp)
        external_path;amp;pop_path;;
    "ampg"|-ampg)
        external_path;ampg;pop_path;;
    "ffp"|-ffp)
        ffmpeg_path;ffplay;pop_path;;
    "mc"|-mc)
        external_path;memory_check;pop_path;;
esac

