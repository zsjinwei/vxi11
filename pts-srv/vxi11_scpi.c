#include "vxi11_scpi.h"

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
	(void) context;
	return fwrite(data, 1, len, stdout);
}

scpi_result_t SCPI_Flush(scpi_t * context) {
	return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
	(void) context;

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
	{ .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
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
	{.pattern = "MEASure:VOLTage:DC?", .callback = DMM_MeasureVoltageDcQ,},
	{.pattern = "MEASure:RESistance?", .callback = SCPI_StubQ,},
	{.pattern = "MEASure:FRESistance?", .callback = SCPI_StubQ,},
	{.pattern = "MEASure:FREQuency?", .callback = SCPI_StubQ,},
	{.pattern = "MEASure:PERiod?", .callback = SCPI_StubQ,},
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
	.idn = {"MANUFACTURE", "INSTR2013", NULL, "01-02"},
};
