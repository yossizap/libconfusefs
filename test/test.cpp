/*
 * Async API sanity test server
*/

#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <fstream>
#include <thread>

#include <fuse.h>
#include <fuse_lowlevel.h>
#include <uv.h>

#include "confusefs.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using json_pointer = nlohmann::json_pointer<json>;
using namespace std;

static void on_uv_walk(uv_handle_t* handle, void* arg)
{
    uv_close(handle, nullptr);
}

static void handle_signal(uv_signal_t *handle, int signum)
{
    int result = uv_loop_close(handle->loop);
    if (result == UV_EBUSY)
    {
        uv_walk(handle->loop, on_uv_walk, NULL);
    }
}

static void poll_callback(uv_poll_t* handle, int status, int events)
{
    auto confusefs = (confusefs::confusefs *)handle->data;

    int ret = confusefs->handle_event();
    if (0 > ret)
    {
        cout << "Stopping poll loop, error: " << ret << endl;
        uv_poll_stop(handle);
    }
}


static int poll_loop(int session_fd, confusefs::confusefs *confusefs)
{
    int res = 0;

    /* Setup signal handlers */
    uv_signal_t sigint, sigterm;
    uv_signal_init(uv_default_loop(), &sigint);
    uv_signal_start(&sigint, handle_signal, SIGINT);

    uv_signal_init(uv_default_loop(), &sigterm);
    uv_signal_start(&sigterm, handle_signal, SIGTERM);

    /* Signal handlers shouldn't keep the loop up in case the poll handle is closed */
    uv_unref((uv_handle_t*)&sigterm);
    uv_unref((uv_handle_t*)&sigint);

    /* Setup confusefs polling */
    uv_poll_t poll_handle;
    uv_poll_init(uv_default_loop(), &poll_handle, session_fd);

    poll_handle.data = (void*)confusefs;

    res = uv_poll_start(&poll_handle, UV_READABLE, (uv_poll_cb) poll_callback);

    res = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    if (res)
    {
        cerr << "Failed to run uv loop: " << uv_err_name(res) 
                                          << uv_strerror(res) << endl;
    }

    res = uv_loop_close(uv_default_loop());
    if (res == UV_EBUSY)
    {
        uv_walk(uv_default_loop(), on_uv_walk, NULL);
    }

    return res;
}

int main(int argc, const char *argv[])
{
    if ( argc != 2 )
    {
        cout<< "Usage: "<< argv[0] << " <mountpoint>\n";
        return EINVAL;
    }

    cout << "Creating JConf configuration..." << endl;
    auto config = new jconf::Config("./config.json", "./schema.json");

    cout << "Initializing confusefs..." << endl;
    auto conffs = new confusefs::confusefs(config); 

    cout << "Starting async loop..." << endl;
    int session_fd = conffs->start_async(argv[1]);
    poll_loop(session_fd, conffs);

    conffs->stop();

    cout << "Starting blocking loop..." << endl;
    std::thread t1(&confusefs::confusefs::start, conffs, argv[1]);
    t1.join();

    delete conffs;
    delete config;

    return 0;
}
