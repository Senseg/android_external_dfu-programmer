/*
 * dfu-programmer
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dfu-bool.h"
#include "config.h"
#include "arguments.h"

struct option_mapping_structure {
    const char *name;
    int32_t value;
};

struct target_mapping_structure {
    const char *name;
    enum targets_enum value;
    enum device_type_enum device_type;
    uint16_t chip_id;
    uint16_t vendor_id;
    size_t memory_size;
    size_t flash_page_size;
    dfu_bool initial_abort;
    dfu_bool honor_interfaceclass;
    size_t eeprom_page_size;
    size_t eeprom_memory_size;
};

/* NOTE FOR: at90usb1287, at90usb1286, at90usb647, at90usb646, at90usb162, at90usb82
 *
 * The actual size of the user-programmable section is limited by the
 * space needed by the bootloader.  The size of the bootloader is set
 * by BOOTSZ0/BOOTSZ1 fuse bits; here we assume the bootloader is 4kb or 8kb.
 * The window used for the bootloader is at the top of the of memory.
 */

/* ----- target specific structures ----------------------------------------- */
static struct target_mapping_structure target_map[] = {
    { "at89c51snd1c", tar_at89c51snd1c, device_8051, 0x2FFF, 0x03eb, 0x10000, 128, false, true,  0,   0      },
    { "at89c5130",    tar_at89c5130,    device_8051, 0x2FFD, 0x03eb, 0x04000, 128, false, true,  128, 0x03FF },
    { "at89c5131",    tar_at89c5131,    device_8051, 0x2FFD, 0x03eb, 0x08000, 128, false, true,  128, 0x03FF },
    { "at89c5132",    tar_at89c5132,    device_8051, 0x2FFF, 0x03eb, 0x10000, 128, false, true,  0,   0      },
    { "at90usb1287",  tar_at90usb1287,  device_AVR,  0x2FFB, 0x03eb, 0x1F000, 128, true,  false, 128, 0x0FFF },
    { "at90usb1286",  tar_at90usb1286,  device_AVR,  0x2FFB, 0x03eb, 0x1F000, 128, true,  false, 128, 0x0FFF },
    { "at90usb647",   tar_at90usb647,   device_AVR,  0x2FFB, 0x03eb, 0x0F000, 128, true,  false, 128, 0x07FF },
    { "at90usb646",   tar_at90usb646,   device_AVR,  0x2FFB, 0x03eb, 0x0F000, 128, true,  false, 128, 0x07FF },
    { "at90usb162",   tar_at90usb162,   device_AVR,  0x2FFA, 0x03eb, 0x03000, 128, true,  false, 128, 0x01FF },
    { "at90usb82",    tar_at90usb82,    device_AVR,  0x2FFA, 0x03eb, 0x01000, 128, true,  false, 128, 0x01FF },
    { NULL }
};

/* ----- command specific structures ---------------------------------------- */
static struct option_mapping_structure command_map[] = {
    { "configure",    com_configure },
    { "dump",         com_dump      },
    { "dump-eeprom",  com_edump     },
    { "erase",        com_erase     },
    { "flash",        com_flash     },
    { "flash-eeprom", com_eflash    },
    { "get",          com_get       },
    { "reset",        com_reset     },
    { "start",        com_start_app },
    { "version",      com_version   },
    { NULL }
};

/* ----- configure specific structures -------------------------------------- */
static struct option_mapping_structure configure_map[] = {
    { "BSB", conf_BSB },
    { "SBV", conf_SBV },
    { "SSB", conf_SSB },
    { "EB",  conf_EB  },
    { "HSB", conf_HSB },
    { NULL }
};

/* ----- get specific structures -------------------------------------- */
static struct option_mapping_structure get_map[] = {
    { "bootloader-version", get_bootloader   },
    { "ID1",                get_ID1          },
    { "ID2",                get_ID2          },
    { "BSB",                get_BSB          },
    { "SBV",                get_SBV          },
    { "SSB",                get_SSB          },
    { "EB",                 get_EB           },
    { "manufacturer",       get_manufacturer },
    { "family",             get_family       },
    { "product-name",       get_product_name },
    { "product-revision",   get_product_rev  },
    { "HSB",                get_HSB          },
    { NULL }
};


static void usage()
{
    struct target_mapping_structure *map = NULL;

    map = target_map;

    fprintf( stderr, PACKAGE_STRING "\n");
    fprintf( stderr, "Usage: dfu-programmer target command [command-options] "
                     "[global-options] [file|data]\n" );
    fprintf( stderr, "targets:\n" );
    while( 0 != *((int32_t *) map) ) {
        fprintf( stderr, "        %s\n", map->name );
        map++;
    }
    fprintf( stderr, "global-options: --quiet, --debug level\n" );
    fprintf( stderr, "commands:\n" );
    fprintf( stderr, "        configure {BSB|SBV|SSB|EB|HSB} "
                     "[--suppress-validation] [global-options] data\n" );
    fprintf( stderr, "        dump "
                     "[global-options]\n" );
    fprintf( stderr, "        dump-eeprom "
                     "[global-options]\n" );
    fprintf( stderr, "        erase "
                     "[--suppress-validation] [global-options]\n" );
    fprintf( stderr, "        flash "
                     "[--suppress-validation] [global-options] file\n" );
    fprintf( stderr, "        flash-eeprom "
                     "[--suppress-validation] [global-options] file\n" );
    fprintf( stderr, "        get {bootloader-version|ID1|ID2|BSB|SBV|SSB|EB|\n"
                     "            manufacturer|family|product-name|\n"
                     "            product-revision|HSB} "
                     "[global-options]\n" );
    fprintf( stderr, "        reset [global-options]\n" );
    fprintf( stderr, "        start [global-options]\n" );
    fprintf( stderr, "        version [global-options]\n" );
}

static int32_t assign_option( int32_t *arg,
                              char *value,
                              struct option_mapping_structure *map )
{
    while( 0 != *((int32_t *) map) ) {
        if( 0 == strcasecmp(value, map->name) ) {
            *arg = map->value;
            return 0;
        }

        map++;
    }

    return -1;
}


static int32_t assign_target( struct programmer_arguments *args,
                              char *value,
                              struct target_mapping_structure *map )
{
    while( 0 != *((int32_t *) map) ) {
        if( 0 == strcasecmp(value, map->name) ) {
            args->target  = map->value;
            args->chip_id = map->chip_id;
            args->vendor_id = map->vendor_id;
            args->device_type = map->device_type;
            args->memory_size = map->memory_size;
            args->eeprom_memory_size = map->eeprom_memory_size;
            args->flash_page_size = map->flash_page_size;
            args->eeprom_page_size = map->eeprom_page_size;
            args->initial_abort = map->initial_abort;
            args->honor_interfaceclass = map->honor_interfaceclass;
            args->top_memory_address = map->memory_size - 1;
            if( 0 < map->eeprom_memory_size ) {
                args->top_eeprom_memory_address = map->eeprom_memory_size - 1;
            } else {
                args->top_eeprom_memory_address = 0;
            }
            switch( args->device_type ) {
                case device_8051:
                    strncpy( args->device_type_string, "8051",
                             DEVICE_TYPE_STRING_MAX_LENGTH );
                    break;
                case device_AVR:
                    strncpy( args->device_type_string, "AVR",
                             DEVICE_TYPE_STRING_MAX_LENGTH );
                    break;
            }
            return 0;
        }

        map++;
    }

    return -1;
}


static int32_t assign_global_options( struct programmer_arguments *args,
                                      const size_t argc,
                                      char **argv )
{
    size_t i = 0;

    /* Find '--quiet' if it is here */
    for( i = 0; i < argc; i++ ) {
        if( 0 == strcmp("--quiet", argv[i]) ) {
            *argv[i] = '\0';
            args->quiet = 1;
            break;
        }
    }
    /* Find '--suppress-validation' if it is here - even though it is not
     * used by all this is easier. */
    for( i = 0; i < argc; i++ ) {
        if( 0 == strcmp("--suppress-validation", argv[i]) ) {
            *argv[i] = '\0';

            switch( args->command ) {
                case com_configure:
                    args->com_configure_data.suppress_validation = 1;
                    break;
                case com_erase:
                    args->com_erase_data.suppress_validation = 1;
                    break;
                case com_flash:
                case com_eflash:
                    args->com_flash_data.suppress_validation = 1;
                    break;
                default:
                    /* not supported. */
                    return -1;
            }

            break;
        }
    }


    /* Find '--debug' if it is here */
    for( i = 0; i < argc; i++ ) {
        if( 0 == strncmp("--debug", argv[i], 7) ) {

            if( 0 == strncmp("--debug=", argv[i], 8) ) {
                if( 1 != sscanf(argv[i], "--debug=%i", &debug) )
                    return -2;
            } else {
                if( (i+1) >= argc )
                    return -3;

                if( 1 != sscanf(argv[i+1], "%i", &debug) )
                    return -4;

                *argv[i+1] = '\0';
            }
            *argv[i] = '\0';
            break;
        }
    }

    return 0;
}


static int32_t assign_com_configure_option( struct programmer_arguments *args,
                                            const int32_t parameter,
                                            char *value )
{
    /* name & value */
    if( 0 == parameter ) {
        /* name */
        if( 0 != assign_option((int32_t *) &(args->com_configure_data.name),
                               value, configure_map) )
        {
            return -1;
        }
    } else {
        int32_t temp = 0;
        /* value */
        if( 1 != sscanf(value, "%i", &(temp)) )
            return -2;

        /* ensure the range is greater than 0 */
        if( temp < 0 )
            return -3;

        args->com_configure_data.value = temp;
    }

    return 0;
}


static int32_t assign_com_flash_option( struct programmer_arguments *args,
                                        const int32_t parameter,
                                        char *value )
{
    /* file */
    args->com_flash_data.original_first_char = *value;
    args->com_flash_data.file = value;

    return 0;
}


static int32_t assign_com_get_option( struct programmer_arguments *args,
                                      const int32_t parameter,
                                      char *value )
{
    /* name */
    if( 0 != assign_option((int32_t *) &(args->com_get_data.name),
                           value, get_map) )
    {
        return -1;
    }

    return 0;
}


static int32_t assign_command_options( struct programmer_arguments *args,
                                       const size_t argc,
                                       char **argv )
{
    size_t i = 0;
    int32_t param = 0;
    int32_t required_params = 0;

    /* Deal with all remaining command-specific arguments. */
    for( i = 0; i < argc; i++ ) {
        if( '\0' == *argv[i] )
            continue;

        switch( args->command ) {
            case com_configure:
                required_params = 2;
                if( 0 != assign_com_configure_option(args, param, argv[i]) )
                    return -1;
                break;

            case com_flash:
            case com_eflash:
                required_params = 1;
                if( 0 != assign_com_flash_option(args, param, argv[i]) )
                    return -3;
                break;

            case com_get:
                required_params = 1;
                if( 0 != assign_com_get_option(args, param, argv[i]) )
                    return -4;
                break;

            default:
                return -5;
        }

        *argv[i] = '\0';
        param++;
    }

    if( required_params != param )
        return -6;

    return 0;
}


static void print_args( struct programmer_arguments *args )
{
    const char *command = "(unknown)";
    const char *target = "(unknown)";
    size_t i;

    for( i = 0; i < sizeof(target_map) / sizeof(target_map[0]); i++ ) {
        if( args->target == target_map[i].value ) {
            target = target_map[i].name;
            break;
        }
    }

    for( i = 0; i < sizeof(command_map) / sizeof(command_map[0]); i++ ) {
        if( args->command == command_map[i].value ) {
            command = command_map[i].name;
            break;
        }
    }

    printf( "     target: %s\n", target );
    printf( "    chip_id: 0x%04x\n", args->chip_id );
    printf( "  vendor_id: 0x%04x\n", args->vendor_id );
    printf( "    command: %s\n", command );
    printf( "      quiet: %s\n", (0 == args->quiet) ? "false" : "true" );
    printf( "      debug: %d\n", debug );
    printf( "device_type: %s\n", args->device_type_string );
    printf( "------ command specific below ------\n" );

    switch( args->command ) {
        case com_configure:
            printf( "       name: %d\n", args->com_configure_data.name );
            printf( "   validate: %s\n",
                    (args->com_configure_data.suppress_validation) ?
                        "false" : "true" );
            printf( "      value: %d\n", args->com_configure_data.value );
            break;
        case com_erase:
            printf( "   validate: %s\n",
                    (args->com_erase_data.suppress_validation) ?
                        "false" : "true" );
            break;
        case com_flash:
        case com_eflash:
            printf( "   validate: %s\n",
                    (args->com_flash_data.suppress_validation) ?
                        "false" : "true" );
            printf( "   hex file: %s\n", args->com_flash_data.file );
            break;
        case com_get:
            printf( "       name: %d\n", args->com_get_data.name );
            break;
        default:
            break;
    }
    printf( "\n" );
    fflush( stdout );
}


int32_t parse_arguments( struct programmer_arguments *args,
                         const size_t argc,
                         char **argv )
{
    int32_t i;
    int32_t status = 0;

    if( NULL == args )
        return -1;

    /* initialize the argument block to empty, known values */
    args->target  = tar_none;
    args->command = com_none;
    args->quiet   = 0;

    /* Make sure there are the minimum arguments */
    if( argc < 3 ) {
        status = -2;
        goto done;
    }

    if( 0 != assign_target(args, argv[1], target_map) ) {
        status = -3;
        goto done;
    }

    if( 0 != assign_option((int32_t *) &(args->command), argv[2], command_map) ) {
        status = -4;
        goto done;
    }

    /* These were taken care of above. */
    *argv[0] = '\0';
    *argv[1] = '\0';
    *argv[2] = '\0';

    if( 0 != assign_global_options(args, argc, argv) ) {
        status = -5;
        goto done;
    }

    if( 0 != assign_command_options(args, argc, argv) ) {
        status = -6;
        goto done;
    }

    /* Make sure there weren't any *extra* options. */
    for( i = 0; i < argc; i++ ) {
        if( '\0' != *argv[i] ) {
            fprintf( stderr, "unrecognized parameter\n" );
            status = -7;
            goto done;
        }
    }

    /* if this is a flash command, restore the filename */
    if( (com_flash == args->command) || (com_eflash == args->command) ) {
        if( 0 == args->com_flash_data.file ) {
            fprintf( stderr, "flash filename is missing\n" );
            status = -8;
            goto done;
        }
        args->com_flash_data.file[0] = args->com_flash_data.original_first_char;
    }

done:
    if( 1 < debug ) {
        print_args( args );
    }

    if( 0 != status ) {
        usage();
    }

    return status;
}
