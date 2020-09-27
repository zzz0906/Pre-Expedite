/**
 * 
 * 
 *  get  request filesystem size 
 * open file and  create fileblock
 *  use  large file  size --> mb
 * if size means GB then create size*1024 if size means TB then create size**1024*1024GB
 */
int VFSBlockCreate(char *fb_name, int size);