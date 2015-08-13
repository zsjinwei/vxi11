#ifndef __VXI11_IIOC_H__
#define __VXI11_IIOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define _DEBUG
#ifdef _DEBUG
#define IIOC_DBG(format,...) printf(format, ##__VA_ARGS__)
#else
#define IIOC_DBG(format,...)
#endif
//#define NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iio.h>
//#include <glib.h>
#include <malloc.h>
#include <math.h>
#pragma comment(lib,"libiio.lib")

#ifdef __GNUC__
#define __cnst __attribute__((const))
#define __pure __attribute__((pure))
#define __notused __attribute__((unused))
#else
#define __cnst
#define __pure
#define __notused
#endif

#ifdef _WIN32
#   ifdef IIOC_EXPORTS
#		define __api __declspec(dllexport)
#   else
#		define __api __declspec(dllimport)
#   endif
#elif __GNUC__ >= 4
#   define __api __attribute__((visibility ("default")))
#else
#   define __api
#endif

struct extra_ctx_info {
	struct iio_context *ctx;
	unsigned int nb_devices;
	//GSList* dev_list;
};

struct extra_chn_info {
	struct iio_device *dev; //绑定的iio_device
	float *data_ref;//通道数据存储空间地址
	off_t offset;//通道数据存储空间偏移
	bool enabled; //是否使能
};

struct extra_dev_info {
	struct extra_ctx_info *ctx_info;
	bool input_device; //是否是输入设备
	unsigned int nb_channels;
	unsigned int nb_attrs;
	struct iio_buffer *buffer;//绑定的iio_buffer
	unsigned int sample_count;//采样数据数量
	unsigned int buffer_size;//buffer大小
	unsigned int sampling_freq;
	//char adc_scale;
};

__api struct extra_ctx_info *iioc_ctx_open();
__api struct iio_device *iioc_dev_open(struct extra_ctx_info *ctx_info, const char *dev_name);
__api int iioc_read_attr(struct iio_device *dev, const char *attr_name, char *attr_val);
__api int iioc_write_attr(struct iio_device *dev, const char *attr_name, const char *attr_val);
__api int iioc_sampling_setup(struct iio_device *adc_dev,
                              struct iio_device *trigger_dev,
                              unsigned int sampling_freq,
                              unsigned int sample_count
                             );
__api int iioc_sampling_capture(struct iio_device *adc_dev);
__api void iioc_ctx_close(struct extra_ctx_info *ctx_info);
__api void iioc_dev_close(struct iio_device *dev);
__api int iioc_channel_enable(const struct iio_device *dev, const unsigned int *enable, unsigned int nb_en_channels);
__api float *iioc_chn_get_data(const struct iio_device *dev, unsigned int chn, unsigned int *data_size);

#ifdef __cplusplus
}
#endif

#endif  /* __VXI11_IIOC_H__ */
