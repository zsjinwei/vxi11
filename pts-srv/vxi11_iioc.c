#define _CRT_SECURE_NO_WARNINGS
#include "vxi11_iioc.h"

static bool device_type_get(const struct iio_device *dev, int type);
static bool is_input_device(const struct iio_device *dev);
static bool is_output_device(const struct iio_device *dev);
static void enable_all_channels(struct iio_device *dev);
static void disable_all_channels(struct iio_device *dev);
static ssize_t demux_sample(const struct iio_channel *chn,
                            void *sample, size_t size, void *d);

struct extra_ctx_info *iioc_ctx_open(void)
{

	struct extra_ctx_info *ctx_info = (struct extra_ctx_info *)calloc(1, sizeof(*ctx_info));
	if (!ctx_info) {
		IIOC_DBG("Can not calloc memory for struct extra_ctx_info.\n");
		return NULL;
	}
	ctx_info->ctx = iio_create_local_context();
	if (!ctx_info->ctx) {
		IIOC_DBG("iio_create_local_context error.\n");
		return NULL;
	}

	ctx_info->nb_devices = iio_context_get_devices_count(ctx_info->ctx);
	IIOC_DBG("There are %d devices in local.\n", ctx_info->nb_devices);
	if (ctx_info->nb_devices == 0) {
		IIOC_DBG("No device found.\n");
		return NULL;
	}

	return ctx_info;
	//for (i = 0; i < ctx_info->nb_devices; i++) {
	//	struct iio_device *dev = iio_context_get_device(ctx_info->ctx, i);
	//	char *dev_name = iio_device_get_name(dev);
	//	char *dev_id = iio_device_get_id(dev);
	//	unsigned int nb_channels = iio_device_get_channels_count(dev);
	//	unsigned int nb_attrs = iio_device_get_attrs_count(dev);
	//
	//
	//	IIOC_DBG("Device%d: %s - %s, Channels count: %d, attritubes count: %d, is_input_device: %s.\n", i, dev_id, dev_name, nb_channels, nb_attrs, dev_info->input_device ? "yes" : "no");

	//	for (j = 0; j < nb_channels; j++) {
	//		struct iio_channel *ch = iio_device_get_channel(dev, j);
	//		char *ch_id = iio_channel_get_id(ch);
	//		char *ch_name = iio_channel_get_name(ch);
	//		IIOC_DBG("\tCH%d: %s - %s.\n", j, ch_id, ch_name);
	//		struct extra_chn_info *info = calloc(1, sizeof(*info));
	//		info->dev = dev;
	//		iio_channel_set_data(ch, info);
	//	}

	//	for (k = 0; k < nb_attrs; k++) {
	//		char *attr_name = iio_device_get_attr(dev, k);
	//		IIOC_DBG("\t>> Attr%d: %s.\n", k, attr_name);
	//	}
	//}
}

struct iio_device *iioc_dev_open(struct extra_ctx_info *ctx_info, const char *dev_name)
{
	unsigned int j, k;
	//Ñ°ÕÒÉè±¸
	struct iio_device *dev = iio_context_find_device(ctx_info->ctx, dev_name);
	if (!dev) {
		IIOC_DBG("No such device(%s).\n", dev_name);
		return NULL;
	}
	IIOC_DBG("Open device - %s.\n", dev_name);
	//·ÖÅädev_info¿Õ¼ä
	struct extra_dev_info *dev_info = (struct extra_dev_info *)calloc(1, sizeof(*dev_info));
	if (!dev_info) {
		IIOC_DBG("Can not calloc memory for struct extra_dev_info.\n");
		return NULL;
	}
	unsigned int nb_channels = iio_device_get_channels_count(dev);
	unsigned int nb_attrs = iio_device_get_attrs_count(dev);
	dev_info->input_device = is_input_device(dev);
	dev_info->nb_channels = nb_channels;
	dev_info->nb_attrs = nb_attrs;
	dev_info->ctx_info = ctx_info;
	iio_device_set_data(dev, dev_info);

	for (j = 0; j < nb_channels; j++) {
		struct iio_channel *ch = iio_device_get_channel(dev, j);
#ifdef _DEBUG
		const char *ch_id = iio_channel_get_id(ch);
		const char *ch_name = iio_channel_get_name(ch);
		IIOC_DBG("\tCH%d: %s - %s.\n", j, ch_id, ch_name);
#endif
		struct extra_chn_info *chn_info = (struct extra_chn_info *)calloc(1, sizeof(*chn_info));
		if (!chn_info) {
			goto error_calloc_chn_info;
		}
		chn_info->dev = dev;
		iio_channel_set_data(ch, chn_info);
	}
#ifdef _DEBUG

	for (k = 0; k < nb_attrs; k++) {
		const char *attr_name = iio_device_get_attr(dev, k);
		IIOC_DBG("\t>> Attr%d: %s.\n", k, attr_name);
	}
#endif
	return dev;
error_calloc_chn_info:
	for (k = 0; k < nb_channels; k++) {
		struct iio_channel *ch = iio_device_get_channel(dev, k);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		if (chn_info)
			free(chn_info);
	}
	IIOC_DBG("Can not calloc memory for struct extra_chn_info.\n");
	return NULL;
}

int iioc_read_attr(struct iio_device *dev, const char *attr_name, char *attr_val)
{
	char *ret;
	if (attr_val == NULL || attr_val == "" || attr_name == NULL || attr_name == "") {
		IIOC_DBG("Parameter attr_name or attr_val is NULL.\n");
		return -EINVAL;
	}
	ret = (char *)iio_device_find_attr(dev, attr_name);
	if (!ret) {
		IIOC_DBG("%s attribute is not found.\n", attr_name);
		return -EINVAL;
	}

	return iio_device_attr_read(dev, attr_name, attr_val, 30);
}

int iioc_write_attr(struct iio_device *dev, const char *attr_name, const char *attr_val)
{
	char *ret;
	if (attr_val == NULL || attr_val == "" || attr_name == NULL || attr_name == "") {
		IIOC_DBG("Parameter attr_name or attr_val is NULL.\n");
		return -EINVAL;
	}
	ret = (char *)iio_device_find_attr(dev, attr_name);
	if (!ret) {
		IIOC_DBG("%s attribute is not found.\n", attr_name);
		return -EINVAL;
	}

	return iio_device_attr_write(dev, attr_name, attr_val);
}

int iioc_sampling_setup(struct iio_device *adc_dev,
                        struct iio_device *trigger_dev,
                        unsigned int sampling_freq,
                        unsigned int sample_count
                       )
{
	unsigned int i;
	int ret;
	unsigned int min_timeout = 2000;
	if (!adc_dev || !trigger_dev)
		return -ENODEV;
	unsigned int nb_channels = iio_device_get_channels_count(adc_dev);
	if (nb_channels == 0) {
		IIOC_DBG("There is 0 Channel in adc_device.\n");
		return -EIO;
	}
	unsigned int sample_size = iio_device_get_sample_size(adc_dev);
	if (sample_size == 0) {
		IIOC_DBG("Sample Size is 0.\n");
		return -ENODATA;
	}
	struct extra_dev_info *dev_info = iio_device_get_data(adc_dev);
	if (dev_info->input_device == false) {
		IIOC_DBG("adc_dev is not an input device.\n");
		return -EIO;
	}
	//°ó¶¨trigger
	ret = iio_device_set_trigger(adc_dev, trigger_dev);
	if (ret)
	{
#ifdef _DEBUG
		const char *trigger_name = iio_device_get_name(trigger_dev);
		const char *adc_name = iio_device_get_name(adc_dev);
		IIOC_DBG("Can not bind the %s with %s.\n", trigger_name, adc_name);
#endif
		return -EIO;
	}
	//É¾³ý¾Ébuffer
	if (dev_info->buffer)
		iio_buffer_destroy(dev_info->buffer);
	dev_info->buffer = NULL;

	//ÉèÖÃsample_count(buffer´óÐ¡)
	dev_info->sample_count = sample_count;
	//Ê¹ÄÜÍ¨µÀ£¬²¢ÎªÃ¿¸öÍ¨µÀ·ÖÅäÄÚ´æ¿Õ¼ä
	for (i = 0; i < nb_channels; i++) {
		struct iio_channel *ch = iio_device_get_channel(adc_dev, i);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		if (chn_info->enabled)
			iio_channel_enable(ch);
		else
			iio_channel_disable(ch);

		if (chn_info->data_ref)
			free(chn_info->data_ref);
		chn_info->data_ref = (int16_t *)calloc(dev_info->sample_count, sizeof(int16_t));
		if (!chn_info->data_ref) {
			IIOC_DBG("Can not calloc channel data mem.\n");
			goto error_calloc_chn_data_ref;
		}
	}
	dev_info->sampling_freq = sampling_freq;
	//ÖØÐÂ°ó¶¨Êý¾Ý
	iio_device_set_data(adc_dev, dev_info);
	//ÉèÖÃ³¬Ê±
	if (sampling_freq > 0) {
		/* 2 x capture time + 2s */
		unsigned int timeout = dev_info->sample_count * 1000 / sampling_freq;
		timeout += 2000;
		if (timeout > min_timeout)
			min_timeout = timeout;
	}
	if (dev_info->ctx_info->ctx)
		iio_context_set_timeout(dev_info->ctx_info->ctx, min_timeout);

	return 0;
error_calloc_chn_data_ref:
	for (i = 0; i < nb_channels; i++) {
		struct iio_channel *ch = iio_device_get_channel(adc_dev, i);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		if (chn_info->data_ref)
			free(chn_info->data_ref);
	}
	return -ENOMEM;
}

int iioc_sampling_capture(struct iio_device *adc_dev)
{
	unsigned int i;
	off_t offset = 0;
	assert(adc_dev);
	struct extra_dev_info *dev_info = iio_device_get_data(adc_dev);
	ssize_t sample_count = dev_info->sample_count;
	unsigned int nb_channels = dev_info->nb_channels;

	if (dev_info->buffer == NULL) {
		dev_info->buffer_size = sample_count;
		dev_info->buffer = iio_device_create_buffer(adc_dev,
		                   sample_count, false);
		if (!dev_info->buffer) {
			IIOC_DBG("Error: Unable to create buffer: %s\n", strerror(errno));
			return -errno;
		}
	}

	/* Reset the data offset for all channels */
	for (i = 0; i < nb_channels; i++) {
		struct iio_channel *ch = iio_device_get_channel(adc_dev, i);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		chn_info->offset = 0;
	}
	IIOC_DBG("Enter buffer refill loop.\n");
	while (true) {
		ssize_t ret = iio_buffer_refill(dev_info->buffer);
		if (ret < 0) {
			IIOC_DBG("Error while reading data: %s\n", strerror(-ret));
			return ret;
		}
		else {
			IIOC_DBG("Read %d bytes from buffer.\n", ret);
		}

		ret /= iio_buffer_step(dev_info->buffer);
		if (ret >= sample_count) {
			iio_buffer_foreach_sample(
			    dev_info->buffer, demux_sample, NULL);

			if (ret >= sample_count * 2) {
				printf("Decreasing buffer size\n");
				iio_buffer_destroy(dev_info->buffer);
				dev_info->buffer_size /= 2;
				dev_info->buffer = iio_device_create_buffer(adc_dev,
				                   dev_info->buffer_size, false);
			}
			break;
		}

		printf("Increasing buffer size\n");
		iio_buffer_destroy(dev_info->buffer);
		dev_info->buffer_size *= 2;
		dev_info->buffer = iio_device_create_buffer(adc_dev,
		                   dev_info->buffer_size, false);
	}

	return 0;
}
void iioc_ctx_close(struct extra_ctx_info *ctx_info)
{
	if (ctx_info->ctx)
		iio_context_destroy(ctx_info->ctx);
	if (ctx_info)
		free(ctx_info);
}

void iioc_dev_close(struct iio_device *dev)
{
	unsigned int j;
	struct extra_dev_info *dev_info = iio_device_get_data(dev);
	if (!dev_info)
		return;

	unsigned int nb_channels = iio_device_get_channels_count(dev);

	for (j = 0; j < nb_channels; j++) {
		struct iio_channel *ch = iio_device_get_channel(dev, j);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		if (chn_info->data_ref) {
			free(chn_info->data_ref);
			chn_info->data_ref = NULL;
		}
		if (chn_info) {
			free(chn_info);
			chn_info = NULL;
		}
	}
	if (dev_info->buffer) {
		iio_buffer_destroy(dev_info->buffer);
		dev_info->buffer = NULL;
		dev_info->buffer_size = 0;
	}
	free(dev_info);
	dev_info = NULL;
}

int iioc_channel_enable(const struct iio_device *dev, const unsigned int *enable, unsigned int nb_en_channels)
{
	unsigned int i, nb_channels;
	if (!dev) {
		IIOC_DBG("No such device.\n");
		return -ENODEV;
	}
	if (!enable) {
		return -EINVAL;
	}
	unsigned int nb_dev_channels = iio_device_get_channels_count(dev);
	if (nb_en_channels < nb_dev_channels)
		nb_channels = nb_en_channels;
	else
		nb_channels = nb_dev_channels;

	for (i = 0; i < nb_channels; i++) {
		struct iio_channel *ch = iio_device_get_channel(dev, i);
		struct extra_chn_info *chn_info = iio_channel_get_data(ch);
		if (enable[i]) {
			chn_info->enabled = true;
			iio_channel_enable(ch);
		}
		else {
			chn_info->enabled = false;
			iio_channel_disable(ch);
		}
	}
	return 0;
}


short *iioc_chn_get_data(const struct iio_device *dev, unsigned int chn, unsigned int *data_size)
{
	assert(dev);
	struct iio_channel *ch = iio_device_get_channel(dev, chn);
	if (!ch) {
		IIOC_DBG("CH%d is not found.\n", chn);
		return NULL;
	}
	struct extra_chn_info *chn_info = iio_channel_get_data(ch);
	if (!chn_info) {
		IIOC_DBG("struct extra_chn_info point is NULL.\n");
		return NULL;
	}
	if (!chn_info->enabled) {
		IIOC_DBG("CH%d is not enabled.\n", chn);
		return NULL;
	}
	*data_size = chn_info->offset;
	return chn_info->data_ref;
}
/*
* Check if a device has scan elements and if it is an output device (type = 0)
* or an input device (type = 1).
*/
static bool device_type_get(const struct iio_device *dev, int type)
{
	struct iio_channel *ch;
	int nb_channels, i;

	if (!dev)
		return false;

	nb_channels = iio_device_get_channels_count(dev);
	for (i = 0; i < nb_channels; i++) {
		ch = iio_device_get_channel(dev, i);
		if (iio_channel_is_scan_element(ch) &&
		        (type ? !iio_channel_is_output(ch) : iio_channel_is_output(ch)))
			return true;
	}

	return false;
}


static bool is_input_device(const struct iio_device *dev)
{
	return device_type_get(dev, 1);
}

static bool is_output_device(const struct iio_device *dev)
{
	return device_type_get(dev, 0);
}

static void enable_all_channels(struct iio_device *dev)
{
	unsigned int i, nb_channels = iio_device_get_channels_count(dev);
	for (i = 0; i < nb_channels; i++)
		iio_channel_enable(iio_device_get_channel(dev, i));
}

static void disable_all_channels(struct iio_device *dev)
{
	unsigned int i, nb_channels = iio_device_get_channels_count(dev);
	for (i = 0; i < nb_channels; i++)
		iio_channel_disable(iio_device_get_channel(dev, i));
}

static ssize_t demux_sample(const struct iio_channel *chn,
                            void *sample, size_t size, void *d)
{
	struct extra_chn_info *chn_info = iio_channel_get_data(chn);
	struct extra_dev_info *dev_info = iio_device_get_data(chn_info->dev);
	const struct iio_data_format *format = iio_channel_get_data_format(chn);

	/* Prevent buffer overflow */
	if ((unsigned long)chn_info->offset == (unsigned long)dev_info->sample_count)
		return 0;

	if (size == 1) {
		int8_t val;
		iio_channel_convert(chn, &val, sample);
		if (format->is_signed)
			*(chn_info->data_ref + chn_info->offset++) = (int8_t)val;
		else
			*(chn_info->data_ref + chn_info->offset++) = (uint8_t)val;
	}
	else if (size == 2) {
		int16_t val;
		iio_channel_convert(chn, &val, sample);
		if (format->is_signed)
			*(chn_info->data_ref + chn_info->offset++) = (int16_t)val;
		else
			*(chn_info->data_ref + chn_info->offset++) = (uint16_t)val;
	}
	else {
		int32_t val;
		iio_channel_convert(chn, &val, sample);
		if (format->is_signed)
			*(chn_info->data_ref + chn_info->offset++) = (int32_t)val;
		else
			*(chn_info->data_ref + chn_info->offset++) = (uint32_t)val;
	}

	return size;
}
