/*****************************************************************************
 * Author:   Remi Flament <remipouak at gmail dot com>
 *****************************************************************************
 * Copyright (c) 2005 - 2018, Remi Flament
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#ifdef linux
/* For pread()/pwrite() */
#define _X_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include "easylogging++.h"
#include <stdarg.h>
#include <getopt.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include "Config.h"
#include <stdexcept>
#include <iostream>

INITIALIZE_EASYLOGGINGPP

#define STR(X) #X
#define rAssert(cond)                                     \
    do                                                    \
    {                                                     \
        if ((cond) == false)                              \
        {                                                 \
            LOG(ERROR) << "Assert failed: " << STR(cond); \
            throw std::runtime_error(STR(cond));          \
        }                                                 \
    } while (false)

#define PUSHARG(ARG)                      \
    rAssert(out->fuseArgc < MaxFuseArgs); \
    out->fuseArgv[out->fuseArgc++] = ARG

using namespace std;
static Config config;
static int savefd;
static el::base::DispatchAction dispatchAction = el::base::DispatchAction::NormalLog;
static const char *loggerId = "default";
static const char *additionalInfoFormat = " {%s} [ pid = %d %s uid = %d ]";
static el::Logger *defaultLogger;

const int MaxFuseArgs = 32;
struct LoggedFS_Args
{
    char *mountPoint; // where the users read files
    char *configFilename;
    bool isDaemon; // true == spawn in background, log to syslog
    const char *fuseArgv[MaxFuseArgs];
    int fuseArgc;
};

static LoggedFS_Args *loggedfsArgs = new LoggedFS_Args;

static bool isAbsolutePath(const char *fileName)
{
    if (fileName && fileName[0] != '\0' && fileName[0] == '/')
        return true;
    else
        return false;
}

static char *getAbsolutePath(const char *path)
{
    char *realPath = new char[strlen(path) + strlen(loggedfsArgs->mountPoint) + 1];

    strcpy(realPath, loggedfsArgs->mountPoint);
    if (realPath[strlen(realPath) - 1] == '/')
        realPath[strlen(realPath) - 1] = '\0';
    strcat(realPath, path);
    return realPath;
}

static char *getRelativePath(const char *path)
{
    if (path[0] == '/')
    {
        if (strlen(path) == 1)
        {
            return strdup(".");
        }
        const char *substr = &path[1];
        return strdup(substr);
    }

    return strdup(path);
}

/*
 * Returns the name of the process which accessed the file system.
 */
static char *getcallername()
{
    char filename[100];
    sprintf(filename, "/proc/%d/cmdline", fuse_get_context()->pid);
    FILE *proc;
    char cmdline[256] = "";

    if ((proc = fopen(filename, "rt")) == NULL)
        return NULL;
    else
    {
        fread(cmdline, sizeof(cmdline), 1, proc);
        fclose(proc);
    }

    return strdup(cmdline);
}

static void loggedfs_log(const char *path, const char *action, const int returncode, const char *format, ...)
{
    const char *retname;

    if (returncode >= 0)
        retname = "SUCCESS";
    else
        retname = "FAILURE";

    if (config.shouldLog(path, fuse_get_context()->uid, action, retname))
    {
        va_list args;
        char *buf = NULL;
        char *additionalInfo = NULL;

        char *caller_name = getcallername();
        asprintf(&additionalInfo, additionalInfoFormat, retname, fuse_get_context()->pid, config.isPrintProcessNameEnabled() ? caller_name : "", fuse_get_context()->uid);

        va_start(args, format);
        vasprintf(&buf, format, args);
        va_end(args);

        if (returncode >= 0)
        {
            ELPP_WRITE_LOG(el::base::Writer, el::Level::Info, dispatchAction, "default") << buf << additionalInfo;
        }
        else
        {
            ELPP_WRITE_LOG(el::base::Writer, el::Level::Error, dispatchAction, "default") << buf << additionalInfo;
        }

        free(buf);
        free(additionalInfo);
        free(caller_name);
    }
}

static void *loggedFS_init(struct fuse_conn_info *info)
{
    fchdir(savefd);
    close(savefd);
    return NULL;
}

static int loggedFS_getattr(const char *orig_path, struct stat *stbuf)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = lstat(path, stbuf);
    loggedfs_log(aPath, "getattr", res, "getattr %s", aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_access(const char *orig_path, int mask)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = access(path, mask);
    loggedfs_log(aPath, "access", res, "access %s", aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_readlink(const char *orig_path, char *buf, size_t size)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = readlink(path, buf, size - 1);
    loggedfs_log(aPath, "readlink", res, "readlink %s", aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    buf[res] = '\0';

    return 0;
}

static int loggedFS_readdir(const char *orig_path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int res;

    (void)offset;
    (void)fi;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    dp = opendir(path);
    if (dp == NULL)
    {
        res = -errno;
        loggedfs_log(aPath, "readdir", -1, "readdir %s", aPath);
        delete[] aPath;
        free(path);
        return res;
    }

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    loggedfs_log(aPath, "readdir", 0, "readdir %s", aPath);
    delete[] aPath;
    free(path);

    return 0;
}

static int loggedFS_mknod(const char *orig_path, mode_t mode, dev_t rdev)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    if (S_ISREG(mode))
    {
        res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
        loggedfs_log(aPath, "mknod", res, "mknod %s %o S_IFREG (normal file creation)", aPath, mode);
        if (res >= 0)
            res = close(res);
    }
    else if (S_ISFIFO(mode))
    {
        res = mkfifo(path, mode);
        loggedfs_log(aPath, "mkfifo", res, "mkfifo %s %o S_IFFIFO (fifo creation)", aPath, mode);
    }
    else
    {
        res = mknod(path, mode, rdev);
        if (S_ISCHR(mode))
        {
            loggedfs_log(aPath, "mknod", res, "mknod %s %o (character device creation)", aPath, mode);
        }
        /*else if (S_IFBLK(mode))
		{
		loggedfs_log(aPath,"mknod",res,"mknod %s %o (block device creation)",aPath, mode);
		}*/
        else
            loggedfs_log(aPath, "mknod", res, "mknod %s %o", aPath, mode);
    }

    delete[] aPath;

    if (res == -1)
    {
        free(path);
        return -errno;
    }
    else
        lchown(path, fuse_get_context()->uid, fuse_get_context()->gid);

    free(path);

    return 0;
}

static int loggedFS_mkdir(const char *orig_path, mode_t mode)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = mkdir(path, mode);
    loggedfs_log(path, "mkdir", res, "mkdir %s %o", aPath, mode);
    delete[] aPath;

    if (res == -1)
    {
        free(path);
        return -errno;
    }
    else
        lchown(path, fuse_get_context()->uid, fuse_get_context()->gid);

    free(path);
    return 0;
}

static int loggedFS_unlink(const char *orig_path)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = unlink(path);
    loggedfs_log(aPath, "unlink", res, "unlink %s", aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_rmdir(const char *orig_path)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = rmdir(path);
    loggedfs_log(aPath, "rmdir", res, "rmdir %s", aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;
    return 0;
}

static int loggedFS_symlink(const char *from, const char *orig_to)
{
    int res;

    char *aTo = getAbsolutePath(orig_to);
    char *to = getRelativePath(orig_to);

    res = symlink(from, to);

    loggedfs_log(aTo, "symlink", res, "symlink from %s to %s", aTo, from);
    delete[] aTo;

    if (res == -1)
    {
        free(to);
        return -errno;
    }
    else
        lchown(to, fuse_get_context()->uid, fuse_get_context()->gid);

    free(to);
    return 0;
}

static int loggedFS_rename(const char *orig_from, const char *orig_to)
{
    int res;
    char *aFrom = getAbsolutePath(orig_from);
    char *aTo = getAbsolutePath(orig_to);
    char *from = getRelativePath(orig_from);
    char *to = getRelativePath(orig_to);
    res = rename(from, to);
    loggedfs_log(aFrom, "rename", res, "rename %s to %s", aFrom, aTo);
    delete[] aFrom;
    delete[] aTo;
    free(from);
    free(to);

    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_link(const char *orig_from, const char *orig_to)
{
    int res;

    char *aFrom = getAbsolutePath(orig_from);
    char *aTo = getAbsolutePath(orig_to);
    char *from = getRelativePath(orig_from);
    char *to = getRelativePath(orig_to);

    res = link(from, to);
    loggedfs_log(aTo, "link", res, "hard link from %s to %s", aTo, aFrom);
    delete[] aFrom;
    delete[] aTo;
    free(from);

    if (res == -1)
    {
        delete[] to;
        return -errno;
    }
    else
        lchown(to, fuse_get_context()->uid, fuse_get_context()->gid);

    free(to);

    return 0;
}

static int loggedFS_chmod(const char *orig_path, mode_t mode)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = chmod(path, mode);
    loggedfs_log(aPath, "chmod", res, "chmod %s to %o", aPath, mode);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static char *getusername(uid_t uid)
{
    struct passwd *p = getpwuid(uid);
    if (p != NULL)
        return p->pw_name;
    return NULL;
}

static char *getgroupname(gid_t gid)
{
    struct group *g = getgrgid(gid);
    if (g != NULL)
        return g->gr_name;
    return NULL;
}

static int loggedFS_chown(const char *orig_path, uid_t uid, gid_t gid)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = lchown(path, uid, gid);

    char *username = getusername(uid);
    char *groupname = getgroupname(gid);

    if (username != NULL && groupname != NULL)
        loggedfs_log(aPath, "chown", res, "chown %s to %d:%d %s:%s", aPath, uid, gid, username, groupname);
    else
        loggedfs_log(aPath, "chown", res, "chown %s to %d:%d", aPath, uid, gid);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_truncate(const char *orig_path, off_t size)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = truncate(path, size);
    loggedfs_log(aPath, "truncate", res, "truncate %s to %d bytes", aPath, size);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#if (FUSE_USE_VERSION == 25)
static int loggedFS_utime(const char *orig_path, struct utimbuf *buf)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = utime(path, buf);
    loggedfs_log(aPath, "utime", res, "utime %s", aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#else

static int loggedFS_utimens(const char *orig_path, const struct timespec ts[2])
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    res = utimensat(AT_FDCWD, path, ts, AT_SYMLINK_NOFOLLOW);

    loggedfs_log(aPath, "utimens", res, "utimens %s", aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#endif

static int loggedFS_open(const char *orig_path, struct fuse_file_info *fi)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = open(path, fi->flags);

    // what type of open ? read, write, or read-write ?
    if (fi->flags & O_RDONLY)
    {
        loggedfs_log(aPath, "open-readonly", res, "open readonly %s", aPath);
    }
    else if (fi->flags & O_WRONLY)
    {
        loggedfs_log(aPath, "open-writeonly", res, "open writeonly %s", aPath);
    }
    else if (fi->flags & O_RDWR)
    {
        loggedfs_log(aPath, "open-readwrite", res, "open readwrite %s", aPath);
    }
    else
        loggedfs_log(aPath, "open", res, "open %s", aPath);

    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int loggedFS_read(const char *orig_path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    int res;

    loggedfs_log(aPath, "read", 0, "read %d bytes from %s at offset %d", size, aPath, offset);
    res = pread(fi->fh, buf, size, offset);
    if (res == -1)
    {
        res = -errno;
        loggedfs_log(aPath, "read", -1, "read %d bytes from %s at offset %d", size, aPath, offset);
    }
    else
    {
        loggedfs_log(aPath, "read", 0, "%d bytes read from %s at offset %d", res, aPath, offset);
    }
    delete[] aPath;
    return res;
}

static int loggedFS_write(const char *orig_path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    (void)fi;

    fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        res = -errno;
        loggedfs_log(aPath, "write", -1, "write %d bytes to %s at offset %d", size, aPath, offset);
        delete[] aPath;
        free(path);
        return res;
    }
    else
    {
        loggedfs_log(aPath, "write", 0, "write %d bytes to %s at offset %d", size, aPath, offset);
    }

    res = pwrite(fd, buf, size, offset);

    if (res == -1)
        res = -errno;
    else
        loggedfs_log(aPath, "write", 0, "%d bytes written to %s at offset %d", res, aPath, offset);

    close(fd);
    delete[] aPath;
    free(path);

    return res;
}

static int loggedFS_statfs(const char *orig_path, struct statvfs *stbuf)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = statvfs(path, stbuf);
    loggedfs_log(aPath, "statfs", res, "statfs %s", aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int loggedFS_release(const char *orig_path, struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    (void)orig_path;

    loggedfs_log(aPath, "release", 0, "release %s", aPath);
    delete[] aPath;

    close(fi->fh);
    return 0;
}

static int loggedFS_fsync(const char *orig_path, int isdatasync,
                          struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    (void)orig_path;
    (void)isdatasync;
    (void)fi;
    loggedfs_log(aPath, "fsync", 0, "fsync %s", aPath);
    delete[] aPath;
    return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int loggedFS_setxattr(const char *orig_path, const char *name, const char *value,
                             size_t size, int flags)
{
    int res = lsetxattr(orig_path, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int loggedFS_getxattr(const char *orig_path, const char *name, char *value,
                             size_t size)
{
    int res = lgetxattr(orig_path, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int loggedFS_listxattr(const char *orig_path, char *list, size_t size)
{
    int res = llistxattr(orig_path, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int loggedFS_removexattr(const char *orig_path, const char *name)
{
    int res = lremovexattr(orig_path, name);
    if (res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static void usage(char *name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-h] | [-l log-file] [-c config-file] [-f] [-p] [-e] /directory-mountpoint\n", name);
    fprintf(stderr, "Type 'man loggedfs' for more details\n");
    return;
}

static bool processArgs(int argc, char *argv[], LoggedFS_Args *out)
{
    // set defaults
    out->isDaemon = true;

    out->fuseArgc = 0;
    out->configFilename = NULL;

    // pass executable name through
    out->fuseArgv[0] = argv[0];
    ++out->fuseArgc;

    // leave a space for mount point, as FUSE expects the mount point before
    // any flags
    out->fuseArgv[1] = NULL;
    ++out->fuseArgc;
    opterr = 0;

    int res;

    bool got_p = false;

    // We need the "nonempty" option to mount the directory in recent FUSE's
    // because it's non empty and contains the files that will be logged.
    //
    // We need "use_ino" so the files will use their original inode numbers,
    // instead of all getting 0xFFFFFFFF . For example, this is required for
    // logging the ~/.kde/share/config directory, in which hard links for lock
    // files are verified by their inode equivalency.

#define COMMON_OPTS "nonempty,use_ino,attr_timeout=0,entry_timeout=0,negative_timeout=0"

    while ((res = getopt(argc, argv, "hpfec:l:")) != -1)
    {
        switch (res)
        {
        case 'h':
            usage(argv[0]);
            return false;
        case 'f':
            out->isDaemon = false;
            // this option was added in fuse 2.x
            PUSHARG("-f");
            defaultLogger->info("LoggedFS not running as a daemon");
            break;
        case 'p':
            PUSHARG("-o");
            PUSHARG("allow_other,default_permissions," COMMON_OPTS);
            got_p = true;
            defaultLogger->info("LoggedFS running as a public filesystem");
            break;
        case 'e':
            PUSHARG("-o");
            PUSHARG("nonempty");
            defaultLogger->info("Using existing directory");
            break;
        case 'c':
            out->configFilename = optarg;
            defaultLogger->info("Configuration file : %v", optarg);
            break;
        case 'l':
        {
            defaultLogger->info("LoggedFS log file : %v", optarg);
            el::Configurations defaultConf;
            defaultConf.setToDefault();
            defaultConf.setGlobally(el::ConfigurationType::ToFile, std::string("true"));
            defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, std::string("false"));
            defaultConf.setGlobally(el::ConfigurationType::Filename, std::string(optarg));
            el::Loggers::reconfigureLogger("default", defaultConf);
            defaultLogger = el::Loggers::getLogger("default");
            break;
        }
        default:
            break;
        }
    }

    if (!got_p)
    {
        PUSHARG("-o");
        PUSHARG(COMMON_OPTS);
    }
#undef COMMON_OPTS

    if (optind + 1 <= argc)
    {
        out->mountPoint = argv[optind++];
        out->fuseArgv[1] = out->mountPoint;
    }
    else
    {
        fprintf(stderr, "Missing mountpoint\n");
        usage(argv[0]);
        return false;
    }

    // If there are still extra unparsed arguments, pass them onto FUSE..
    if (optind < argc)
    {
        rAssert(out->fuseArgc < MaxFuseArgs);

        while (optind < argc)
        {
            rAssert(out->fuseArgc < MaxFuseArgs);
            out->fuseArgv[out->fuseArgc++] = argv[optind];
            ++optind;
        }
    }

    if (!isAbsolutePath(out->mountPoint))
    {
        fprintf(stderr, "You must use absolute paths "
                        "(beginning with '/') for %s\n",
                out->mountPoint);
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(el::ConfigurationType::ToFile, std::string("false"));
    el::Loggers::reconfigureLogger("default", defaultConf);
    defaultLogger = el::Loggers::getLogger("default");

    char *input = new char[2048]; // 2ko MAX input for configuration

    umask(0);
    fuse_operations loggedFS_oper;
    // in case this code is compiled against a newer FUSE library and new
    // members have been added to fuse_operations, make sure they get set to
    // 0..
    memset(&loggedFS_oper, 0, sizeof(fuse_operations));
    loggedFS_oper.init = loggedFS_init;
    loggedFS_oper.getattr = loggedFS_getattr;
    loggedFS_oper.access = loggedFS_access;
    loggedFS_oper.readlink = loggedFS_readlink;
    loggedFS_oper.readdir = loggedFS_readdir;
    loggedFS_oper.mknod = loggedFS_mknod;
    loggedFS_oper.mkdir = loggedFS_mkdir;
    loggedFS_oper.symlink = loggedFS_symlink;
    loggedFS_oper.unlink = loggedFS_unlink;
    loggedFS_oper.rmdir = loggedFS_rmdir;
    loggedFS_oper.rename = loggedFS_rename;
    loggedFS_oper.link = loggedFS_link;
    loggedFS_oper.chmod = loggedFS_chmod;
    loggedFS_oper.chown = loggedFS_chown;
    loggedFS_oper.truncate = loggedFS_truncate;
#if (FUSE_USE_VERSION == 25)
    loggedFS_oper.utime = loggedFS_utime;
#else
    loggedFS_oper.utimens = loggedFS_utimens;
    loggedFS_oper.flag_utime_omit_ok = 1;
#endif
    loggedFS_oper.open = loggedFS_open;
    loggedFS_oper.read = loggedFS_read;
    loggedFS_oper.write = loggedFS_write;
    loggedFS_oper.statfs = loggedFS_statfs;
    loggedFS_oper.release = loggedFS_release;
    loggedFS_oper.fsync = loggedFS_fsync;
#ifdef HAVE_SETXATTR
    loggedFS_oper.setxattr = loggedFS_setxattr;
    loggedFS_oper.getxattr = loggedFS_getxattr;
    loggedFS_oper.listxattr = loggedFS_listxattr;
    loggedFS_oper.removexattr = loggedFS_removexattr;
#endif

    for (int i = 0; i < MaxFuseArgs; ++i)
        loggedfsArgs->fuseArgv[i] = NULL; // libfuse expects null args..

    if (processArgs(argc, argv, loggedfsArgs))
    {

        if (loggedfsArgs->isDaemon)
        {
            dispatchAction = el::base::DispatchAction::SysLog;
            loggerId = "syslog";
        }

        defaultLogger->info("LoggedFS starting at %v.", loggedfsArgs->mountPoint);

        if (loggedfsArgs->configFilename != NULL)
        {

            if (strcmp(loggedfsArgs->configFilename, "-") == 0)
            {
                defaultLogger->info("Using stdin configuration");
                memset(input, 0, 2048);
                char *ptr = input;

                int size = 0;
                do
                {
                    size = fread(ptr, 1, 1, stdin);
                    ptr++;
                } while (!feof(stdin) && size > 0);
                config.loadFromXmlBuffer(input);
            }
            else
            {
                defaultLogger->info("Using configuration file %v.", loggedfsArgs->configFilename);
                config.loadFromXmlFile(loggedfsArgs->configFilename);
            }
        }
        delete[] input;
        defaultLogger->info("chdir to %v", loggedfsArgs->mountPoint);
        chdir(loggedfsArgs->mountPoint);
        savefd = open(".", 0);

#if (FUSE_USE_VERSION == 25)
        fuse_main(loggedfsArgs->fuseArgc,
                  const_cast<char **>(loggedfsArgs->fuseArgv), &loggedFS_oper);
#else
        fuse_main(loggedfsArgs->fuseArgc,
                  const_cast<char **>(loggedfsArgs->fuseArgv), &loggedFS_oper, NULL);
#endif

        defaultLogger->info("LoggedFS closing.");
    }
}
