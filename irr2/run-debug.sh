RENDER_HOME=$(pwd)
echo RENDER_HOME=$RENDER_HOME
PORT=23432
export LD_LIBRARY_PATH=${RENDER_HOME}/boost_1_65_1/stage/lib/:${RENDER_HOME}/lib64/:${LD_LIBRARY_PATH}
unset HTTP_PROXY HTTPS_PROXY FTP_PROXY ALL_PROXY NO_PROXY SOCKS_PROXY
unset http_proxy https_proxy ftp_proxy all_proxy no_proxy socks_proxy

# CMD FORMAT:
# ./bin/RenderServer ${PORT} -url udp://localhost:12345 [-b 2M | -codec h264_qsv | -gop 50 | -fr 25 | -res 480x640]
gdb --args ./bin/RenderServer ${PORT} -url udp://localhost:12345
