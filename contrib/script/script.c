#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>
#include "dmtcp.h"

#define MAX_CKPT_DIR_LENGTH 128
#define MAX_SCRIPT_PATH_LENGTH 128

#define HOOK_PATH "/home/work/software/dmtcp/contrib/script"

static char *baseCkptDir;
static char newCkptDir[MAX_CKPT_DIR_LENGTH];
static char scriptCmd[MAX_SCRIPT_PATH_LENGTH];

void generate_new_directory(void)
{
  if (dmtcp_get_generation() == 1) {
    baseCkptDir = (char *)dmtcp_get_ckpt_dir();
  }
  snprintf(newCkptDir, MAX_CKPT_DIR_LENGTH, "%s/%s_%05"PRIu32"", baseCkptDir,
      dmtcp_get_uniquepid_str(), dmtcp_get_generation());

}

void dmtcp_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  switch (event) {
  case DMTCP_EVENT_WRITE_CKPT:
    generate_new_directory();

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



