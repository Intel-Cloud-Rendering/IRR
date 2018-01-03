RENDER_HOME=$(pwd)
echo RENDER_HOME=$RENDER_HOME
PORT=23432
export LD_LIBRARY_PATH=${RENDER_HOME}/boost_1_65_1/stage/lib/:${RENDER_HOME}/lib64/:${LD_LIBRARY_PATH}
unset HTTP_PROXY HTTPS_PROXY FTP_PROXY ALL_PROXY NO_PROXY SOCKS_PROXY
unset http_proxy https_proxy ftp_proxy all_proxy no_proxy socks_proxy
./bin/RenderServer ${PORT}
