#LoggedFS - Filesystem monitoring with Fuse
##Description

LoggedFS is a fuse-based filesystem which can log every operations that happens in it. Sourceforge page
How does it work ?

Fuse does almost everything. LoggedFS only sends a message to syslog when called by fuse and then let the real filesystem do the rest of the job.

##Installation

If loggedfs is included in your distribution you can just install with your package manager.

##Installation from source


First you have to make sure that fuse is installed on your computer. 
If you have a recent distribution it should be. Fuse can be downloaded here : http://fuse.sourceforge.net.
Then you should download the loggedfs archive and install it with the make command :

    tar xfj loggedfs-X.Y.tar.bz2
    cd loggedfs-X.Y
    make
    make install
    
LoggedFS has the following dependencies :

    fuse
    rlog
    pcre
    libxml2

##Configuration

LoggedFS can use an XML configuration file if you want it to log operations only for certain files, for certain users, or for certain operations.

Here is a sample configuration file :

    <?xml version="1.0" encoding="UTF-8"?>
    
    <loggedFS logEnabled="true" printProcessName="true">
      <includes>
        <include extension=".*" uid="*" action=".*" retname=".*"/>
      </includes>
      <excludes>
        <exclude extension=".*\.bak$" uid="*" action=".*" retname="SUCCESS"/>
        <exclude extension=".*" uid="1000" action=".*" retname="FAILURE"/>
        <exclude extension=".*" uid="*" action="getattr" retname=".*"/>
      </excludes>
    </loggedFS>

This configuration can be used to log everything except it if concerns a *.bak file, or if the uid is 1000, or if the operation is getattr.

##Launching LoggedFS

If you just want to test LoggedFS you don't need any configuration file.
 
Just use that command :

    loggedfs -f -p /var

You should see logs like these :

    tail -f /var/log/syslog
    17:29:34 (src/loggedfs.cpp:552) LoggedFS running as a public filesystem
    17:29:34 (src/loggedfs.cpp:547) LoggedFS not running as a daemon
    17:29:34 (src/loggedfs.cpp:666) LoggedFS starting at /var.
    17:29:34 (src/loggedfs.cpp:691) chdir to /var
    17:29:35 (src/loggedfs.cpp:136) getattr /var/ {SUCCESS} [ pid = 8700 kded [kdeinit] uid = 1000 ]
    17:29:41 (src/loggedfs.cpp:136) getattr /var/ {SUCCESS} [ pid = 10923 ls uid = 1000 ]
    17:29:41 (src/loggedfs.cpp:136) getattr /var/run {SUCCESS} [ pid = 10923 ls uid = 1000 ]
    17:29:41 (src/loggedfs.cpp:136) getattr /var/run/nscd {FAILURE} [ pid = 10923 ls uid = 1000 ]
    17:29:41 (src/loggedfs.cpp:136) readdir /var/ {SUCCESS} [ pid = 10923 ls uid = 1000 ]
    17:29:41 (src/loggedfs.cpp:136) getattr /var/pouak {SUCCESS} [ pid = 10923 ls uid = 1000 ]

If you have a configuration file to use you should use this command :

    ./loggedfs -c loggedfs.xml -p /var

If you want to log what other users do on your filesystem, you should use the -p option to allow them to see your mounted files. For a complete documentation see the manual page

Rémi Flament - remipouak at gmail.com