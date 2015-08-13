#ifndef __VXI11__SVC_FUNC_H__
#define __VXI11__SVC_FUNC_H__

#define RESP_DATA_BUF_SIZE 2000

#define __VXI11_SVC_DEBUG__
#ifdef __VXI11_SVC_DEBUG__
#define VXI11_SVC_DEBUG(format,...) printf(format, ##__VA_ARGS__)
#else
#define VXI11_SVC_DEBUG(format,...)
#endif

#define VXI_NO_ERROR				0
#define VXI_SYNTAX_ERROR			1
#define VXI_DEV_NOT_ACCESS			3
#define VXI_INVALID_LID				4
#define VXI_PARAM_ERROR				5
#define VXI_CHAN_NOT_ESTAB			6
#define VXI_OP_NOT_SUPPORT			8
#define VXI_OUT_OF_RES				9
#define VXI_DEV_LOCK_BY_OTH_LINK	11
#define VXI_NO_LOCK_HELD_BY_LINK	12
#define VXI_IO_TIMEOUT				15
#define VXI_IO_ERROR				17
#define VXI_INVALID_ADDR			21
#define VXI_ABORT					23
#define VXI_CHAN_ALREADY_EATAB		29

#define MAX_RECV_SIZE 1024

#endif /* __VXI11__SVC_FUNC_H__ */