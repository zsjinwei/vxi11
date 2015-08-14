#ifndef __VXI11_SCPI_H__
#define __VXI11_SCPI_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi/scpi.h"

#define SCPI_DEBUG
#ifdef SCPI_DEBUG
#define SCPI_DBG(format,...) fprintf(stderr, format, ##__VA_ARGS__)
#else
#define SCPI_DBG(format,...)
#endif

extern scpi_t scpi_context;

size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
int SCPI_Error(scpi_t * context, int_fast16_t err);
scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
scpi_result_t SCPI_Reset(scpi_t * context);
scpi_result_t SCPI_Flush(scpi_t * context);

#endif /* __VXI11_SCPI_H__ */