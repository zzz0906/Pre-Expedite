# Pre-Expedite
Today’s distributed and parallel file systems, e.g.  Lustre and HDFS, have been deployed to provide aggregate I/O bandwidth for data-intensive applications. They typically configure centralized, shared metadata servers to server meta-data requests. However, when dealing with emerging ML/DL applications (mainly N-1 access and N-N write), the metadata servers are potentially overwhelmedby the large amount of small training files, leading to performance degradation.  

Pre-Expedite investigate an approach of using HSS (Hierarchical Structure Space) to reduce massive number of files access to metadata server. HSS utlize negligible client-side resource to manage and aggregate the massive number of files, so as to alleviate the bottleneck of metadata server and achieve high performance.  Pre-expedite creates a HSS with an indicated size for managing massive number of files requests at the client of distributed or parallel file systems.  The HSS can be seen as a lightweight file system that can aggregate massive number of files together, store them into remote storage servers and serve metadata requests locally without involving with metadata servers.  To avoid write conflict on the HSS, we also design a permission control and access coordination mechanism at client side.  We evaluate the performance of Pre-Expedite through benchmarks and real-world workloads. Pre-Expedite provides up to 17x speedup for metadata operationsand 1.25-3.5x performance improvement for various data-intensive applications

# Install

## Dependency
- Please ensure your machine is 64-bit machine
- Please install libconfuse [https://github.com/libconfuse/libconfuse] first
- Please ensure you have install gcc/make/xfsprogs before using Pre-Expedite. If you use a Ubuntu machine you can use following command:

```
apt-get install gcc ubuntu-make xfsprogs
```


## Make

```
git clone https://github.com/zzz0906/Pre-Expedite

make

cd bin/

sudo ./Pre-Expedite ZHOU 1 1000 1000
```

## Usage

First after you make Pre-Expedite. You can move Pre-Expedite to the /usr/bin dirctory to let you use Pre-Expedite everywhere.

If you want to use Pre-Expedite in DFS. Please ensure you have root permission. If you do not have root permission, please ask administrator.

```
Pre-Expedite [Hierarchical Structure Space You want to Name] [Size GB] [UID for HSS] [GID for HSS]
```

### Example
```
Pre-Expedite ZHOU 10 1000 1000
```
Through this command, you will creat ZHOU in the HSS directory and you can put your data in HSS-SHARE-DIR (<10G in this example) for your AI/BIG DATA process to read. You will find that the 'read' will be **Expedited UP**!

If you want to change the HSS-SHARE-DIR name or HSS name please modify the 

```
static char *NFS_SERVER_SHARE_DIR = "HSS-SHARE-DIR/";
static char *NFS_BLOCK_DEFAULT_DIR = "HSS/";
```

in PRE-Expedite.c


## Notes
Pre-Expedite still have **not** open-source the NFS-Server-Client application. It only support stand-alone version currently.