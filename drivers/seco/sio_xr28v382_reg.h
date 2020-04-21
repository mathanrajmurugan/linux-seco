#ifndef __SIO_XR28V382_DEVICE__
#define __SIO_XR28V382_DEVICE__

/*  GLOBAL REGISTERs  */
#define REG_GBL_SREST                      0x02     /*  Software Reset  */
#define REG_GBL_LDN                        0x07     /*  Logic Device Number Register  */
#define REG_GBL_DEV_ID_M                   0x20     /*  Device ID MSB Register  */
#define REG_GBL_DEV_ID_L                   0x21     /*  Device ID LSB Register  */
#define REG_GBL_VID_M                      0x23     /*  Vendor ID MSB Register  */
#define REG_GBL_VID_L                      0x24     /*  Vendor ID LSB Register  */
#define REG_GBL_CLKSEL                     0x25     /*  Clock Select Register */
#define REG_GBL_WDT                        0x26     /*  Watchdog Timer Control Register  */
#define REG_GBL_PSEL                       0x27     /*  Port Select Register  */


/*  UART REGISTERs  */
#define REG_UART_EN                        0x30     /*  UART Enable Register  */
#define REG_UART_BADDR_H                   0x60     /*  Base Address High Register  */
#define REG_UART_BADDR_L                   0x61     /*  Base Address Low Register  */
#define REG_UART_IRQ_CH_SEL                0x70     /*  IRQ Channel Select Register  */
#define REG_UART_ENH_MULTIFUN              0xF0     /*  Enhanced Multifunction Register  */
#define REG_UART_IRC                       0xF1     /*  IR Control Register  */
#define REG_UART_S_ADDR_MODE               0xF4     /*  9-bit Mode Slave Address Register  */
#define REG_UART_S_ADDR_MODE_MASK          0xF5     /*  9-bit Mode Slave Address Mask Regiser  */
#define REG_UART_FIFO_MOD_SEL              0xF6     /*  FIFO Mode Select Register  */


/*  WDT REGISTERs  */
#define REG_WDT_EN                         0x30     /*  Watchdog Enable Register  */
#define REG_WDT_BADDR_H                    0x60     /*  Base Address High Regiser  */
#define REG_WDT_BADDR_L                    0x61     /*  Base Address Low Register  */
#define REG_WDT_IRQ_CH_SEL                 0x70     /*  IRQ Channel Select Register  */
#define REG_WDT_T_STATUS_CTRL              0xF0     /*  Timer Status And Control Register  */
#define REG_WDT_TMR_COUNT                  0xF1     /*  Timer Count Numner Register  */



/*  MASKs  */
#define MASK_SRST                          0x01     /*  Software Reset  */
#define MASK_LDN                           0xFF     /*  Logic Device Number Register  */
#define MASK_DEV_ID                        0xFF     /*  Device ID Registers  */
#define MASK_VID                           0xFF     /*  Vendor ID Registers  */
#define MASK_CLKSEL                        0x01     /*  Clock Select Register */
#define MASK_WDT_ASSERT                    0x01     /*  Watchdog Timer - Assert a low pulse from WDTOUT# pin  */
#define MASK_WDT_RST_TIME                  0x02     /*  Watchdog Timer - Reset timer  */
#define MASK_PSEL_SEL_CONF_KEY             0x03     /*  Port Select Register - Select configuration entry key  */
#define MASK_PSEL_SEL_CONF_PORT            0x10     /*  Port Select Register - Select configuration port  */
#define MASK_UART_EN                       0x01     /*  UART Enable Register - Enable/Disable  */
#define MASK_UART_BADDR                    0xFF     /*  Base Address Register */
#define MASK_UART_IRQ_SEL                  0x0F     /*  IRQ Channel Select Register - Select IRQ channel  */
#define MASK_UART_IRQ_EN                   0x10     /*  IRQ Channel Select Register - Enable/Disable  */
#define MASK_UART_IRQ_SHARING_MOD          0x60     /*  IRQ Channel Select Register - Sharing Mode  */
#define MASK_UART_ENH_CLK_FREQ             0x03     /*  Enhanced Multifunction Register - Internal Clock frequency  */
#define MASK_UART_ENH_IR_TX_DELAY          0x04     /*  Enhanced Multifunction Register - IR mode TX Delay  */
#define MASK_UART_ENH_IR_RX_DELAY          0x08     /*  Enhanced Multifunction Register - IR mode RX Delay  */
#define MASK_UART_ENH_EN_AUTO_485          0x10     /*  Enhanced Multifunction Register - Enable/Disable Auto RS-485 Half-Duplex Control mode  */
#define MASK_UART_ENH_INVERT_POL_485       0x20     /*  Enhanced Multifunction Register - Invert the RTS#/RS485 signal polarity for RS485 Half-Duplex mode  */
#define MASK_UART_ENH_AUTO_ADDR_DETECT     0x40     /*  Enhanced Multifunction Register - Auto Address Detection  */
#define MASK_UART_ENH_ENABLE_9BIT_MOD      0x80     /*  Enhanced Multifunction Register - Enable/Disable the 9-bit mode  */
#define MASK_UART_IRC_IRRXA_INVERT         0x01     /*  IR Control Register - IR mode IRRXA# invert  */
#define MASK_UART_IRC_IRTXA_INVERT         0x02     /*  IR Control Register - IR mode IRTXA# invert  */
#define MASK_UART_IRC_HALF_DUPLEX          0x04     /*  IR Control Register - IR mode Half-Duplex */
#define MASK_UART_IRC_EN                   0x18     /*  IR Control Register - IR mode Enable  */
#define MASK_UART_FIFO_MOD_SEL_SIZE        0x02     /*  FIFO Mode Select Register - FIFO size for TX/RX  */
#define MASK_UART_FIFO_MOD_SEL_RX_TRG_L    0x20     /*  FIFO Mode Select Register - RX trigger level  */
#define MASK_UART_FIFO_MOD_SEL_TX_TRG_L    0x80     /*  FIFO Mode Select Register - TX trigger level  */
#define MASK_WDT_EN                        0x01     /*  Timer Count Numner Register - WDT Enable/Disable  */
#define MASK_WDT_BADDR                     0xFF     /*  Timer Count Numner Register - Base Address High/Low Regisers  */
#define MASK_WDT_IRQ_CH_SEL_WDT            0x0F     /*  IRQ Channel Select Register - Select the IRQ channel for watchdog timer  */
#define MASK_WDT_IRQ_CH_SEL_EN             0x10     /*  IRQ Channel Select Register - Enable/Disable the watchdog time IRQ  */
#define MASK_WDT_T_STATUS_TIME_OUT_EVN     0x01     /*  Timer Status And Control Register - Time Out Event  */
#define MASK_WDT_T_STATUS_WDT_INTERVAL     0x06     /*  Timer Status And Control Register - WDT Interval  */
#define MASK_WDT_TMR_COUNT                 0xFF     /*  Timer Count Numner Register  */




#define DEVICE_SEL_UARTA                   0x00
#define DEVICE_SEL_UARTB                   0x01
#define DEVICE_SEL_WDT                     0x08

#define DFL_DEV_ID_M                       0x03
#define DFL_DEV_ID_L                       0x82

#define  DFL_VEN_ID_M                       0x13
#define DFL_VEN_ID_L                       0xA8

#define CLK_SEL_24MHZ                      0x00
#define  CLK_SEL_48MHZ                      0x01

#define  PORT_ENTRY_KEY_77                  0x00
#define  PORT_ENTRY_KEY_A0                  0x01
#define  PORT_ENTRY_KEY_87                  0x02
#define  PORT_ENTRY_KEY_67                  0x03

#define  SEL_CONF_PORT_0                    0x00     /*  The configuration port is 0x2E/0x2F  */
#define  SEL_CONF_PORT_1                    0x01     /*  The configuration port is 0x4E/0x4F  */

#define  UART_DISABLE                       0x00
#define  UART_ENABLE                        0x01

#define  IRQ_SHARING_DISPLAY                0x00
#define IRQ_SHARING_ENABLE                 0x01

#define FIFO_TXRX_SIZE_16                  0x00
#define FIFO_TXRX_SIZE_32                  0x01
#define FIFO_TXRX_SIZE_64                  0x02
#define FIFO_TXRX_SIZE_128                 0x03

#define RX_TRIGGER_LEVEL_X1                0x00
#define RX_TRIGGER_LEVEL_X2                0x01
#define RX_TRIGGER_LEVEL_X4                0x02
#define RX_TRIGGER_LEVEL_X8                0x03

#define TX_HOLDING_NO_DELAY                0x00
#define TX_HOLDING_DELAY_1TX               0x01

#define WDT_DISABLE                        0x00
#define WDT_ENABLE                         0x01

#define WDT_IRQ_DISABLE                    0x00
#define WDT_IRQ_ENABLE                     0x01

#define WDT_INTERVAL_10MSEC                0x00
#define WDT_INTERVAL_1SEC                  0x01
#define WDT_INTERVAL_1MIN                  0x02


#define REG_EFER              0x2E    // Extended Function Enable/Index Register
#define REG_EFDR              0x2F    // Extended Function Data Register

#define ENTER_EXT_MODE_CODE                0x67
#define EXIT_EXT_MODE_CODE                 0xAA


#define XR28V382_UART_CLOCK                1843200  /* frequency of 24MHz with prescaler */ 
#define SUPERIO_UARTA_BASE                 0x3F8
#define SUPERIO_UARTB_BASE                 0x2F8



#define SLOT_UART_A                        3
#define SLOT_UART_B                        4


#endif     /* __SIO_XR28V382_DEVICE__  */