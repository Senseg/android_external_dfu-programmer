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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <usb.h>

#include "config.h"
#include "dfu.h"
#include "atmel.h"
#include "arguments.h"
#include "commands.h"


/*
 *  device_init is designed to find one of the usb devices which match
 *  the vendor and product parameters passed in.
 *
 *  vendor  - the vender number of the device to look for
 *  product - the product number of the device to look for
 *
 *  return a pointer to the usb_device if found, or NULL otherwise
 */
static struct usb_device *device_init( unsigned int vendor, unsigned int product )
{
    struct usb_bus *usb_bus;
    struct usb_device *dev;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    /* Walk the tree and find our device. */
    for( usb_bus = usb_get_busses(); NULL != usb_bus; usb_bus = usb_bus->next ) {
        for( dev = usb_bus->devices; NULL != dev; dev = dev->next) {
            if(    (vendor  == dev->descriptor.idVendor)
                && (product == dev->descriptor.idProduct) )
            {
                return dev;
            }
        }
    }

    return NULL;
}

int debug;

int main( int argc, char **argv )
{
    static const char *progname = PACKAGE;
    int retval = 0;
    struct usb_device *device = NULL;
    struct usb_dev_handle *usb_handle = NULL;
    struct dfu_status status;
    struct programmer_arguments args;
    int interface = 0;

    memset( &args, 0, sizeof(args) );
    if( 0 != parse_arguments(&args, argc, argv) ) {
        retval = 1;
        goto exit_0;
    }

    if( args.command == com_version ) {
        printf( PACKAGE_STRING "\n" );
        return 0;
    }

    if( debug >= 200 ) {
        usb_set_debug( debug );
    }

    device = device_init( args.vendor_id, args.chip_id );

    if( NULL == device ) {
        fprintf( stderr, "%s: no device present.\n", progname );
        retval = 1;
        goto exit_0;
    }

    usb_handle = usb_open( device );

    if( NULL == usb_handle ) {
        fprintf( stderr, "%s: device failed to open.\n", progname );
        retval = 1;
        goto exit_1;
    }

    if( 0 != usb_claim_interface(usb_handle, interface) ) {
        fprintf( stderr, "%s: failed to claim interface.  "
                         "Check permissions.\n", progname );
        retval = 1;
        goto exit_1;
    }

    dfu_get_status( usb_handle, interface, &status );

    if( DFU_STATUS_OK != status.bStatus ) {
        /* Clear our status & try again. */
        dfu_clear_status( usb_handle, interface );

        dfu_get_status( usb_handle, interface, &status );

        if( DFU_STATUS_OK != status.bStatus ) {
            fprintf( stderr, "%s: error: %d\n", progname, status.bStatus );
            retval = 1;
            goto exit_2;
        }
    }


    if( 0 != execute_command(usb_handle, interface, args) ) {
        /* command issued a specific diagnostic already */
        retval = 1;
        goto exit_2;
    }

    retval = 0;

exit_2:
    if( 0 != usb_release_interface(usb_handle, interface) ) {
        fprintf( stderr, "%s: failed to release interface %d.\n",
                         progname, interface );
        retval = 1;
    }

exit_1:
    if( 0 != usb_close(usb_handle) ) {
        fprintf( stderr, "%s: failed to close the handle.\n", progname );
        retval = 1;
    }

exit_0:
    return retval;
}
