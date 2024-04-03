#include <Arduino.h>
#include <ESP32_JPEG_Library.h>

static jpeg_error_t jpeg_decode(uint8_t *input_buf, int len, uint8_t *output_buf) {
    jpeg_error_t ret = JPEG_ERR_OK;
    int inbuf_consumed = 0;

    // Generate default configuration
    jpeg_dec_config_t config = {
        .output_type = JPEG_RAW_TYPE_GRAY,
        .rotate = JPEG_ROTATE_0D,
    };

    // Empty handle to jpeg_decoder
    jpeg_dec_handle_t *jpeg_dec = NULL;

    // Create jpeg_dec
    jpeg_dec = jpeg_dec_open(&config);

    // Create io_callback handle
    jpeg_dec_io_t *jpeg_io = (jpeg_dec_io_t *)calloc(1, sizeof(jpeg_dec_io_t));
    if (jpeg_io == NULL) {
        return JPEG_ERR_MEM;
    }

    // Create out_info handle
    jpeg_dec_header_info_t *out_info = (jpeg_dec_header_info_t *)calloc(1, sizeof(jpeg_dec_header_info_t));
    if (out_info == NULL) {
        return JPEG_ERR_MEM;
    }
    // Set input buffer and buffer len to io_callback
    jpeg_io->inbuf = input_buf;
    jpeg_io->inbuf_len = len;

    // Parse jpeg picture header and get picture for user and decoder
    ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
    if (ret < 0) {
        Serial.println("JPEG decode parse failed");
        goto _exit;
    }

    jpeg_io->outbuf = output_buf;
    inbuf_consumed = jpeg_io->inbuf_len - jpeg_io->inbuf_remain;
    jpeg_io->inbuf = input_buf + inbuf_consumed;
    jpeg_io->inbuf_len = jpeg_io->inbuf_remain;

    // Start decode jpeg raw data
    ret = jpeg_dec_process(jpeg_dec, jpeg_io);
    if (ret < 0) {
        Serial.println("JPEG decode process failed");
        goto _exit;
    }

    _exit:
        // Decoder deinitialize
        jpeg_dec_close(jpeg_dec);
        free(out_info);
        free(jpeg_io);
        return ret;
}