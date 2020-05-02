#pragma once

//
// Device type           -- in the "User Defined" range."
//
#define NON_PNP_TYPE 40001
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//
#define IOCTL_NONPNP_GET_MESSAGE \
    CTL_CODE( NON_PNP_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define NON_PNP_DRIVER_FILE_NAME    "\\\\.\\NonPnp"