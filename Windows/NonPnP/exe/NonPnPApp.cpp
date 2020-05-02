#include <Windows.h>
#include <iostream>
#include "..\sys\public.h"

int main()
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    CHAR InputBuffer[500] = { 0 };
    CHAR OutputBuffer[500] = { 0 };
    ULONG BytesReturned = 0;

    handle = CreateFileA(
        NON_PNP_DRIVER_FILE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Unable to open handle to " << NON_PNP_DRIVER_FILE_NAME << std::endl;
        return 1;
    }

    std::cout << "Enter your name: ";
    std::cin >> InputBuffer;

    bool result = DeviceIoControl(
        handle,
        IOCTL_NONPNP_GET_MESSAGE,
        InputBuffer,
        sizeof(InputBuffer),
        OutputBuffer,
        sizeof(OutputBuffer),
        &BytesReturned,
        NULL);
    if (!result)
    {
        std::cout << "Error in DeviceIoControl " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Message from driver: " << OutputBuffer << std::endl;

    //
    // Close the handle to the device before unloading the driver.
    //
    CloseHandle(handle);
}
