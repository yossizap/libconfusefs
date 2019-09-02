#ifndef CONFUSEFS_H
#define CONFUSEFS_H

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <fuse_lowlevel.h>

#include "jconf/jconf.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace confusefs { 

class confusefs
{
   public:
    confusefs(jconf::Config* config);
    ~confusefs();

    /**
     * Start/mount the FUSE filesystem and return a file descriptor to integrate
     * confusefs with a custom event loop. The event loop should call 'handle_event'
     * to process the fd's events.
     *
     * @param mountpoint filesystem mount path 
     * @return a file descriptor
     */
    int start_async(std::string mountpoint);

    /**
     * Stops/umounts the filesystem.
     *
     * @return success/failure
     */
    int stop(void);

    /**
     * Process FUSE events asynchronously.
     *
     * @return success/failure
     */
    int handle_event(void);

   private:
    /** JSON Schema configuration used to create the filesystem */
    jconf::Config *m_config;

    /** FUSE low level operations struct */
    struct fuse_lowlevel_ops m_oper;

    /** Current FUSE session */
    struct fuse_session *m_session;

    /** Map of unique inodes for each file path - generated from m_config */
    std::map<fuse_ino_t, std::string> m_inodes;

    /** Current session file descriptor */
    int m_session_fd;

    /* FUSE utility functions */
    int reply_buf_limited(fuse_req_t req,
            const char* buf,
            size_t bufsize,
            off_t off,
            size_t maxsize);
    void dirbuf_add(fuse_req_t req,
            std::vector<char> &dirbuf,
            const char* name,
            fuse_ino_t ino);
    void init_inodes(const json& j,
            const std::string path_prefix,
            fuse_ino_t& inode);
    int stat(fuse_ino_t ino, struct stat* stbuf);

    /* FUSE operations */
    static void init(void* userdata, struct fuse_conn_info* conn);
    static void getattr(fuse_req_t req,
                        fuse_ino_t ino,
                        struct fuse_file_info* fi);
    static void lookup(fuse_req_t req, fuse_ino_t parent, const char* name);
    static void readdir(fuse_req_t req,
                        fuse_ino_t ino,
                        size_t size,
                        off_t off,
                        struct fuse_file_info* fi);
    static void open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
    static void read(fuse_req_t req,
                     fuse_ino_t ino,
                     size_t size,
                     off_t off,
                     struct fuse_file_info* fi);
    static void write(fuse_req_t req,
                      fuse_ino_t ino,
                      const char* buf,
                      size_t size,
                      off_t off,
                      struct fuse_file_info* fi);
};

} // namespace confusefs

#endif  // CONFUSEFS_H
