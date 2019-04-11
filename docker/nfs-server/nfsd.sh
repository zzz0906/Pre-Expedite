#!/bin/bash
set -e

export_base="/nfs/share/"

### Handle `docker  stop` for graceful shutdown
function shutdown {
    echo "- Shutting down nfs-server.."
    service nfs-kernel-server stop
    echo "- Nfs server is down"
    exit 0
}

trap "shutdown" SIGTERM
####

echo -e "\n- Create block device and mount it to /nfs/share .."
/bin/nfsmount $1 

echo "Export points:"
echo "$export_base *(rw,sync,insecure,no_subtree_check,no_root_squash,crossmnt,no_acl,no_all_squash)" | tee /etc/exports
#echo "$export_base *(rw,sync,insecure,no_subtree_check,no_root_squash,crossmnt,no_acl)" | tee /etc/exports


#read -a exports <<< "${@}"
#for export in "${exports[@]}"; do
#    src=`echo "$export" | sed 's/^\///'` # trim the first '/' if given in export path
#    src="$export_base$src"
#    mkdir -p $src
#    chmod 777 $src
#    echo "$src *(rw,sync,insecure,no_subtree_check,no_root_squash,crossmnt,no_acl,no_all_squash)" | tee -a /etc/exports
#    echo "$src *(rw,sync,insecure,no_subtree_check,no_root_squash,crossmnt,no_acl)" | tee -a /etc/exports
#done

echo -e "\n- Initializing nfs server.."
rpcbind
service nfs-kernel-server start

echo "- Nfs server is up and running.."

## Run forever
echo "--starting nfs-server---"
/bin/nfsserver

## Run forever 
sleep infinity
