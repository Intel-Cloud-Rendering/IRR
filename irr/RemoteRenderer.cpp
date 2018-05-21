// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include <cstdio>
#include <cassert>

#include "android/cmdline-option.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/emuctl-client.h"
#include "android/opengles.h"
#include "android/OpenGLESHostServer.h"
#include "RemoteRenderer.h"
#include "android/streaming/utils.h"
#include "Dump.h"
#include "rpc-thrift/IrrRpcMaintainer.h"

struct SignalWatchDeleter {
    void operator()(android::base::Looper::FdWatch *watch) const;
};

using ScopedSignalWatch = std::unique_ptr<android::base::Looper::FdWatch, SignalWatchDeleter>;
using ScopedSocket = android::base::ScopedSocket;

static irr::IrrConfigPtr sConfig = irr::IrrConfigPtr(new irr::IrrConfig());
static irr::IrrOnPostContext sOnPostCntxt;
static ScopedSocket sRead, sWrite;

void SignalWatchDeleter::operator()(android::base::Looper::FdWatch *watch) const
{
   int fd = watch->fd();
   if (fd > 0){
       close(fd);
   }
   delete watch;
}

static int parse_options(int* p_argc, char*** p_argv, AndroidOptions *opts)
{
    D("argc=%d", *p_argc);

    if (android_parse_options(p_argc, p_argv, opts) < 0){
       fprintf(stderr, "Error during options parsing\n");
       return -1;
    }

    android_cmdLineOptions = opts;

    if (android_cmdLineOptions->streaming)
        D("streaming flag(%d) is %s", android_cmdLineOptions->streaming,
                android_cmdLineOptions->streaming ? "set":"unset");
    if (android_cmdLineOptions->url)
        D("streaming to %s", android_cmdLineOptions->url);
    if (android_cmdLineOptions->b)
        D("streaming with bit rate %s", android_cmdLineOptions->b);
    if (android_cmdLineOptions->codec)
        D("streaming with codec %s", android_cmdLineOptions->codec);
    if (android_cmdLineOptions->fr)
        D("streaming with frame rate %s", android_cmdLineOptions->fr);
    if (android_cmdLineOptions->res) {
        D("rendering and streaming with resolution %s", android_cmdLineOptions->res);
        sscanf(android_cmdLineOptions->res, "%dx%d", &sConfig->hw_lcd_width, &sConfig->hw_lcd_height);
    }
    else{
        sConfig->hw_lcd_width = 576;
        sConfig->hw_lcd_height = 960;
        fprintf(stderr, "Warning: streaming resolution not set, using default 576x960 \n");
    }

    sConfig->is_phone_api = true; // TODO: not used for now
    sConfig->api_level = 27; // just follow the API level used in image creation in avd

    return 0;
}

static void signal_handler(void* opaque, int fd, unsigned events)
{
    int si = 0;
    ssize_t res;
    int* flag = reinterpret_cast<int *>(opaque);
    D("invoked by fd=%d, events=%d", fd, events);
    res = read (fd, &si, sizeof(si));
    if (res != sizeof(si)){
        fprintf (stderr, "Something wrong with signal read\n");
        *flag |= ~0;
        goto Exit;
    }
    D("read signal=%d, flag=%#x", si, *flag);

    switch(si){
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            android::base::ThreadLooper::get()->forceQuit();
            *flag |= 0x2;
            break;
        default:
            fprintf (stderr, "Something wrong with signal handling\n");
            break;
    }
Exit:
    D("return with flag=%#x", *flag);
    return;
}

static void signal_action(int sig, siginfo_t *siginfo, void *context)
{
    D("Getting signal:%d PID:%ld, UID:%ld", sig, (long)siginfo->si_pid, (long)siginfo->si_uid);
    write(sWrite.get(), &sig, sizeof(sig));
}

static int init_signals_handler(void)
{
    sigset_t sigset;
    int err = 0;
    int fd0 = -1, fd1 = -1;
    struct sigaction act;

    if (android::base::socketCreatePair(&fd0, &fd1)<0){
        fprintf (stderr, "Failed to create paired sockets.\n");
        assert(err == 0);
    }
    D("Get socket pair read:%d write:%d", fd0, fd1);

    sRead.reset(fd0);
    sWrite.reset(fd1);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = &signal_action;
    act.sa_flags = SA_SIGINFO;
    err = sigaction(SIGINT, &act, NULL);
    assert(err == 0);
    err = sigaction(SIGTERM, &act, NULL);
    assert(err == 0);
    err = sigaction(SIGQUIT, &act, NULL);
    assert(err == 0);

    return err;
}

static void on_post_callback(void* context, int width,
                           int height, int ydir,
                           int format, int type,
                           unsigned char* pixels)
{
    struct IrrOnPostContext* pCtxt
        = reinterpret_cast<struct IrrOnPostContext*>(context);

    auto dump = std::make_shared<irr::Dump>("RENDERER_FRAME_DUMP_DIR", "frames");
    dump->tryFrame(width, height, ydir, format, type, pixels);

    return;
}

static void on_post_callback2(void* context, int width,
                           int height, int ydir,
                           int format, int type,
                           unsigned char* pixels)
{
    // Simply added here FB copy callback for ffmpeg encoding
    fresh_screen(width, height, pixels);
    return;
}

static void *reqBuffer(void *context, int size)
{
    return irr_get_buffer(size);
}

static void rpc_callback(void* opaque, int fd, unsigned events)
{
    IrrRpcMaintainer *rpc = reinterpret_cast<IrrRpcMaintainer *> (opaque);

    rpc->serve();
}

extern "C" int main(int argc, char** argv)
{
    int ret = 0;
    char* dump_frame_dir = NULL;
    int event_flag = 0;
    ScopedSignalWatch sig_watch;
    AndroidOptions opts[1];
    char *render_port = NULL;
    android::base::Looper::FdWatch *thrift_watch = NULL;
    IrrRpcMaintainer *rpc_mt = NULL;

    D("Hello, this is an intel remote renderer!\n");

    //
    // parse input options
    //
    if (parse_options(&argc, &argv, opts) < 0 ){
        fprintf(stderr, "Failed to initialize GLES emulation.\n");
        ret = -1;
        goto Exit;
    }

    //
    // Initializing signal handling
    //
    ret = init_signals_handler();
    assert(ret == 0);
    sig_watch.reset(android::base::ThreadLooper::get()->createFdWatch(sRead.get(),
                    signal_handler, &event_flag));
    assert(sig_watch.get() != NULL);
    assert(sig_watch->fd() != -1);
    D("Get signal watch fd:%d", sig_watch->fd());
    sig_watch->wantRead();

    XInitThreads();
    //
    // A) Initializing all components
    //
    // A.1) initialize GLES emulation
    if (android_initOpenglesEmulation() != 0) {
        fprintf(stderr, "Failed to initialize GLES emulation.\n");
        ret = -1;
        goto Exit;
    }

    // A.2) start GLES renderer
    if (android_startOpenglesRenderer(sConfig->hw_lcd_width,
                                      sConfig->hw_lcd_height,
                                      sConfig->is_phone_api,
                                      sConfig->api_level)
                                      != 0){
        fprintf(stderr, "Failed to start GLES renderer.\n");
        ret = -1;
        goto Exit;
    }

    //
    // Initilize encoder
    //
    android_setIrrCallback(reqBuffer, on_post_callback2, &sOnPostCntxt);
    register_stream_publishment(sConfig->hw_lcd_width, sConfig->hw_lcd_height, 30.f);

    if (opts->streaming){
        IrrStreamInfo info = { 0 };
        if (opts->fr)
            info.framerate = opts->fr;

        if (opts->b)
            info.bitrate = strtol(opts->b, nullptr, 10);

        info.url   = opts->url;
        info.codec = opts->codec;

        if (opts->lowpower) {
            fprintf(stderr, "Low power enabled.\n");
            info.low_power = 1;
        }
        info.exp_vid_param = opts->exp_vid_param;

        irr_stream_start(&info);
    }
    else{
        dump_frame_dir = getenv("RENDERER_FRAME_DUMP_DIR");
        if (dump_frame_dir){
            android_setPostCallback(on_post_callback, &sOnPostCntxt);
        }
    }

    //
    // Initialize server side opengles socket listener and connection
    //
    render_port = getenv("render_server_port"); // TODO: add command option for this setting
    if (render_port != NULL){
        android_opengles_server_init(atoi(render_port));
    }
    else{
        fprintf(stderr, "render server port is not set in environment variable.\n");
        goto Exit;
    }

    //
    // Initialize RPC listener
    //
    if (opts->rpc_serv_port) {
        try {
            rpc_mt = new IrrRpcMaintainer(atoi(opts->rpc_serv_port));
            if (!rpc_mt) {
                fprintf(stderr, "Fail to create a RPC listener: OOM.\n");
                goto Exit;
            } else {
                D("Starting RPC(thrift) on port %s.", opts->rpc_serv_port);

                thrift_watch = android::base::ThreadLooper::get()->createFdWatch(rpc_mt->getFD(),
                                                                                 rpc_callback, rpc_mt);
                if (!thrift_watch) {
                    fprintf(stderr, "Fail to create a RPC fd watcher: OOM\n");
                    goto Exit;
                } else
                    thrift_watch->wantRead();
            }
        } catch (apache::thrift::transport::TTransportException &ttx) {
            fprintf(stderr, "Fail to create a RPC listener: %s\n", ttx.what());
            goto Exit;
        }
    }

    //
    // Mainloop
    //
    do{
        //TODO: dealing with events, signals, exit...
        // hardcode for now
        if (event_flag != 0){
            D("get exit flag=%#x", event_flag);
            break;
        }
        android::base::ThreadLooper::get()->run();
    }while(1);

    //
    // TODO: need to enhance and handle real error/threads return process here!!!
    //
    android_opengles_server_undo_init();
    unregister_stream_publishment();
    if (thrift_watch) delete thrift_watch;
    if (rpc_mt) delete rpc_mt;

    sleep(1);

    exit(EXIT_SUCCESS);
Exit:
    fprintf (stderr, "Exiting remote renderer process\n");
    exit(ret);
}
