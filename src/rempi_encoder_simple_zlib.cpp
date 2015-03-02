#include <vector>
#include <algorithm>

#include "rempi_err.h"
#include "rempi_event.h"
#include "rempi_encoder.h"
#include "rempi_config.h"
#include "rempi_clock_delta_compression.h"
#include "rempi_compression_util.h"

#define MAX_CDC_INPUT_FORMAT_LENGTH (1024 * 1024)


/* ==================================== */
/*      CLASS rempi_encoder_zlib         */
/* ==================================== */


rempi_encoder_simple_zlib::rempi_encoder_simple_zlib(int mode): rempi_encoder_zlib(mode)
{}

rempi_encoder_input_format* rempi_encoder_simple_zlib::create_encoder_input_format()
{
  return new rempi_encoder_input_format();
}





