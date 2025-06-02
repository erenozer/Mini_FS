#ifndef FS_H

void mkfs(const char *diskfile);
int mkdir_fs(const char *path);
int create_fs(const char *path);
int write_fs(const char *path, const char *data);
int read_fs(const char *path, char *buf, int bufsize); int delete_fs(const char *path);
int rmdir_fs(const char *path); int ls_fs(const char *path,
DirectoryEntry *entries , int max_entries);

#endif // FS_H