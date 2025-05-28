#include <stdio.h>
#include <stdlib.h>
#include "coines.h"

#define REG_CHIP_ID 0x00

int main(void)
{
    int8_t result;
    uint8_t chip_id;
    enum coines_comm_intf intf = COINES_COMM_INTF_USB;

    // Open USB connection
    result = coines_open_comm_intf(intf, NULL);
    if (result != COINES_SUCCESS)
    {
        printf("Unable to connect to COINES board (Error: %d)\n", result);
        return result;
    }

    // Power up the board
    coines_set_shuttleboard_vdd_vddio_config(3300, 3300);
    coines_delay_msec(3);

    // Setup I2C bus
    coines_config_i2c_bus(COINES_I2C_BUS_0, COINES_I2C_STANDARD_MODE);

    // Scan from 0x20 to 0x30 
    for (uint8_t addr = 0x20; addr <= 0x30; addr++)
    {
        chip_id = 0;
        printf("\n Reading CHIP_ID from I2C address 0x%02X...\n", addr);

        // Dummy transaction 
        uint8_t dummy = 0;
        coines_read_i2c(COINES_I2C_BUS_0, addr, REG_CHIP_ID, &dummy, 1);
        coines_delay_msec(1);

        // Actual read
        result = coines_read_i2c(COINES_I2C_BUS_0, addr, REG_CHIP_ID, &chip_id, 1);
        if (result == COINES_SUCCESS)
        {
            printf("CHIP_ID at 0x%02X: 0x%02X\n", addr, chip_id);
        }
        else
        {
            printf("No response at 0x%02X\n", addr);
        }
    }

    // Power down and cleanup
    coines_set_shuttleboard_vdd_vddio_config(0, 0);
    coines_soft_reset();
    coines_close_comm_intf(intf, NULL);
    return 0;
}
