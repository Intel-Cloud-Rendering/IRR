#!/bin/bash

## Situation 1: without stream pushing
##   Format: ./run.sh <PORT_NUM>
##   Arguments: <PORT_NUM> is used by rendering server to listen to rendering request, which is sent from render client. (Should be >= 23432)
##   Example: ./run.sh 23432
##
## Situation 2: with stream pushing
##   Format: ./run.sh <PORT_NUM> <STREAM_SERVER_URL>
##   Arguments: <PORT_NUM> is used by rendering server to listen to rendering request, which is sent from render client. (Should be >= 23432)
##              <STREAM_SERVER_URL> is the url of media server to receive pushing stream, it can be udp or rtmp. For rtmp, you may need setup a nginx server. (or likewise servers)
##   Example: ./run.sh 23432 udp://localhost:12345
##            ./run.sh 23432 rtmp://172.31.2.254:1935/demo/1.flv

irr_root=$PWD/..

if [ ! "$#" -eq 1 ] && [ ! "$#" -eq 2 ]; then
    printf "Argument count error! \nExamples: \n  ./run.sh <PORT_NUM>\n  ./run.sh <PORT_NUM> <STREAM_SERVER_URL>\n"
    exit 1
fi

set_env(){
    export DISPLAY=:0
    export LD_LIBRARY_PATH=$irr_root/lib64     ## Add library path here, if the depending lib is not in default search route.
    export render_server_port=$1
    export SHOW_FPS_STATS=true
    export irr_lcd_width=576 #720
    export irr_lcd_height=960 #1280
    export LIBVA_DRIVERS_PATH=/usr/lib/dri     ## Check if iHD_drv_video.so is here, otherwise change to the right path
    export LIBVA_DRIVER_NAME=iHD

    # list out exported env variables
    echo "DISPLAY=$DISPLAY"
    echo $LD_LIBRARY_PATH
    echo "render_server_port=$render_server_port"
    echo "SHOW_FPS_STATS=$SHOW_FPS_STATS"
    echo "intel remote renderer lcd width=$irr_lcd_width"
    echo "intel remote renderer lcd height=$irr_lcd_height"
    glxinfo|egrep "version|render"
}

set_env $1

ulimit -c unlimited
if [ ! "$#" -eq 1 ]; then
## without pushing stream
    ./intel_remote_renderer
else
## with pushing stream
    ./intel_remote_renderer -streaming -url $2 -fr 25
fi


