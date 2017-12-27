#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>


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
#include "Dump.h"

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
    if (android_cmdLineOptions->res)
        D("streaming with output resolution %s", android_cmdLineOptions->res);

    // hard code initialize value, need a parse function here
    sConfig->hw_lcd_width = atoi(getenv("irr_lcd_width"));
    sConfig->hw_lcd_height = atoi(getenv("irr_lcd_height"));
    if ((0 == sConfig->hw_lcd_width) || (0 == sConfig->hw_lcd_height)){
        fprintf(stderr, "Warning: invalid lcd width=%d or height=%d, force to 1080p\n",
                sConfig->hw_lcd_width, sConfig->hw_lcd_height);
        sConfig->hw_lcd_width = 1920;
        sConfig->hw_lcd_height = 1080;
    }
    sConfig->is_phone_api = true;
    sConfig->api_level = 1;

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

extern "C" int main(int argc, char** argv)
{
    int ret = 0;
    char* dump_frame_dir = NULL;
    int count = 0;
    int event_flag = 0;
    ScopedSignalWatch sig_watch;
    AndroidOptions opts[1];

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
    if (opts->streaming){
        android_emuctl_client_init();
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
    android_opengles_server_init(atoi(getenv("render_server_port")));

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
    // TODO: need to enhance and handle real threads return process here!!!
    //
    android_opengles_server_undo_init();
    sleep(1);

    exit(EXIT_SUCCESS);
Exit:
    fprintf (stderr, "Exiting remote renderer process\n");
    exit(ret);
}
