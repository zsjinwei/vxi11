#include "vxi11.h"
#include "vxi11_scpi.h"
#include "vxi11_iioc.h"
#include "vxi11_svc_func.h"

// === iio struct ===
extern struct extra_ctx_info *ctx_info;
extern struct iio_device *adc_dev;
extern struct iio_device *pwm_dev;
extern struct iio_device *trigger_dev;
extern struct iio_device *dds_dev;
extern struct iio_device *dpot0_dev;
extern struct iio_device *dpot1_dev;
extern struct iio_device *dpot2_dev;
extern struct iio_device *dpot3_dev;
// =======end========

extern Device_ReadResp read_resp;
extern Device_WriteResp write_resp;

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
	(void) context;
	return fwrite(data, 1, len, stdout);
}

scpi_result_t SCPI_Flush(scpi_t * context) {
	return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
	(void) context;
	read_resp.error = VXI_PARAM_ERROR;
	fprintf(stderr, "**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err));
	return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
	if (SCPI_CTRL_SRQ == ctrl) {
		fprintf(stderr, "**SRQ: 0x%X (%d)\r\n", val, val);
	} else {
		fprintf(stderr, "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
	}
	return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
	fprintf(stderr, "**Reset\r\n");
	return SCPI_RES_OK;
}

static scpi_result_t  PTS_IdnQ(scpi_t * context)
{
	int i;
	int offset = 0;
	for (i = 0; i < 4; i++) {
		if (context->idn[i]) {
			strcpy(read_resp.data.data_val + offset, context->idn[i]);
			offset += strlen(context->idn[i]);
		} else {
			strcpy(read_resp.data.data_val + offset, "0");
			offset += 1;
		}
		if (i != 3) {
			strcpy(read_resp.data.data_val + offset, ",");
			offset += 1;
		}
	}
	read_resp.error = VXI_NO_ERROR;
	read_resp.data.data_len = offset + 1;
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

/**
 * [PTS_MeasureChannelEnableQ description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_MeasureChannelEnableQ(scpi_t * context)
{
	int32_t chn_idx;
	struct iio_device *adc_dev = iio_context_find_device(ctx_info->ctx, ADC_NAME);
	if (!adc_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", ADC_NAME);
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	unsigned int nb_channels = iio_device_get_channels_count(adc_dev);
	SCPI_CommandNumbers(context, &chn_idx, 1);
	if (nb_channels == 0 || nb_channels < chn_idx || chn_idx < 0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_channel *chn = iio_device_get_channel(adc_dev, chn_idx);
	if (!chn) {
		SCPI_DBG("get channel-%d error.\n", chn_idx);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	if (iio_channel_is_enabled (chn)) {
		SCPI_DBG("channel-%d is enable.\n", chn_idx);
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = 2;
		strcpy(read_resp.data.data_val, "1");
	}
	else {
		SCPI_DBG("channel-%d is disable.\n", chn_idx);
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = 2;
		strcpy(read_resp.data.data_val, "0");
	}
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

/**
 * [PTS_MeasureEnableQ description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_MeasureEnableQ(scpi_t * context)
{
	int ret = 0;
	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	ret = iioc_read_attr(pwm_dev, PWM_ATTR_ENABLE, read_resp.data.data_val);
	if (ret > 0) {
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = ret;
		write_resp.error = VXI_NO_ERROR;
		return SCPI_RES_OK;
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

static scpi_result_t  PTS_MeasureResistanceQ(scpi_t * context)
{
	return SCPI_RES_OK;
}

/**
 * [PTS_MeasureFrequencyQ description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_MeasureFrequencyQ(scpi_t * context)
{
	char rw_buf[30];
	int ret = 0;
	int period;
	unsigned int freq;
	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	ret = iioc_read_attr(pwm_dev, PWM_ATTR_PERIOD, rw_buf);
	if (ret > 0) {
		period = atoi(rw_buf);
	}
	else {
		SCPI_DBG("IIOC: read attr %s error, return %d.\n", PWM_ATTR_PERIOD, ret);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
	if (period <= 0) {
		freq = 0;
	}
	else {
		freq = (unsigned int)((1.0 / period) * 1000000000.0);
	}
	SCPI_DBG("Read from %s: period=%d, frequency=%d.\n", PWM_NAME, period, freq);
	ret = sprintf(read_resp.data.data_val, "%d", freq);
	if (ret > 0) {
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = ret;
		write_resp.error = VXI_NO_ERROR;
		return SCPI_RES_OK;
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

/**
 * [PTS_MeasurePeriodQ description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_MeasurePeriodQ(scpi_t * context)
{
	int ret = 0;
	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	ret = iioc_read_attr(pwm_dev, PWM_ATTR_PERIOD, read_resp.data.data_val);
	if (ret > 0) {
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = ret;
		write_resp.error = VXI_NO_ERROR;
		return SCPI_RES_OK;
	}
	else {
		SCPI_DBG("IIOC: read attr %s error, return %d.\n", PWM_ATTR_PERIOD, ret);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

/**
 * [PTS_MeasureDutyCycleQ description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_MeasureDutyCycleQ(scpi_t * context)
{
	int ret = 0;
	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	ret = iioc_read_attr(pwm_dev, PWM_ATTR_DUTY_CYCLE, read_resp.data.data_val);
	if (ret > 0) {
		read_resp.error = VXI_NO_ERROR;
		read_resp.data.data_len = ret;
		write_resp.error = VXI_NO_ERROR;
		return SCPI_RES_OK;
	}
	else {
		SCPI_DBG("IIOC: read attr %s error, return %d.\n", PWM_ATTR_DUTY_CYCLE, ret);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

/**
 * [PTS_ConfigureChannelEnable description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_ConfigureChannelEnable(scpi_t * context)
{
	int32_t chn_idx;
	scpi_bool_t chn_enable;

	// read first parameter if present
	if (!SCPI_ParamBool(context, &chn_enable, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_device *adc_dev = iio_context_find_device(ctx_info->ctx, ADC_NAME);
	if (!adc_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", ADC_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	unsigned int nb_channels = iio_device_get_channels_count(adc_dev);
	// read channel index
	SCPI_CommandNumbers(context, &chn_idx, 1);
	if (nb_channels == 0 || nb_channels < chn_idx || chn_idx < 0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_channel *chn = iio_device_get_channel(adc_dev, chn_idx);
	if (!chn) {
		SCPI_DBG("get channel-%d error.\n", chn_idx);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
	struct extra_chn_info *chn_info = iio_channel_get_data(chn);
	if (!chn_info) {
		SCPI_DBG("get channel-%d info data error.\n", chn_idx);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	if (chn_enable) {
		SCPI_DBG("Enable CH%d.\n", chn_idx);
		iio_channel_enable(chn);
		chn_info->enabled = 1;
	}
	else {
		SCPI_DBG("Disable CH%d.\n", chn_idx);
		iio_channel_disable(chn);
		chn_info->enabled = 0;
	}
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

/**
 * [PTS_ConfigureEnable description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_ConfigureEnable(scpi_t * context)
{
	int ret = 0;
	scpi_bool_t conf_enable;
	// read first parameter if present
	if (!SCPI_ParamBool(context, &conf_enable, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	if (conf_enable) {
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_ENABLE, "1");
	}
	else {
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_ENABLE, "0");
	}
	if (ret > 0) {
		write_resp.error = VXI_NO_ERROR;
		return SCPI_RES_OK;
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

static scpi_result_t  PTS_ConfigureResistance(scpi_t * context)
{
	return SCPI_RES_OK;
}

/**
 * [PTS_ConfigureFrequency description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_ConfigureFrequency(scpi_t * context)
{
	int ret = 0;
	unsigned int period;
	unsigned int duty_cycle;
	char rw_buf[30];
	scpi_number_t freq_t;
	// read first parameter if present
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &freq_t, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}
	if (freq_t.value <= 0.0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	period = (unsigned int)((1.0 / freq_t.value) * 1000000000.0);
	duty_cycle = period / 2;

	SCPI_DBG("write to %s: period=%d, duty_cycle=%d.\n", PWM_NAME, period, duty_cycle);
	ret = sprintf(rw_buf, "%u", period);
	if (ret > 0) {
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_DUTY_CYCLE, "0");
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_PERIOD, rw_buf);
		if (ret <= 0) {
			write_resp.error = VXI_IO_ERROR;
			return SCPI_RES_ERR;
		}
		else {
			ret = sprintf(rw_buf, "%u", duty_cycle);
			if (ret > 0) {
				ret = iioc_write_attr(pwm_dev, PWM_ATTR_DUTY_CYCLE, rw_buf);
				if (ret <= 0) {
					write_resp.error = VXI_IO_ERROR;
					return SCPI_RES_ERR;
				}
				else {
					write_resp.error = VXI_NO_ERROR;
					return SCPI_RES_OK;
				}
			}
			else {
				write_resp.error = VXI_IO_ERROR;
				return SCPI_RES_ERR;
			}
		}
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

/**
 * [PTS_ConfigurePeriod description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_ConfigurePeriod(scpi_t * context)
{
	int ret = 0;
	unsigned int period;
	char rw_buf[30];
	scpi_number_t period_t;
	// read first parameter if present
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &period_t, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}
	if (period_t.value <= 0.0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	if (period_t.unit == 6) {
		period = (unsigned int)(period_t.value * 1000000000.0);
	}
	else {
		period = (unsigned int)period_t.value;
	}
	SCPI_DBG("set period = %d.\n", period);
	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: cannot find %s.\n", PWM_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	ret = sprintf(rw_buf, "%u", period);
	if (ret > 0) {
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_PERIOD, rw_buf);
		if (ret <= 0) {
			SCPI_DBG("IIOC: write attr %s error, return %d.\n", PWM_ATTR_PERIOD, ret);
			write_resp.error = VXI_IO_ERROR;
			return SCPI_RES_ERR;
		}
		else {
			write_resp.error = VXI_NO_ERROR;
			return SCPI_RES_OK;
		}
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

/**
 * [PTS_ConfigureDutyCycle description]
 * @param  context [description]
 * @return         [description]
 */
static scpi_result_t  PTS_ConfigureDutyCycle(scpi_t * context)
{
	int ret = 0;
	unsigned int duty_cycle;
	char rw_buf[30];
	scpi_number_t duty_cycle_t;
	// read first parameter if present
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &duty_cycle_t, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}
	if (duty_cycle_t.value <= 0.0) {
		return SCPI_RES_ERR;
	}
	if (duty_cycle_t.unit == 6) {
		duty_cycle = (unsigned int)(duty_cycle_t.value * 1000000000.0);
	}
	else {
		duty_cycle = (unsigned int)duty_cycle_t.value;
	}
	SCPI_DBG("set duty_cycle = %d.\n", duty_cycle);

	struct iio_device *pwm_dev = iio_context_find_device(ctx_info->ctx, PWM_NAME);
	if (!pwm_dev) {
		SCPI_DBG("IIOC: write attr %s error, return %d.\n", PWM_ATTR_DUTY_CYCLE, ret);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	ret = sprintf(rw_buf, "%u", duty_cycle);
	if (ret > 0) {
		ret = iioc_write_attr(pwm_dev, PWM_ATTR_DUTY_CYCLE, rw_buf);
		if (ret <= 0) {
			SCPI_DBG("IIOC: write attr %s error, return %d.\n", PWM_ATTR_DUTY_CYCLE, ret);
			write_resp.error = VXI_IO_ERROR;
			return SCPI_RES_ERR;
		}
		else {
			write_resp.error = VXI_NO_ERROR;
			return SCPI_RES_OK;
		}
	}
	else {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
}

static scpi_result_t  PTS_BufferClear(scpi_t * context)
{
	read_resp.error = VXI_NO_ERROR;
	read_resp.data.data_len = 0;
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

static scpi_result_t  PTS_BufferSamplingSetup(scpi_t * context)
{
	int ret = 0;
	scpi_number_t sampling_freq_t, sample_count_t;
	// read first parameter if present
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &sampling_freq_t, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	// read second paraeter if present
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &sample_count_t, TRUE)) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	if (sampling_freq_t.value <= 0.0 || sample_count_t.value <= 0.0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_device *adc_dev = iio_context_find_device(ctx_info->ctx, ADC_NAME);
	struct iio_device *trigger_dev = iio_context_find_device(ctx_info->ctx, TRIGGER_NAME);
	if ((!adc_dev) || (!trigger_dev)) {
		SCPI_DBG("Cannot find %s or %s.\n", ADC_NAME, TRIGGER_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	SCPI_DBG("Sampling Setup: sampling_freq=%f, sample_count=%f.\n", sampling_freq_t.value, sample_count_t.value);
	ret = iioc_sampling_setup(adc_dev, trigger_dev, sampling_freq_t.value, sample_count_t.value);
	if (ret) {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

static scpi_result_t  PTS_BufferSamplingCapture(scpi_t * context)
{
	int ret = 0;
	struct iio_device *adc_dev = iio_context_find_device(ctx_info->ctx, ADC_NAME);
	if (!adc_dev) {
		SCPI_DBG("IIOC: Cannot find device %s.\n", ADC_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
	SCPI_DBG("Sampling start.\n");
	ret = iioc_sampling_capture(adc_dev);
	if (ret) {
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}

static scpi_result_t  PTS_BufferFetchQ(scpi_t * context)
{
	int32_t chn_idx;
	bool chn_enable;
	struct iio_device *adc_dev = iio_context_find_device(ctx_info->ctx, ADC_NAME);
	if (!adc_dev) {
		SCPI_DBG("IIOC: Cannot find device %s.\n", ADC_NAME);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	unsigned int nb_channels = iio_device_get_channels_count(adc_dev);
	// read channel index
	SCPI_CommandNumbers(context, &chn_idx, 1);
	if (nb_channels == 0 || nb_channels < chn_idx || chn_idx < 0) {
		write_resp.error = VXI_PARAM_ERROR;
		return SCPI_RES_ERR;
	}

	struct iio_channel *chn = iio_device_get_channel(adc_dev, chn_idx);
	if (!chn) {
		SCPI_DBG("Cannot get CH%d.\n", chn_idx);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	struct extra_chn_info *chn_info = iio_channel_get_data(chn);
	if (!chn_info)
	{
		SCPI_DBG("Cannot get CH%d info data.\n", chn_idx);
		write_resp.error = VXI_IO_ERROR;
		return SCPI_RES_ERR;
	}

	memcpy((char *)read_resp.data.data_val, (char *)chn_info->data_ref, chn_info->offset * 2);
	read_resp.error = VXI_NO_ERROR;
	read_resp.data.data_len = chn_info->offset * 2;
	SCPI_DBG("get %d bytes data from CH%d.\n", read_resp.data.data_len, chn_idx);
	write_resp.error = VXI_NO_ERROR;
	return SCPI_RES_OK;
}



static scpi_result_t TEST_Bool(scpi_t * context) {
	scpi_bool_t param1;
	fprintf(stderr, "TEST:BOOL\r\n"); // debug command name

	// read first parameter if present
	if (!SCPI_ParamBool(context, &param1, TRUE)) {
		return SCPI_RES_ERR;
	}

	fprintf(stderr, "\tP1=%d\r\n", param1);

	return SCPI_RES_OK;
}

scpi_choice_def_t trigger_source[] = {
	{"BUS", 5},
	{"IMMediate", 6},
	{"EXTernal", 7},
	SCPI_CHOICE_LIST_END /* termination of option list */
};


static scpi_result_t TEST_ChoiceQ(scpi_t * context) {

	int32_t param;
	const char * name;

	if (!SCPI_ParamChoice(context, trigger_source, &param, TRUE)) {
		return SCPI_RES_ERR;
	}

	SCPI_ChoiceToName(trigger_source, param, &name);
	fprintf(stderr, "\tP1=%s (%ld)\r\n", name, (long int)param);

	SCPI_ResultInt(context, param);

	return SCPI_RES_OK;
}

static scpi_result_t TEST_Numbers(scpi_t * context) {
	int32_t numbers[2];

	SCPI_CommandNumbers(context, numbers, 2);

	fprintf(stderr, "TEST numbers %d %d\r\n", numbers[0], numbers[1]);

	return SCPI_RES_OK;
}

static scpi_result_t TEST_Text(scpi_t * context) {
	char buffer[100];
	size_t copy_len;

	buffer[0] = 0;
	SCPI_ParamCopyText(context, buffer, 100, &copy_len, FALSE);

	fprintf(stderr, "TEXT: ***%s***\r\n", buffer);

	return SCPI_RES_OK;
}

static scpi_result_t TEST_ArbQ(scpi_t * context) {
	const char * data;
	size_t len;

	SCPI_ParamArbitraryBlock(context, &data, &len, FALSE);

	SCPI_ResultArbitraryBlock(context, data, len);

	return SCPI_RES_OK;
}

/**
 * Reimplement IEEE488.2 *TST?
 *
 * Result should be 0 if everything is ok
 * Result should be 1 if something goes wrong
 *
 * Return SCPI_RES_OK
 */
static scpi_result_t My_CoreTstQ(scpi_t * context) {

	SCPI_ResultInt(context, 0);

	return SCPI_RES_OK;
}

static const scpi_command_t scpi_commands[] = {
	/* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
	{ .pattern = "*CLS", .callback = SCPI_CoreCls,},
	{ .pattern = "*ESE", .callback = SCPI_CoreEse,},
	{ .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
	{ .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
	{ .pattern = "*IDN?", .callback = PTS_IdnQ,},
	{ .pattern = "*OPC", .callback = SCPI_CoreOpc,},
	{ .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
	{ .pattern = "*RST", .callback = SCPI_CoreRst,},
	{ .pattern = "*SRE", .callback = SCPI_CoreSre,},
	{ .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
	{ .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
	{ .pattern = "*TST?", .callback = My_CoreTstQ,},
	{ .pattern = "*WAI", .callback = SCPI_CoreWai,},

	/* Required SCPI commands (SCPI std V1999.0 4.2.1) */
	{.pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
	{.pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
	{.pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

	//{.pattern = "STATus:OPERation?", .callback = scpi_stub_callback,},
	//{.pattern = "STATus:OPERation:EVENt?", .callback = scpi_stub_callback,},
	//{.pattern = "STATus:OPERation:CONDition?", .callback = scpi_stub_callback,},
	//{.pattern = "STATus:OPERation:ENABle", .callback = scpi_stub_callback,},
	//{.pattern = "STATus:OPERation:ENABle?", .callback = scpi_stub_callback,},

	{.pattern = "STATus:QUEStionable[:EVENt]?", .callback = SCPI_StatusQuestionableEventQ,},
	//{.pattern = "STATus:QUEStionable:CONDition?", .callback = scpi_stub_callback,},
	{.pattern = "STATus:QUEStionable:ENABle", .callback = SCPI_StatusQuestionableEnable,},
	{.pattern = "STATus:QUEStionable:ENABle?", .callback = SCPI_StatusQuestionableEnableQ,},

	{.pattern = "STATus:PRESet", .callback = SCPI_StatusPreset,},

	/* PTS */
	{.pattern = "MEASure:CHANnel#:ENABle?", .callback = PTS_MeasureChannelEnableQ,},
	{.pattern = "MEASure:ENABle?", .callback = PTS_MeasureEnableQ,},
	{.pattern = "MEASure:RESistance?", .callback = PTS_MeasureResistanceQ,},
	{.pattern = "MEASure:FREQuency?", .callback = PTS_MeasureFrequencyQ,},
	{.pattern = "MEASure:PERiod?", .callback = PTS_MeasurePeriodQ,},
	{.pattern = "MEASure:DCYCle?", .callback = PTS_MeasureDutyCycleQ,},

	{.pattern = "CONFigure:CHANnel#:ENABle", .callback = PTS_ConfigureChannelEnable,},
	{.pattern = "CONFigure:ENABle", .callback = PTS_ConfigureEnable,},
	{.pattern = "CONFigure:RESistance", .callback = PTS_ConfigureResistance,},
	{.pattern = "CONFigure:FREQuency", .callback = PTS_ConfigureFrequency,},
	{.pattern = "CONFigure:PERiod", .callback = PTS_ConfigurePeriod,},
	{.pattern = "CONFigure:DCYCle", .callback = PTS_ConfigureDutyCycle,},

	{.pattern = "BUFFer:CLEar", .callback = PTS_BufferClear,},
	{.pattern = "BUFFer:SSETup", .callback = PTS_BufferSamplingSetup,},
	{.pattern = "BUFFer:SCAPture", .callback = PTS_BufferSamplingCapture,},
	{.pattern = "BUFFer:FETCh:CHANnel#?", .callback = PTS_BufferFetchQ,},
	/* DMM */
	// {.pattern = "MEASure:VOLTage:DC?", .callback = DMM_MeasureVoltageDcQ,},
	// {.pattern = "CONFigure:VOLTage:DC", .callback = DMM_ConfigureVoltageDc,},
	// {.pattern = "MEASure:VOLTage:DC:RATio?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:VOLTage:AC?", .callback = DMM_MeasureVoltageAcQ,},
	// {.pattern = "MEASure:CURRent:DC?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:CURRent:AC?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:RESistance?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:FRESistance?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:FREQuency?", .callback = SCPI_StubQ,},
	// {.pattern = "MEASure:PERiod?", .callback = SCPI_StubQ,},

	// {.pattern = "SYSTem:COMMunication:TCPIP:CONTROL?", .callback = SCPI_SystemCommTcpipControlQ,},

	{.pattern = "TEST:BOOL", .callback = TEST_Bool,},
	{.pattern = "TEST:CHOice?", .callback = TEST_ChoiceQ,},
	{.pattern = "TEST#:NUMbers#", .callback = TEST_Numbers,},
	{.pattern = "TEST:TEXT", .callback = TEST_Text,},
	{.pattern = "TEST:ARBitrary?", .callback = TEST_ArbQ,},

	SCPI_CMD_LIST_END
};

static scpi_interface_t scpi_interface = {
	.error = SCPI_Error,
	.write = SCPI_Write,
	.control = SCPI_Control,
	.flush = SCPI_Flush,
	.reset = SCPI_Reset,
};

#define SCPI_INPUT_BUFFER_LENGTH 256
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

static scpi_reg_val_t scpi_regs[SCPI_REG_COUNT];


scpi_t scpi_context = {
	.cmdlist = scpi_commands,
	.buffer = {
		.length = SCPI_INPUT_BUFFER_LENGTH,
		.data = scpi_input_buffer,
	},
	.interface = &scpi_interface,
	.registers = scpi_regs,
	.units = scpi_units_def,
	.idn = {"SYSU-SIST637", "INSTR2015", NULL, "08-13"},
};
