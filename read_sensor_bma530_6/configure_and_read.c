#include <stdio.h>
#include <stdlib.h>
#include "coines.h"

#define ACC_CONF_0         0x30
#define ACC_CONF_1         0x31
#define ACC_CONF_2         0x32
#define SENSOR_STATUS      0x11
#define ACC_DATA_0         0x18
#define TEMP_DATA          0x1E
#define SENSOR_TIME_0      0x1F

uint8_t sensor_addresses[] = {0x21, 0x22, 0x23, 0x24};

void read_header(uint8_t addr)
{
    uint8_t id_bytes[6] = {0};
    uint8_t dummy = 0;
    coines_read_i2c(COINES_I2C_BUS_0, addr, 0x00, &dummy, 1);
    coines_read_i2c(COINES_I2C_BUS_0, addr, 0x00, id_bytes, 6);
    printf("HEAD;0x%02X;", addr);
    for (int i = 0; i < 6; i++) {
        printf("0x%02X;", id_bytes[i]);
    }
    printf("\n");
}

void configure_sensor(uint8_t addr)
{
    uint8_t acc_conf_1 = 0xA6;  // HPM
    uint8_t acc_conf_2 = 0x03;

    // Disable accelerometer
    uint8_t disable = 0x00;
    coines_write_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_0, &disable, 1);

    // Set configuration
    coines_write_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_1, &acc_conf_1, 1);
    coines_write_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_2, &acc_conf_2, 1);

    // Enable accelerometer
    uint8_t enable = 0x0F;
    coines_delay_msec(50);
    coines_write_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_0, &enable, 1);

    // Read back config
    uint8_t read_conf0, read_conf1, read_conf2;
    coines_read_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_0, &read_conf0, 1);
    coines_read_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_1, &read_conf1, 1);
    coines_read_i2c(COINES_I2C_BUS_0, addr, ACC_CONF_2, &read_conf2, 1);

    printf("Sensor 0x%02X Config: ACC_CONF_0=0x%02X; ACC_CONF_1=0x%02X; ACC_CONF_2=0x%02X\n",
           addr, read_conf0, read_conf1, read_conf2);
}

void read_sensor_data(uint8_t addr)
{
    uint8_t sensor_status = 0;
    uint8_t acc_data[12] = {0};

    int retry = 10;
    do {
        coines_read_i2c(COINES_I2C_BUS_0, addr, SENSOR_STATUS, &sensor_status, 1);
        coines_delay_msec(10);
    } while ((sensor_status & 0x01) == 0 && --retry);

    if ((sensor_status & 0x01) == 0)
    {
        printf("0x%02X;DataNotReady;\n", addr);
        return;
    }

    coines_read_i2c(COINES_I2C_BUS_0, addr, ACC_DATA_0, acc_data, 12);

    int16_t raw_x = (acc_data[1] << 8) | acc_data[0];
    int16_t raw_y = (acc_data[3] << 8) | acc_data[2];
    int16_t raw_z = (acc_data[5] << 8) | acc_data[4];
    uint8_t temp_raw = acc_data[6];
    uint32_t sensor_time = (acc_data[9] << 16) | (acc_data[8] << 8) | acc_data[7];

    float scale = 16.0f / 32768.0f;
    float x_g = raw_x * scale;
    float y_g = raw_y * scale;
    float z_g = raw_z * scale;
    float temp_c = temp_raw * 0.5f + 23.0f;
   
    // Print data
    printf("0x%02X;%.2f;%.2f;%.2f;%.1f;%lu\n", addr, x_g, y_g, z_g, temp_c, sensor_time);
}

int main(void)
{
    if (coines_open_comm_intf(COINES_COMM_INTF_USB, NULL) != COINES_SUCCESS)
    {
        printf("Failed to open COINES interface.\n");
        return -1;
    }

    coines_set_shuttleboard_vdd_vddio_config(3300, 3300);
    coines_delay_msec(3);
    coines_config_i2c_bus(COINES_I2C_BUS_0, COINES_I2C_FAST_MODE);

    int count = sizeof(sensor_addresses) / sizeof(sensor_addresses[0]);

    // Configure once and print configuration
    for (int i = 0; i < count; i++)
    {   
        read_header(sensor_addresses[i]);
        configure_sensor(sensor_addresses[i]);
    }

    printf("\nSensorAddr;X_g;Y_g;Z_g;Temp_C;SensorTime\n");

    while (1)
    {
        for (int i = 0; i < count; i++)
        {
            read_sensor_data(sensor_addresses[i]);
        }
        coines_delay_msec(10); 
    }

    coines_set_shuttleboard_vdd_vddio_config(0, 0);
    coines_soft_reset();
    coines_close_comm_intf(COINES_COMM_INTF_USB, NULL);

    return 0;
}
