#ifndef CONFIG_H
#define CONFIG_H

extern int DO_PHT_SNAPSHOTS;
extern int SNAPSHOT_INTERVAL;
extern char *SNAPSHOT_FILE_BASE_NAME;
extern int MAX_SNAPSHOT_FILES;
extern double GEOMETRIC_BASE; //scale up snapshots with geometric series; use base less than 1 to disable

#endif