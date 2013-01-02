#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <psflib.h>

/* Meh, the name collides with Qt crap */
#include "../QSoundCore/Core/qsound.h"
#include "../QSoundCore/Core/qmix.h"

inline unsigned get_be16( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 8 |
            (unsigned) ((unsigned char const*) p) [1];
}

inline unsigned get_be32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 24 |
            (unsigned) ((unsigned char const*) p) [1] << 16 |
            (unsigned) ((unsigned char const*) p) [2] <<  8 |
            (unsigned) ((unsigned char const*) p) [3];
}

inline unsigned get_le32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [3] << 24 |
            (unsigned) ((unsigned char const*) p) [2] << 16 |
            (unsigned) ((unsigned char const*) p) [1] <<  8 |
            (unsigned) ((unsigned char const*) p) [0];
}

inline void set_le32( void* p, unsigned n )
{
    ((unsigned char*) p) [0] = (unsigned char) n;
    ((unsigned char*) p) [1] = (unsigned char) (n >> 8);
    ((unsigned char*) p) [2] = (unsigned char) (n >> 16);
    ((unsigned char*) p) [3] = (unsigned char) (n >> 24);
}

struct qsf_loader_state
{
    uint8_t * key;
    uint32_t key_size;

    uint8_t * z80_rom;
    uint32_t z80_size;

    uint8_t * sample_rom;
    uint32_t sample_size;
};

static int upload_section( struct qsf_loader_state * state, const char * section, uint32_t start,
                           const uint8_t * data, uint32_t size )
{
    uint8_t ** array = NULL;
    uint32_t * array_size = NULL;
    uint32_t max_size = 0x7fffffff;

    if ( !strcmp( section, "KEY" ) ) { array = &state->key; array_size = &state->key_size; max_size = 11; }
    else if ( !strcmp( section, "Z80" ) ) { array = &state->z80_rom; array_size = &state->z80_size; }
    else if ( !strcmp( section, "SMP" ) ) { array = &state->sample_rom; array_size = &state->sample_size; }
    else return -1;

    if ( ( start + size ) < start ) return -1;

    uint32_t new_size = start + size;
    uint32_t old_size = *array_size;
    if ( new_size > max_size ) return -1;

    if ( new_size > old_size ) {
        *array = ( uint8_t * ) realloc( *array, new_size );
        *array_size = new_size;
        memset( (*array) + old_size, 0, new_size - old_size );
    }

    memcpy( (*array) + start, data, size );

    return 0;
}

static int qsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    struct qsf_loader_state * state = ( struct qsf_loader_state * ) context;

    for (;;) {
        char s[4];
        if ( exe_size < 11 ) break;
        memcpy( s, exe, 3 ); exe += 3; exe_size -= 3;
        s [3] = 0;
        uint32_t dataofs  = get_le32( exe ); exe += 4; exe_size -= 4;
        uint32_t datasize = get_le32( exe ); exe += 4; exe_size -= 4;
        if ( datasize > exe_size )
            return -1;

        if ( upload_section( state, s, dataofs, exe, datasize ) < 0 )
            return -1;

        exe += datasize;
        exe_size -= datasize;
    }

    return 0;
}

static void * stdio_fopen( const char * path )
{
    return fopen( path, "rb" );
}

static size_t stdio_fread( void *p, size_t size, size_t count, void *f )
{
    return fread( p, size, count, (FILE*) f );
}

static int stdio_fseek( void * f, int64_t offset, int whence )
{
    return fseek( (FILE*) f, offset, whence );
}

static int stdio_fclose( void * f )
{
    return fclose( (FILE*) f );
}

static long stdio_ftell( void * f )
{
    return ftell( (FILE*) f );
}

static const psf_file_callbacks stdio_callbacks =
{
    "\\/:",
    stdio_fopen,
    stdio_fread,
    stdio_fseek,
    stdio_fclose,
    stdio_ftell
};

enum {
    cmd_gg_stereo       = 0x4F,
    cmd_gg_stereo_2     = 0x3F,
    cmd_psg             = 0x50,
    cmd_psg_2           = 0x30,
    cmd_ym2413          = 0x51,
    cmd_ym2413_2        = 0xA1,
    cmd_ym2612_port0    = 0x52,
    cmd_ym2612_2_port0  = 0xA2,
    cmd_ym2612_port1    = 0x53,
    cmd_ym2612_2_port1  = 0xA3,
    cmd_ym2151          = 0x54,
    cmd_ym2151_2        = 0xA4,
    cmd_ym2203          = 0x55,
    cmd_ym2203_2        = 0xA5,
    cmd_ym2608_port0    = 0x56,
    cmd_ym2608_2_port0  = 0xA6,
    cmd_ym2608_port1    = 0x57,
    cmd_ym2608_2_port1  = 0xA7,
    cmd_ym2610_port0    = 0x58,
    cmd_ym2610_2_port0  = 0xA8,
    cmd_ym2610_port1    = 0x59,
    cmd_ym2610_2_port1  = 0xA9,
    cmd_ym3812          = 0x5A,
    cmd_ym3812_2        = 0xAA,
    cmd_ymz280b         = 0x5D,
    cmd_ymf262_port0    = 0x5E,
    cmd_ymf262_2_port0  = 0xAE,
    cmd_ymf262_port1    = 0x5F,
    cmd_ymf262_2_port1  = 0xAF,
    cmd_delay           = 0x61,
    cmd_delay_735       = 0x62,
    cmd_delay_882       = 0x63,
    cmd_byte_delay      = 0x64,
    cmd_end             = 0x66,
    cmd_data_block      = 0x67,
    cmd_ram_block       = 0x68,
    cmd_short_delay     = 0x70,
    cmd_pcm_delay       = 0x80,
    cmd_dacctl_setup    = 0x90,
    cmd_dacctl_data     = 0x91,
    cmd_dacctl_freq     = 0x92,
    cmd_dacctl_play     = 0x93,
    cmd_dacctl_stop     = 0x94,
    cmd_dacctl_playblock= 0x95,
    cmd_ay8910          = 0xA0,
    cmd_rf5c68          = 0xB0,
    cmd_rf5c164         = 0xB1,
    cmd_pwm             = 0xB2,
    cmd_okim6258_write  = 0xB7,
    cmd_okim6295_write  = 0xB8,
    cmd_huc6280_write   = 0xB9,
    cmd_k053260_write   = 0xBA,
    cmd_segapcm_write   = 0xC0,
    cmd_rf5c68_mem      = 0xC1,
    cmd_rf5c164_mem     = 0xC2,
    cmd_qsound_write    = 0xC4,
    cmd_k051649_write   = 0xD2,
    cmd_k054539_write   = 0xD3,
    cmd_c140            = 0xD4,
    cmd_pcm_seek        = 0xE0,

    rf5c68_ram_block    = 0x01,
    rf5c164_ram_block   = 0x02,

    pcm_block_type      = 0x00,
    pcm_aux_block_type  = 0x40,
    rom_block_type      = 0x80,
    ram_block_type      = 0xC0,

    rom_segapcm         = 0x80,
    rom_ym2608_deltat   = 0x81,
    rom_ym2610_adpcm    = 0x82,
    rom_ym2610_deltat   = 0x83,
    rom_ymz280b         = 0x86,
    rom_okim6295        = 0x8B,
    rom_k054539         = 0x8C,
    rom_c140            = 0x8D,
    rom_k053260         = 0x8E,
    rom_qsound          = 0x8F,

    ram_rf5c68          = 0xC0,
    ram_rf5c164         = 0xC1,
    ram_nesapu          = 0xC2,

    ym2612_dac_port     = 0x2A,
    ym2612_dac_pan_port = 0xB6
};

enum { vgm_header_size_min = 0x40 };
enum { vgm_header_size_151 = 0x80 };
enum { vgm_header_size_max = 0xC0 };

typedef struct vgm_header_t
{
    char tag               [4]; // 0x00
    uint8_t data_size      [4]; // 0x04
    uint8_t version        [4]; // 0x08
    uint8_t psg_rate       [4]; // 0x0C
    uint8_t ym2413_rate    [4]; // 0x10
    uint8_t gd3_offset     [4]; // 0x14
    uint8_t track_duration [4]; // 0x18
    uint8_t loop_offset    [4]; // 0x1C
    uint8_t loop_duration  [4]; // 0x20
    uint8_t frame_rate     [4]; // 0x24 v1.01 V
    uint8_t noise_feedback [2]; // 0x28 v1.10 V
    uint8_t noise_width;        // 0x2A
    uint8_t sn76489_flags;      // 0x2B v1.51 <
    uint8_t ym2612_rate    [4]; // 0x2C v1.10 V
    uint8_t ym2151_rate    [4]; // 0x30
    uint8_t data_offset    [4]; // 0x34 v1.50 V
    uint8_t segapcm_rate   [4]; // 0x38 v1.51 V
    uint8_t segapcm_reg    [4]; // 0x3C
    uint8_t rf5c68_rate    [4]; // 0x40
    uint8_t ym2203_rate    [4]; // 0x44
    uint8_t ym2608_rate    [4]; // 0x48
    uint8_t ym2610_rate    [4]; // 0x4C
    uint8_t ym3812_rate    [4]; // 0x50
    uint8_t ym3526_rate    [4]; // 0x54
    uint8_t y8950_rate     [4]; // 0x58
    uint8_t ymf262_rate    [4]; // 0x5C
    uint8_t ymf278b_rate   [4]; // 0x60
    uint8_t ymf271_rate    [4]; // 0x64
    uint8_t ymz280b_rate   [4]; // 0x68
    uint8_t rf5c164_rate   [4]; // 0x6C
    uint8_t pwm_rate       [4]; // 0x70
    uint8_t ay8910_rate    [4]; // 0x74
    uint8_t ay8910_type;        // 0x78
    uint8_t ay8910_flags;       // 0x79
    uint8_t ym2203_ay8910_flags;// 0x7A
    uint8_t ym2608_ay8910_flags;// 0x7B
    uint8_t volume_modifier;    // 0x7C v1.60 V
    uint8_t reserved;           // 0x7D
    uint8_t loop_base;          // 0x7E
    uint8_t loop_modifier;      // 0x7F v1.51 <
    uint8_t gbdmg_rate     [4]; // 0x80 v1.61 V
    uint8_t nesapu_rate    [4]; // 0x84
    uint8_t multipcm_rate  [4]; // 0x88
    uint8_t upd7759_rate   [4]; // 0x8C
    uint8_t okim6258_rate  [4]; // 0x90
    uint8_t okim6258_flags;     // 0x94
    uint8_t k054539_flags;      // 0x95
    uint8_t c140_type;          // 0x96
    uint8_t reserved_flags;     // 0x97
    uint8_t okim6295_rate  [4]; // 0x98
    uint8_t k051649_rate   [4]; // 0x9C
    uint8_t k054539_rate   [4]; // 0xA0
    uint8_t huc6280_rate   [4]; // 0xA4
    uint8_t c140_rate      [4]; // 0xA8
    uint8_t k053260_rate   [4]; // 0xAC
    uint8_t pokey_rate     [4]; // 0xB0
    uint8_t qsound_rate    [4]; // 0xB4
    uint8_t reserved2      [4]; // 0xB8
    uint8_t extra_offset   [4]; // 0xBC
} vgm_header_t;

struct qsf_command
{
    uint32_t timestamp;
    uint8_t command;
    uint16_t data;
};

struct qsf_command_list
{
    unsigned int count;
    unsigned int count_allocated;

    uint32_t timestamp;

    struct qsf_command *commands;
};

void EMU_CALL qsf_command_advance(void *context, uint32 samples)
{
    struct qsf_command_list * list = ( struct qsf_command_list * ) context;
    list->timestamp += samples;
}

void EMU_CALL qsf_command_add(void *context, uint8 cmd, uint16 data)
{
    struct qsf_command_list * list = ( struct qsf_command_list * ) context;
    struct qsf_command * command;
    unsigned int new_count = list->count + 1;
    if ( new_count > list->count_allocated )
    {
        list->count_allocated += 32768;
        list->commands = ( struct qsf_command * ) realloc( list->commands, list->count_allocated * sizeof( struct qsf_command ) );
    }
    command = list->commands + list->count;
    ++ list->count;

    command->timestamp = list->timestamp;
    command->command = cmd;
    command->data = data;
}

struct qsf_sample
{
    uint32_t address;
    uint32_t size;
};

struct qsf_sample_list
{
    unsigned int count;

    struct qsf_sample *samples;
};

void EMU_CALL qsf_sample_add(void *context, uint32 offset, uint32 size)
{
    struct qsf_sample_list * list = ( struct qsf_sample_list * ) context;
    struct qsf_sample * sample;
    unsigned int i;
    for ( i = 0; i < list->count; ++i )
    {
        sample = list->samples + i;
        if ( sample->address == offset && sample->size == size )
            return;
    }
    list->samples = ( struct qsf_sample * ) realloc( list->samples, ( list->count + 1 ) * sizeof( struct qsf_sample ) );
    sample = list->samples + list->count;
    ++ list->count;
    sample->address = offset;
    sample->size = size;
}

int qsf_sample_compare(const void *a, const void *b)
{
    struct qsf_sample * aa = ( struct qsf_sample * ) a;
    struct qsf_sample * bb = ( struct qsf_sample * ) b;

    return (int)(aa->address) - (int)(bb->address);
}

int main(int argc, char const* const* argv)
{
    int rval = 1;
    struct vgm_header_t header;
    struct qsf_loader_state state;
    struct qsf_command_list cmd_list;
    struct qsf_sample_list sample_list;
    void * qsound;
    void * qmix;
    FILE * f;
    uint32_t howmany;
    uint32_t rendered;
    uint8_t command_buffer[16];

    if ( argc != 3 )
    {
        fprintf( stderr, "Syntax:\t%s <in.qsf> <out.vgm>\n", argv[0] );
        return 1;
    }

    memset( &header, 0, sizeof(header) );
    memset( &state, 0, sizeof(state) );
    memset( &cmd_list, 0, sizeof(cmd_list) );
    memset( &sample_list, 0, sizeof(sample_list) );
    qsound = NULL;
    f = NULL;

    if ( psf_load( argv[ 1 ], &stdio_callbacks, 0x41, qsf_loader, &state, 0, 0 ) < 0 )
    {
        fprintf( stderr, "Invalid QSF file: %s\n", argv[ 1 ] );
        goto cleanup;
    }

    qsound_init();

    qsound = malloc( qsound_get_state_size() );
    if ( !qsound )
    {
        fprintf( stderr, "Out of memory\n" );
        goto cleanup;
    }

    qsound_clear_state( qsound );

    if(state.key_size == 11) {
        uint8_t * ptr = state.key;
        uint32_t swap_key1 = get_be32( ptr +  0 );
        uint32_t swap_key2 = get_be32( ptr +  4 );
        uint32_t addr_key  = get_be16( ptr +  8 );
        uint8_t  xor_key   =        *( ptr + 10 );
        qsound_set_kabuki_key( qsound, swap_key1, swap_key2, addr_key, xor_key );
    } else {
        qsound_set_kabuki_key( qsound, 0, 0, 0, 0 );
    }
    qsound_set_z80_rom( qsound, state.z80_rom, state.z80_size );
    qsound_set_sample_rom( qsound, state.sample_rom, state.sample_size );

    qmix = qsound_get_qmix_state( qsound );

    qmix_set_advance_callback( qmix, qsf_command_advance, &cmd_list );
    qmix_set_command_callback( qmix, qsf_command_add, &cmd_list );

    qmix_set_sample_usage_callback( qmix, qsf_sample_add, &sample_list );

    rendered = 0;
    while ( rendered < 44100 * 60 * 10 )
    {
        howmany = 2048;
        qsound_execute( qsound, 0x7fffffff, NULL, &howmany );
        if ( howmany < 2048 ) break;
        rendered += howmany;
    }

    free( qsound );
    qsound = NULL;

    fprintf( stderr, "Samples used: %u\n", sample_list.count );
    fprintf( stderr, "Commands used: %u\n", cmd_list.count );

    qsort( sample_list.samples, sample_list.count, sizeof(*sample_list.samples), qsf_sample_compare );

    memcpy( &header.tag, "Vgm ", 4 );
    set_le32( &header.version, 0x161 );
    set_le32( &header.qsound_rate, 4000000 );
    set_le32( &header.data_offset, sizeof( header ) - offsetof( vgm_header_t, data_offset ) );
    set_le32( &header.track_duration, 44100 * 60 * 10 );

    f = fopen( argv[ 2 ], "wb" );
    if ( !f )
    {
        fprintf( stderr, "Error opening output file: %s\n", argv[ 2 ] );
        goto cleanup;
    }

    fwrite( &header, 1, sizeof( header ), f );

    command_buffer[ 0 ] = cmd_data_block;
    command_buffer[ 1 ] = cmd_end;
    command_buffer[ 2 ] = rom_qsound;
    set_le32( command_buffer + 7, state.sample_size );

    for ( rendered = 0; rendered < sample_list.count; ++rendered )
    {
        struct qsf_sample * sample = sample_list.samples + rendered;

        set_le32( command_buffer + 3, sample->size + 8 );
        set_le32( command_buffer + 11, sample->address );

        fwrite( command_buffer, 1, 15, f );
        fwrite( state.sample_rom + sample->address, 1, sample->size, f );
    }

    for ( rendered = 0, howmany = 0; rendered < cmd_list.count; ++rendered )
    {
        struct qsf_command * command = cmd_list.commands + rendered;
        uint32_t delta = command->timestamp - howmany;
        howmany = command->timestamp;
        while (delta >= 256)
        {
            command_buffer[ 0 ] = cmd_delay;
            command_buffer[ 1 ] = (uint8_t)( delta );
            command_buffer[ 2 ] = (uint8_t)( delta >> 8 );
            fwrite( command_buffer, 1, 3, f );
            delta -= (uint16_t)delta;
        }
        while (delta)
        {
            if ( delta > 16 )
            {
                uint32_t delta_partial = delta;
                if ( delta_partial > 255 )
                    delta_partial = 255;
                delta -= delta_partial;
                command_buffer[ 0 ] = cmd_byte_delay;
                command_buffer[ 1 ] = (uint8_t) delta_partial;
                fwrite( command_buffer, 1, 2, f );
            }
            else
            {
                command_buffer[ 0 ] = cmd_short_delay + delta - 1;
                fwrite( command_buffer, 1, 1, f );
                break;
            }
        }
        command_buffer[ 0 ] = cmd_qsound_write;
        command_buffer[ 1 ] = (uint8_t)(command->data >> 8);
        command_buffer[ 2 ] = (uint8_t)(command->data);
        command_buffer[ 3 ] = (uint8_t)(command->command);
        fwrite( command_buffer, 1, 4, f );
    }

    command_buffer[ 0 ] = cmd_end;
    fwrite( command_buffer, 1, 1, f );

    rendered = ftell( f );
    set_le32( &header.data_size, rendered - offsetof(vgm_header_t, data_size) );

    fseek( f, 0, SEEK_SET );
    fwrite( &header, 1, sizeof(header), f );

    fprintf( stderr, "Done.\n" );

    rval = 0;

cleanup:
    if ( f ) fclose( f );

    if ( qsound ) free( qsound );

    if ( sample_list.samples ) free( sample_list.samples );

    if ( cmd_list.commands ) free( cmd_list.commands );

    if ( state.key ) free( state.key );
    if ( state.z80_rom ) free( state.z80_rom );
    if ( state.sample_rom ) free( state.sample_rom );

    return rval;
}
