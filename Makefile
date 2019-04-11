.PHONY: all binnfsclient docker/nfs-server/nfsserver bin/nfsmount bin/nfstranfs

all: bin/nfsclient docker/nfs-server/nfsserver bin/nfsmount bin/nfstranfs

#bin/nfsclient:
	#gcc nfsclient/nfsclient.c nfsclient/vfsTools.c nfsclient/fsmk.c nfsclient/fsperm.c nfsclient/fsmount.c nfsclient/fstats.c nfsclient/fsutils.c nfsclient/fsnetutils.c nfsclient/fsclient.c nfsclient/cJSON.c -o bin/nfsclient -I include/  -lcurl  -lconfuse -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -Wall
#	gcc -g src/nfsclient/nfsclient.c src/nfsclient/fsmk.c src/nfsclient/fsperm.c src/nfsclient/fsmount.c  src/nfsclient/fsutils.c src/nfsclient/fsnetutils.c src/nfsclient/fsclient.c src/nfsclient/cJSON.c src/nfsclient/nfsnetclient.c  -o bin/nfsclient -I include/  -lcurl  -lconfuse -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -Wall
	
#docker/nfs-server/nfsserver:
#	gcc -g src/nfsserver/nfsserver.c src/nfsserver/fsmk.c src/nfsserver/fsperm.c src/nfsserver/fsmount.c  src/nfsserver/fsutils.c  src/nfsserver/cJSON.c src/nfsserver/mongoose.c  -o docker/nfs-server/nfsserver -I include/  -lcurl  -lconfuse -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE  -D MG_DISABLE_MQTT -D MG_DISABLE_COAP -Wall

 
#	gcc nfsserver/nfsserver.c -o bin/nfsserver -I include/  -lconfuse -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -Wall
bin/nfstranfs:
	gcc -g src/nfstranfs/nfsclient.c src/nfstranfs/fsmk.c src/nfstranfs/fsperm.c src/nfstranfs/fsmount.c  src/nfstranfs/fsutils.c    -o bin/nfstranfs -I include/   -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -Wall
	
clean:
	rm bin/*
