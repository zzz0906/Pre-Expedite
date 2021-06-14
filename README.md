# Pre-Expedite
Today's parallel and distributed file systems (PFS/DFS), e.g. Lustre and HDFS, have been deployed to provide aggregate IO bandwidth for data-intensive applications. They typically configure centralized, shared metadata servers (MDS) to serve metadata requests. However, when dealing with emerging Deep Learning (DL) applications (mainly N-1 and N-M reads), the metadata servers are potentially overwhelmed by a large amount of small training files in the dataset, leading to performance degradation.

To overcome this issue, we design and implement , which uses Hierarchical Structure Space (HSS) to reduce a massive number of file access MDS. HSS is a user-space storage that utilizes negligible client-side resources to manage and aggregate the massive number of small files, so as to hide all of these small files from MDS and achieve high performance. Pre-Expedite adopts POSIX to create HSS with a fixed size for managing a massive number of file requests at the compute node. HSS can be viewed as a lightweight file system that can aggregate a massive number of files together, store them into remote data servers and serve metadata requests locally without involving with MDS. To avoid access conflict on HSS, we also design access coordination mechanisms at client side. We evaluate the performance of Pre-Expedite through benchmarks and real-world workloads. Our experiments show that Pre-Expedite provides up to 17x speedup for throughput of reading small files and 1.25 - 3.5x performance improvement for various data-intensive applications.

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
Pre-Expedite still have **not** made the Pre-Expedite open-source application. It only support stand-alone version currently.