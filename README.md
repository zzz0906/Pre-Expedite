# Pre-Expedite
Today, Parallel and Distributed File Systems (PFS/DFS) such as Lustre and HDFS have gained extensive usage in data-intensive applications due to their provision of high I/O bandwidth. However, these systems rely on centralized shared metadata servers (MDS) to handle metadata requests, leading to potential performance degradation, especially when managing small files. This is commonly witnessed in the context of deep learning applications where the dataset comprises a large volume of small training files. To address this challenge, we design and implement Pre-Expedite, a solution that employs a specially designed Hierarchical Structure Space (HSS) to mitigate the file access load on the MDS. The HSS, functioning as a lightweight file system, aggregates numerous files by storing them in the memory of compute nodes, as well as remote data servers, hence responding to metadata requests locally without necessitating MDS involvement. The POSIX is used to create a fixed-size HSS for storing files on a large scale. To forestall access conflicts on HSS, we have also incorporated client-side access coordination mechanisms on the compute nodes. We evaluated the performance of Pre-Expedite through benchmarks and real-world workloads and found significant improvements. Specifically, Pre-Expedite improved throughput for reading small files by up to 17X and enhanced the performance of various data-intensive, especially including deep learning applications by 1.25-3.5X.

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
