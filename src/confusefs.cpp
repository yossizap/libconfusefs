#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <fuse_lowlevel.h>
#include <syslog.h>
#include <cstdlib>
#include <map>

#include "jconf/jconf.hpp"
#include "nlohmann/json.hpp"

#include "confusefs.hpp"

using json = nlohmann::json;

namespace confusefs
{
confusefs::confusefs(jconf::Config *configuration) : m_config(configuration)
{
    /* Initialize fuse ops struct */
    m_oper.getattr = confusefs::getattr;
    m_oper.open = confusefs::open;
    m_oper.read = confusefs::read;
    m_oper.lookup = confusefs::lookup;
    m_oper.readdir = confusefs::readdir;
    m_oper.write = confusefs::write;
    m_oper.init = confusefs::init;

    /* Make sure the json schema is loaded, reloading won't cause an issue */
    m_config->load();
}

confusefs::~confusefs()
{
    stop();
}

int confusefs::init_session(const std::string &mountpoint)
{
    struct fuse_cmdline_opts opts;
    int ret = -1;

    const char *fuse_args[] = {"confusefs", mountpoint.c_str()};
    struct fuse_args args = FUSE_ARGS_INIT(2, (char **)fuse_args);

    if (fuse_parse_cmdline(&args, &opts) != 0)
    {
        return ret;
    }

    /* Pass 'this' as userdata to access the class' utility functions and
     * members from the static functions */
    m_session = fuse_session_new(&args, &m_oper, sizeof(m_oper), (void *)this);
    if (m_session == NULL)
    {
        ret = -1;
        goto out;
    }

    ret = fuse_set_signal_handlers(m_session);
    if (ret)
    {
        goto out;
    }

    ret = fuse_session_mount(m_session, opts.mountpoint);
    if (ret)
    {
        goto out;
    }

out:
    if (NULL != opts.mountpoint)
        free(opts.mountpoint);

    return ret;
}

int confusefs::start(const std::string &mountpoint)
{
    init_session(mountpoint);

    int ret = fuse_session_loop(m_session);

    return ret;
}

int confusefs::start_async(const std::string &mountpoint)
{
    init_session(mountpoint);

    m_session_fd = fuse_session_fd(m_session);

    return m_session_fd;
}

int confusefs::stop(void)
{
    if (NULL != m_session)
    {
        fuse_session_exit(m_session);
        fuse_session_unmount(m_session);
        fuse_remove_signal_handlers(m_session);
        fuse_session_destroy(m_session);
    }
    return 0;
}

int confusefs::handle_event(void)
{
    int ret = 0;
    struct fuse_buf fbuf;

    bzero(&fbuf, sizeof(fbuf));

    ret = fuse_session_exited(m_session);
    if (ret)
    {
        goto out;
    }

    ret = fuse_session_receive_buf(m_session, &fbuf);
    /* Empty request */
    if (ret == 0)
    {
        goto out;
    }
    else if (ret < 0)
    {
        syslog(LOG_ERR, "fuse_session_receive_buf failed: %s\n",
               strerror(errno));
        goto out;
    }

    fuse_session_process_buf(m_session, &fbuf);

out:
    if (NULL != fbuf.mem)
    {
        free(fbuf.mem);
    }

    if (!ret)
    {
        stop();
    }

    return ret;
}

void confusefs::init_inodes(const json &j,
                            const std::string path_prefix,
                            fuse_ino_t &inode)
{
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        if (it->is_structured())
        {
            m_inodes[inode] = path_prefix + it.key();
            ++inode;
            init_inodes(*it, path_prefix + it.key() + "/", inode);
        }
        else
        {
            m_inodes[inode] = path_prefix + it.key();
            ++inode;
        }
    }
}

void confusefs::init(void *userdata, struct fuse_conn_info *conn)
{
    (void)conn;
    auto klass = (confusefs *)userdata;

    json j = klass->m_config->get("/");
    fuse_ino_t i = 1;
    // Parent dir
    klass->m_inodes[i++] = "/";
    klass->init_inodes(j, "/", i);
}

int confusefs::stat(fuse_ino_t ino, struct stat *stbuf)
{
    stbuf->st_ino = ino;
    if (1 == ino)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }

    auto node = m_config->get(m_inodes[ino]);

    if (node.is_structured())
    {
        stbuf->st_mode = S_IFDIR | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = m_inodes[ino].length();
    }
    else
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = m_inodes[ino].length();
    }

    return 0;
}

void confusefs::getattr(fuse_req_t req,
                        fuse_ino_t ino,
                        struct fuse_file_info *fi)
{
    struct stat stbuf;
    auto klass = (confusefs *)fuse_req_userdata(req);

    syslog(LOG_DEBUG, "getattr\n");

    (void)fi;

    memset(&stbuf, 0, sizeof(stbuf));
    if (klass->stat(ino, &stbuf) == -1)
        fuse_reply_err(req, ENOENT);
    else
        fuse_reply_attr(req, &stbuf, 1.0);
}

void confusefs::lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    auto klass = (confusefs *)fuse_req_userdata(req);
    std::string path;

    syslog(LOG_DEBUG, "lookup\n");

    memset(&e, 0, sizeof(e));

    /* Construct a full path to search the inodes structure */
    if (parent != 1)
    {
        path += klass->m_inodes[parent];
    }

    path += "/" + std::string(name);

    /* Attempt to find a matching file path in the inodes structure  to get the
     * inode */
    bool match = false;
    for (auto it = klass->m_inodes.begin(); it != klass->m_inodes.end(); ++it)
    {
        if (0 == it->second.compare(path))
        {
            e.ino = it->first;
            match = true;
            break;
        }
    }
    if (!match)
    {
        fuse_reply_err(req, ENOENT);
        return;
    }

    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    klass->stat(e.ino, &e.attr);

    fuse_reply_entry(req, &e);
}

void confusefs::dirbuf_add(fuse_req_t req,
                           std::vector<char> &dirbuf,
                           const char *name,
                           fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = dirbuf.size();

    /* FUSE aligns the size of buffers and adds metadata to each name
     * so we have to check the size of the entry */
    size_t direntry_size = fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    dirbuf.resize(dirbuf.size() + direntry_size);

    stbuf.st_ino = ino;

    fuse_add_direntry(req, dirbuf.data() + oldsize, direntry_size, name, &stbuf,
                      dirbuf.size());
}

int confusefs::reply_buf_limited(fuse_req_t req,
                                 const char *buf,
                                 size_t bufsize,
                                 off_t off,
                                 size_t maxsize)
{
    if (off < bufsize)
        return fuse_reply_buf(req, buf + off, std::min(bufsize - off, maxsize));
    else
        return fuse_reply_buf(req, NULL, 0);
}

void confusefs::readdir(fuse_req_t req,
                        fuse_ino_t ino,
                        size_t size,
                        off_t off,
                        struct fuse_file_info *fi)
{
    (void)fi;
    auto klass = (confusefs *)fuse_req_userdata(req);

    syslog(LOG_DEBUG, "readdir\n");

    json j = klass->m_config->get(klass->m_inodes[ino]);
    if (!j.is_structured())
    {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    std::vector<char> dirbuf;
    klass->dirbuf_add(req, dirbuf, "..", 1);
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        klass->dirbuf_add(req, dirbuf, it.key().c_str(), ino++);
    }

    klass->reply_buf_limited(req, dirbuf.data(), dirbuf.size(), off, size);
}

void confusefs::open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    (void)ino;
    auto klass = (confusefs *)fuse_req_userdata(req);

    syslog(LOG_DEBUG, "open\n");

    /* Prevent open on directories */
    json j = klass->m_config->get(klass->m_inodes[ino]);
    if (j.is_structured())
    {
        fuse_reply_err(req, EISDIR);
    }

    fuse_reply_open(req, fi);
}

void confusefs::read(fuse_req_t req,
                     fuse_ino_t ino,
                     size_t size,
                     off_t off,
                     struct fuse_file_info *fi)
{
    (void)fi;
    auto klass = (confusefs *)fuse_req_userdata(req);

    syslog(LOG_DEBUG, "read\n");

    std::string content = klass->m_config->get(klass->m_inodes[ino]).dump();

    klass->reply_buf_limited(req, content.c_str(), content.length(), off, size);
}

void confusefs::write(fuse_req_t req,
                      fuse_ino_t ino,
                      const char *buf,
                      size_t size,
                      off_t off,
                      struct fuse_file_info *fi)
{
    (void)fi, (void)ino, (void)off;
    auto klass = (confusefs *)fuse_req_userdata(req);
    json value;

    syslog(LOG_DEBUG, "write\n");

    try
    {
        value = json::parse(buf, buf + size);
    }
    catch (const std::exception &e)
    {
        fuse_reply_err(req, EBADMSG);
        syslog(LOG_ERR, "Write - parse failed\n");
        return;
    }

    try
    {
        klass->m_config->set(klass->m_inodes[ino], value);
    }
    catch (const std::exception &e)
    {
        fuse_reply_err(req, ENOENT);
        syslog(LOG_ERR, "Write - path not found\n");
        return;
    }

    fuse_reply_write(req, size);
}
}  // namespace confusefs
