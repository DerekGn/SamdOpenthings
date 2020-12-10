/* MIT License

Copyright (c) 2020 DerekGn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "openthings.h"
#include "openthings_message.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define OPENTHINGS_CRC_START 5

static int16_t crc( const uint8_t const *buf, size_t size );

/*-----------------------------------------------------------*/
void openthings_init_message( struct openthings_messge_context *const context,
                              const uint8_t manufacturer_id,
                              const uint8_t product_id, const uint16_t pip,
                              const uint32_t sensor_id )
{
    struct openthings_message_header *header =
        (struct openthings_message_header *)context->openthings_message_buffer;

	context->eom = 0;

    header->manu_id = manufacturer_id;
    header->prod_id = product_id;
    header->pip = pip;
    header->sensor_id_0 = ( uint8_t )( sensor_id & 0xFF );
    header->sensor_id_1 = ( uint8_t )( ( sensor_id & 0xFF00 ) >> 8 );
    header->sensor_id_2 = ( uint8_t )( ( sensor_id & 0xFF0000 ) >> 16 );

    context->eom += sizeof( struct openthings_message_header );
}
/*-----------------------------------------------------------*/
void openthings_write_record( struct openthings_messge_context *const context,
                              struct openthings_message_record *const record )
{
    memcpy( context->openthings_message_buffer + context->eom, record,
            record->description.len );

    context->eom += record->description.len;
}
/*-----------------------------------------------------------*/
bool openthings_read_message( struct openthings_messge_context *const context,
                              const struct openthings_message_record *record )
{
    if ( context->eom != 0 ) {
        record = (const struct openthings_message_record *)&context
                     ->openthings_message_buffer +
                 context->eom;

        context->eom += record->description.len;
        return true;
    }

    return false;
}
/*-----------------------------------------------------------*/
void openthings_close_message( struct openthings_messge_context *const context )
{
    struct openthings_message_header *header =
        (struct openthings_message_header *)context->openthings_message_buffer;

    struct openthings_message_footer *footer =
        (struct openthings_message_footer *)context->openthings_message_buffer +
        context->eom;

    footer->eod = 0;
    footer->crc = crc(
        &context->openthings_message_buffer[OPENTHINGS_CRC_START],
        context->eom - OPENTHINGS_CRC_START );

    header->hdr_len = context->eom - OPENTHINGS_CRC_START;
}
/*-----------------------------------------------------------*/
void openthings_reset_message( struct openthings_messge_context *const context )
{
    context->eom = OPENTHINGS_CRC_START;
}
/*-----------------------------------------------------------*/
bool openthings_open_message( struct openthings_messge_context *const context )
{
    bool result = false;

    struct openthings_message_header *header =
        (struct openthings_message_header *)context->openthings_message_buffer;

    struct openthings_message_footer *footer =
        (struct openthings_message_footer *)context->openthings_message_buffer +
        header->hdr_len - sizeof( struct openthings_message_footer );

    int16_t calculated_crc = crc(
        ( const uint8_t *const ) &
            context->openthings_message_buffer + OPENTHINGS_CRC_START,
        header->hdr_len - OPENTHINGS_CRC_START );

    if ( footer->eod == 0 && footer->crc == calculated_crc ) {
        context->eom = sizeof( struct openthings_message_header );
    }

    return result;
}
/*-----------------------------------------------------------*/
static int16_t crc( const uint8_t const *buf, size_t size )
{
    uint16_t rem = 0;
    size_t byte, bit;
    for ( byte = 0; byte < size; ++byte ) {
        rem ^= ( buf[byte] << 8 );
        for ( bit = 8; bit > 0; --bit ) {
            rem = ( ( rem & ( 1 << 15 ) ) ? ( ( rem << 1 ) ^ 0x1021 )
                                          : ( rem << 1 ) );
        }
    }
    return rem;
}
/*-----------------------------------------------------------*/