# Pre-Expedite
Today’s parallel and distributed file systems (PFS/DFS), e.g., Lustre and HDFS, have been deployed to provideaggregate IO bandwidth for data-intensive applications. They typically configure centralized, shared metadata servers (MDS)to serve metadata requests.  However, when dealing with emerging Deep Learning (DL) applications, the metadata serversare potentially overwhelmed by a large amount of small training files in the dataset, leading to performance degradation.To overcome this issue, we design and implement Pre-Expedite, which uses Hierarchical Structure Space (HSS) to reduce amassive number of file access to MDS. HSS is user-space storage that utilizes negligible client-side resources to manage andaggregate the massive number of small files, so as to hide all of these small files from MDS and achieve high performance.Pre-Expedite adopts POSIX to create a fixed size HSS for managing a massive number of file requests at the compute node.HSS can be viewed as a lightweight file system that can aggregate a massive number of files together, store them into remotedata servers and serve metadata requests locally without involving MDS. To avoid access conflict on HSS, we also designaccess coordination mechanisms on the client side.  We evaluate the performance of Pre-Expedite through benchmarks andreal-world workloads.  Our experiments show that Pre-Expedite provides up to 17x speedup for throughput of reading smallfiles and 1.25-3.5x performance improvement for various data-intensive applications.

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
