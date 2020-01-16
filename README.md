# PRE-Expedite
Today’s  distributed  and  parallel  file  systems,  e.g.Lustre  and  HDFS,  have  been  deployed  to  provide  aggregateI/O  bandwidth  for  data-intensive  applications.  They  typicallyconfigure centralized, shared metadata servers to server metadatarequests.  However,  when  dealing  with  emerging  ML/DL  appli-cations,  the  metadata  servers  are  potentially  overwhelmed  bythe large amount of small training files, leading to performancedegradation.   We   investigate   an   approach   of   reducing   mass-file  access  to  metadata  server,  which  uses  negligible  client-sideresource to manage and aggregate the mass files, so as to alleviatethe bottleneck of metadata server and achieve high performance.

To this end, we design and implement Pre-Expedite, a middle-ware  framework  that  creates  virtual  file  blocks  for  managingmass-file  requests  at  the  client  of  distributed  or  parallel  filesystems. The block is formatted as a lightweight file system thatcan aggregate mass files together, store them into remote storageservers  and  serve  metadata  requests  locally  without  involvingwith metadata servers. To avoid access conflict on the virtual fileblocks,  we  also  design  a  permission  control  and  access  coordi-nation  mechanism  at  client  side.  We  evaluate  the  performanceof Pre-Expedite through benchmarks and real-world workloads.Our  experiments  show  that  the  framework  can  improve  theoverall performance through reducing the frequency of metadataserver  accesses.

## Install
