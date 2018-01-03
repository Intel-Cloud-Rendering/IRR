setup-env() {
    RENDER_HOME=$(pwd)../render
    PORT=23432
    export LD_LIBRARY_PATH=${RENDER_HOME}/boost_1_65_1/stage/lib/:${LD_LIBRARY_PATH}
}
