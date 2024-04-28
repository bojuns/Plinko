#include "config.h"

int DO_PHT_SNAPSHOTS = 1;
int SNAPSHOT_INTERVAL = 10000;
char *SNAPSHOT_FILE_BASE_NAME = "./PHT_Snapshot_";
int MAX_SNAPSHOT_FILES = 10;
double GEOMETRIC_BASE = 0; //scale up snapshots with geometric series; use base less than 1 to disable