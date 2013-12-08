#ifndef IBWRAPPERS_H
#define IBWRAPPERS_H
#include "dmtcpplugin.h"

#define _real_ibv_get_device_list NEXT_FNC(ibv_get_device_list)
#define _real_ibv_open_device NEXT_FNC(ibv_open_device)
#define _real_ibv_get_device_name NEXT_FNC(ibv_get_device_name)
#define _real_ibv_query_device NEXT_FNC(ibv_query_device)
#define _real_ibv_query_port NEXT_FNC(ibv_query_port)
#define _real_ibv_query_pkey NEXT_FNC(ibv_query_pkey)
#define _real_ibv_query_gid NEXT_FNC(ibv_query_gid)
#define _real_ibv_get_cq_event NEXT_FNC(ibv_get_cq_event)
#define _real_ibv_get_async_event NEXT_FNC(ibv_get_async_event)
#define _real_ibv_ack_async_event NEXT_FNC(ibv_ack_async_event)
#define _real_ibv_get_device_guid NEXT_FNC(ibv_get_device_guid)
#define _real_ibv_create_comp_channel NEXT_FNC(ibv_create_comp_channel)
#define _real_ibv_destroy_comp_channel NEXT_FNC(ibv_destroy_comp_channel)
#define _real_ibv_resize_cq NEXT_FNC(ibv_resize_cq)
#define _real_ibv_alloc_pd NEXT_FNC(ibv_alloc_pd)
#define _real_ibv_reg_mr NEXT_FNC(ibv_reg_mr)
#define _real_ibv_create_cq NEXT_FNC(ibv_create_cq)
#define _real_ibv_create_srq NEXT_FNC(ibv_create_srq)
#define _real_ibv_modify_srq NEXT_FNC(ibv_modify_srq)
#define _real_ibv_query_srq NEXT_FNC(ibv_query_srq)
#define _real_ibv_create_qp NEXT_FNC(ibv_create_qp)
#define _real_ibv_modify_qp NEXT_FNC(ibv_modify_qp)
#define _real_ibv_query_qp NEXT_FNC(ibv_query_qp)
#define _real_ibv_ack_cq_events NEXT_FNC(ibv_ack_cq_events)
#define _real_ibv_destroy_cq NEXT_FNC(ibv_destroy_cq)
#define _real_ibv_destroy_qp NEXT_FNC(ibv_destroy_qp)
#define _real_ibv_destroy_srq NEXT_FNC(ibv_destroy_srq)
#define _real_ibv_dereg_mr NEXT_FNC(ibv_dereg_mr)
#define _real_ibv_dealloc_pd NEXT_FNC(ibv_dealloc_pd)
#define _real_ibv_close_device NEXT_FNC(ibv_close_device)
#define _real_ibv_free_device_list NEXT_FNC(ibv_free_device_list)
#endif
