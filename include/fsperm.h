/*
change user filesysyem perimission
refer to https://stackoverflow.com/questions/4568681/using-chmod-in-a-c-program
*/

//perm_mode demo 0755 0644 
int VFSPermission(char *fspath, char *perm_mode);
int VFSDirPermission(char *vfs_path, int user_id, int group_id, char *perm_mode) ;

