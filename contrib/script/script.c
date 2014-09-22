#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>
#include "dmtcp.h"

#define MAX_CKPT_DIR_LENGTH 512
#define MAX_SCRIPT_PATH_LENGTH 512
#define MAX_CREATEDIR_CMD 512

#define HOOK_PATH "/home/work/software/dmtcp/contrib/script"

static char *baseCkptDir;
static char createCkptDirCmd[MAX_CREATEDIR_CMD];
static char newCkptDir[MAX_CKPT_DIR_LENGTH];
static char scriptCmd[MAX_SCRIPT_PATH_LENGTH];

void generate_new_directory(void)
{
  if (dmtcp_get_generation() == 1) {
    baseCkptDir = (char *)dmtcp_get_ckpt_dir();
  }
  snprintf(newCkptDir, MAX_CKPT_DIR_LENGTH, "%s/%05"PRIu32"/%s", baseCkptDir,
      dmtcp_get_generation(), dmtcp_get_uniquepid_str());
}

void dmtcp_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  switch (event) {
  case DMTCP_EVENT_WRITE_CKPT:
    generate_new_directory();
    snprintf(createCkptDirCmd, MAX_CREATEDIR_CMD, "mkdir -p %s", newCkptDir);	
    system(createCkptDirCmd);
    dmtcp_set_ckpt_dir(newCkptDir);
    dmtcp_set_coord_ckpt_dir(newCkptDir);
    break;
  case DMTCP_EVENT_RESUME:
    snprintf(scriptCmd, MAX_SCRIPT_PATH_LENGTH, "%s%s %s %s", HOOK_PATH, "/dmtcp_event_resume", newCkptDir, "&");
    system(scriptCmd);
    break;
  default:
    ;
  }

  /* Call this next line in order to pass DMTCP events to later plugins. */
  DMTCP_NEXT_EVENT_HOOK(event, data);
}



