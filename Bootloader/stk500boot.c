/*****************************************************************************
Title:     STK500v2 compatible bootloader
Author:    Peter Fleury  <pfleury@gmx.ch> http://tinyurl.com/peterfleury
File:      $Id: stk500boot.c,v 1.19 2015/08/23 15:34:01 peter Exp $
Compiler:  avr-gcc 4.8.1 / avr-libc 1.8 
Hardware:  All AVRs with bootloader support, tested with ATmega8
License:   GNU General Public License Version 3
          
DESCRIPTION:
    This program allows an AVR with bootloader capabilities to 
    read/write its own Flash/EEprom. To enter Programming mode   
    an input pin is checked. If this pin is pulled low, programming mode  
    is entered. If not, normal execution is done from $0000 
    "reset" vector in Application area.
    Size < 500 words, fits into a 512 word bootloader section 
     
   
USAGE:
    - Set AVR MCU type and clock-frequency (F_CPU) in the Makefile.
    - Set bootloader start address in bytes (BOOTLOADER_ADDRESS) in Makefile
      this must match selected "Boot Flash section size" fuses below
    - Set baud rate below (AVRISP only works with 115200 bps)
    - compile/link the bootloader with the supplied Makefile
    - program the "Boot Flash section size" (BOOTSZ fuses),
      for boot-size 512 words:  program BOOTSZ1
    - enable the BOOT Reset Vector (program BOOTRST)
    - Upload the hex file to the AVR using any ISP programmer
    - Program Boot Lock Mode 3 (program BootLock 11 and BootLock 12 lock bits)
    - Reset your AVR while keeping PROG_PIN pulled low
    - Start AVRISP Programmer (AVRStudio/Tools/Program AVR)
    - AVRISP will detect the bootloader
    - Program your application FLASH file and optional EEPROM file using AVRISP
    
Note: 
    Erasing the device without flashing, through AVRISP GUI button "Erase Device" 
    is not implemented, due to AVRStudio limitations.
    Flash is always erased before programming.
    
    Normally the bootloader accepts further commands after programming. 
    The bootloader exits and starts applicaton code after programming 
    when ENABLE_LEAVE_BOOTLADER is defined.
    Use Auto Programming mode to programm both flash and eeprom, 
    otherwise bootloader will exit after flash programming.
    
    AVRdude:
    Please uncomment #define REMOVE_CMD_SPI_MULTI when using AVRdude.
    Comment #define REMOVE_PROGRAM_LOCK_BIT_SUPPORT and 
    #define REMOVE_READ_LOCK_FUSE_BIT_SUPPORT to reduce code size.
    Read Fuse Bits and Read/Write Lock Bits is not supported when using AVRdude.

NOTES:
    Based on Atmel Application Note AVR109 - Self-programming
    Based on Atmel Application Note AVR068 - STK500v2 Protocol    
          
LICENSE:
    Copyright (C) 2006 Peter Fleury
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include "command.h"

/*
 * Uncomment the following lines to save code space 
 */
//#define REMOVE_PROGRAM_LOCK_BIT_SUPPORT    // disable program lock bits
//#define REMOVE_READ_LOCK_FUSE_BIT_SUPPORT  // disable reading lock and fuse bits
//#define REMOVE_BOOTLOADER_LED              // no LED to show active bootloader
//#define REMOVE_PROG_PIN_PULLUP             // disable internal pull-up, use external pull-up resistor
#define REMOVE_CMD_SPI_MULTI               // disable processing of SPI_MULTI commands (SPI MULTI command only used by AVRdude)

/*
 *  Uncomment to leave bootloader and jump to application after programming.
 */
//#define ENABLE_LEAVE_BOOTLADER           

/* 
 * Pin "PROG_PIN" on port "PROG_PORT" has to be pulled low
 * (active low) to start the bootloader 
 * uncomment #define REMOVE_PROG_PIN_PULLUP if using an external pullup
 */
#define PROG_PORT  PORTD
#define PROG_DDR   DDRD
#define PROG_IN    PIND
#define PROG_PIN   PD2

/*
 * Active-low LED on pin "PROGLED_PIN" on port "PROGLED_PORT" 
 * indicates that bootloader is active
 */
#define PROGLED_PORT PORTB
#define PROGLED_DDR  DDRB
#define PROGLED_PIN  PB5


/*
 * define which UART channel will be used, if device with two UARTs is used
 */
#define USE_USART0        // use first USART
//#define USE_USART1      // use second USART
 
/*
 * UART Baudrate, AVRStudio AVRISP only accepts 115200 bps
 */
#define BAUDRATE 115200


/*
 *  Enable (1) or disable (0) USART double speed operation
 */
#define UART_BAUDRATE_DOUBLE_SPEED 0


/*
 * HW and SW version, reported to AVRISP, must match version of AVRStudio
 */
#define CONFIG_PARAM_BUILD_NUMBER_LOW   0
#define CONFIG_PARAM_BUILD_NUMBER_HIGH  0
#define CONFIG_PARAM_HW_VER             0x0F
#define CONFIG_PARAM_SW_MAJOR           2
#define CONFIG_PARAM_SW_MINOR           0x0A



/*
 * Calculate the address where the application section ends from FLASHEND and from start address of bootloader
 * The address where the bootloader starts is passed by Makefile using -DBOOTLOADER_ADDRESS
 */
#define APP_END (BOOTLOADER_ADDRESS - 1)


/*
 *  Defines for the various USART registers
 */
#ifdef USE_USART0
 
#ifdef UBRRL
#define UART_BAUD_RATE_LOW       UBRRL
#else
#ifdef UBRR0L
#define UART_BAUD_RATE_LOW       UBRR0L
#endif
#endif 

#ifdef UBRR0H
#define UART_BAUD_RATE_HIGH      UBRR0H
#endif

#ifdef UCSRA
#define UART_STATUS_REG          UCSRA
#else
#ifdef UCSR0A
#define UART_STATUS_REG          UCSR0A
#endif
#endif 

#ifdef UCSRB
#define UART_CONTROL_REG         UCSRB
#else
#ifdef UCSR0B
#define UART_CONTROL_REG         UCSR0B
#endif
#endif

#ifdef TXEN
#define UART_ENABLE_TRANSMITTER  TXEN
#else
#ifdef TXEN0
#define UART_ENABLE_TRANSMITTER  TXEN0
#endif
#endif

#ifdef RXEN
#define UART_ENABLE_RECEIVER     RXEN
#else
#ifdef RXEN0
#define UART_ENABLE_RECEIVER     RXEN0
#endif
#endif

#ifdef TXC
#define UART_TRANSMIT_COMPLETE   TXC
#else
#ifdef TXC0
#define UART_TRANSMIT_COMPLETE   TXC0
#endif
#endif

#ifdef RXC
#define UART_RECEIVE_COMPLETE    RXC
#else
#ifdef RXC0
#define UART_RECEIVE_COMPLETE    RXC0
#endif
#endif

#ifdef UDR
#define UART_DATA_REG            UDR
#else
#ifdef UDR0
#define UART_DATA_REG            UDR0
#endif
#endif

#ifdef U2X
#define UART_DOUBLE_SPEED        U2X
#else
#ifdef U2X0
#define UART_DOUBLE_SPEED        U2X0
#endif
#endif

/* ATMega with two USART, select second USART for bootloader using USE_USART1 define */
#else
#ifdef USE_USART1

#ifdef UBRR1L
#define UART_BAUD_RATE_LOW       UBRR1L
#endif

#ifdef UBRR1H
#define UART_BAUD_RATE_HIGH      UBRR1H
#endif

#ifdef UCSR1A
#define UART_STATUS_REG          UCSR1A
#endif

#ifdef UCSR1B
#define UART_CONTROL_REG         UCSR1B
#endif

#ifdef TXEN1
#define UART_ENABLE_TRANSMITTER  TXEN1
#endif

#ifdef RXEN1
#define UART_ENABLE_RECEIVER     RXEN1
#endif

#ifdef TXC1
#define UART_TRANSMIT_COMPLETE   TXC1
#endif

#ifdef RXC1
#define UART_RECEIVE_COMPLETE    RXC1
#endif

#ifdef UDR1
#define UART_DATA_REG            UDR1
#endif

#ifdef U2X1
#define UART_DOUBLE_SPEED        U2X1
#endif

#else
#error "USE_USART0 / USE_USART1 undefined  !"
#endif
#endif

#if defined(UART_BAUD_RATE_LOW) && defined(UART_STATUS_REG) && defined(UART_CONTROL_REG) && defined(UART_ENABLE_TRANSMITTER) && defined(UART_ENABLE_RECEIVER) && defined(UART_TRANSMIT_COMPLETE) && defined(UART_RECEIVE_COMPLETE) && defined(UART_DATA_REG) && defined(UART_DOUBLE_SPEED)
#else
#error "no UART definition for MCU available"
#endif


/*
 * Macros to map the new ATmega88/168 EEPROM bits
 */
#ifdef EEMPE                                                    
#define EEMWE EEMPE
#define EEWE  EEPE
#endif


/*
 * Macro to calculate UBBR from XTAL and baudrate
 */
#if UART_BAUDRATE_DOUBLE_SPEED
#define UART_BAUD_SELECT(baudRate,xtalCpu) (((float)(xtalCpu))/(((float)(baudRate))*8.0)-1.0+0.5)
#else
#define UART_BAUD_SELECT(baudRate,xtalCpu) (((float)(xtalCpu))/(((float)(baudRate))*16.0)-1.0+0.5)
#endif


/*
 * States used in the receive state machine
 */
#define ST_START        0
#define ST_GET_SEQ_NUM  1
#define ST_MSG_SIZE_1   2
#define ST_MSG_SIZE_2   3
#define ST_GET_TOKEN    4
#define ST_GET_DATA     5
#define ST_GET_CHECK    6
#define ST_PROCESS      7


/*
 * use 16bit address variable for ATmegas with <= 64K flash
 */
#if defined(RAMPZ)
typedef uint32_t address_t;
#else
typedef uint16_t address_t;
#endif


/*
 * function prototypes
 */ 
static void sendchar(char c);
static unsigned char recchar(void);


/*
 * since this bootloader is not linked against the avr-gcc crt1 functions,
 * to reduce the code size, we need to provide our own initialization
 */
// void __jumpMain     (void) __attribute__ ((naked)) __attribute__ ((section (".init9")));

// void __jumpMain(void)
// {    
//     asm volatile ( ".set __stack, %0" :: "i" (RAMEND) );       // init stack
//     asm volatile ( "clr __zero_reg__" );                       // GCC depends on register r1 set to 0
//     asm volatile ( "out %0, __zero_reg__" :: "I" (_SFR_IO_ADDR(SREG)) );  // set SREG to 0
// #ifndef REMOVE_PROG_PIN_PULLUP   
//     PROG_PORT |= (1<<PROG_PIN);                                // Enable internal pullup
// #endif    
//     asm volatile ( "rjmp main");                               // jump to main()
// }


/*
 * send single byte to USART, wait until transmission is completed
 */
static void sendchar(char c)
{
    UART_DATA_REG = c;                                         // prepare transmission
    while (!(UART_STATUS_REG & (1 << UART_TRANSMIT_COMPLETE)));// wait until byte sent
    UART_STATUS_REG |= (1 << UART_TRANSMIT_COMPLETE);          // delete TXCflag
}

/*
 * Read single byte from USART, block if no data available
 */
static unsigned char recchar(void)
{
    while(!(UART_STATUS_REG & (1 << UART_RECEIVE_COMPLETE)));  // wait for data
    return UART_DATA_REG;
}


int main(void) __attribute__ ((OS_main));
int main(void)
{
    address_t       address = 0;
    address_t       eraseAddress = 0;   
    unsigned char   msgParseState;
    unsigned int    i = 0;
    unsigned char   checksum = 0;
    unsigned char   seqNum = 0;
    unsigned int    msgLength = 0;
    unsigned char   msgBuffer[285];
    unsigned char   c, *p;
    unsigned char   isLeave = 0;

    PROG_DDR &= ~(1<<PROG_PIN);
    PROG_PORT |= (1<<PROG_PIN);

    /*
     * Branch to bootloader or application code ?
     */ 
    if(!(PROG_IN & (1<<PROG_PIN)))  
    {       
#ifndef REMOVE_BOOTLOADER_LED
        /* PROG_PIN pulled low, indicate with LED that bootloader is active */
        PROGLED_DDR  |= (1<<PROGLED_PIN);
        PROGLED_PORT |= (1<<PROGLED_PIN);
#endif
        /*
         * Init UART
         * set baudrate and enable USART receiver and transmiter without interrupts 
         */     
#if UART_BAUDRATE_DOUBLE_SPEED
        UART_STATUS_REG   |=  (1 <<UART_DOUBLE_SPEED);
#endif
  
#ifdef UART_BAUD_RATE_HIGH    
        UART_BAUD_RATE_HIGH = 0;     
#endif       
        UART_BAUD_RATE_LOW = UART_BAUD_SELECT(BAUDRATE,F_CPU);
        UART_CONTROL_REG   = (1 << UART_ENABLE_RECEIVER) | (1 << UART_ENABLE_TRANSMITTER); 
        
        
        /* main loop */
        while(!isLeave)                             
        {   
            /*
             * Collect received bytes to a complete message
             */            
            msgParseState = ST_START;
            while ( msgParseState != ST_PROCESS )
            {
                c = recchar();
                switch (msgParseState)
                {
                case ST_START:
                    if( c == MESSAGE_START )
                    {
                        msgParseState = ST_GET_SEQ_NUM;
                        checksum = MESSAGE_START^0;
                    }
                    break;
                    
                case ST_GET_SEQ_NUM:
                    if ( (c == 1) || (c == seqNum) )
                    {
                        seqNum = c;
                        msgParseState = ST_MSG_SIZE_1;
                        checksum ^= c;
                    }
                    else
                    {
                        msgParseState = ST_START;
                    }
                    break;
                    
                case ST_MSG_SIZE_1:             
                    msgLength = (unsigned int)c<<8;
                    msgParseState = ST_MSG_SIZE_2;
                    checksum ^= c;
                    break;
                    
                case ST_MSG_SIZE_2:         
                    msgLength |= c;
                    msgParseState = ST_GET_TOKEN;
                    checksum ^= c;
                    break;
                
                case ST_GET_TOKEN:
                    if ( c == TOKEN )
                    {
                        msgParseState = ST_GET_DATA;
                        checksum ^= c;
                        i = 0;
                    }
                    else
                    {
                        msgParseState = ST_START;
                    }
                    break;
                
                case ST_GET_DATA:                   
                    msgBuffer[i++] = c;
                    checksum ^= c;
                    if ( i == msgLength )
                    {
                        msgParseState = ST_GET_CHECK;
                    }
                    break;
                        
                case ST_GET_CHECK:
                    if( c == checksum )
                    {
                        msgParseState = ST_PROCESS;                     
                    }
                    else
                    {
                        msgParseState = ST_START;
                    }
                    break;
                }//switch
            }//while(msgParseState)
            
            /*
             * Now process the STK500 commands, see Atmel Appnote AVR068
             */
            
            switch (msgBuffer[0])
            {
#ifndef REMOVE_CMD_SPI_MULTI
            case CMD_SPI_MULTI:
                {
                    unsigned char answerByte = 0;

                    // only Read Signature Bytes implemented, return dummy value for other instructions
                    if ( msgBuffer[4]== 0x30 )
                    {                       
                        unsigned char signatureIndex = msgBuffer[6];                        

                        if ( signatureIndex == 0 )
                            answerByte = SIGNATURE_0;
                        else if ( signatureIndex == 1 )
                            answerByte = SIGNATURE_1;
                        else
                            answerByte = SIGNATURE_2;
                    }                   
                    msgLength = 7;
                    msgBuffer[1] = STATUS_CMD_OK;
                    msgBuffer[2] = 0;                   
                    msgBuffer[3] = msgBuffer[4];  // Instruction Byte 1
                    msgBuffer[4] = msgBuffer[5];  // Instruction Byte 2
                    msgBuffer[5] = answerByte;                  
                    msgBuffer[6] = STATUS_CMD_OK;
                }
                break;
#endif
            case CMD_SIGN_ON:
                msgLength = 11;                             
                msgBuffer[1]  = STATUS_CMD_OK;
                msgBuffer[2]  = 8;
                msgBuffer[3]  = 'A';
                msgBuffer[4]  = 'V';
                msgBuffer[5]  = 'R';
                msgBuffer[6]  = 'I';
                msgBuffer[7]  = 'S';
                msgBuffer[8]  = 'P';
                msgBuffer[9]  = '_';
                msgBuffer[10] = '2';
                break;
            
            case CMD_GET_PARAMETER:
                switch(msgBuffer[1])
                {
                case PARAM_BUILD_NUMBER_LOW:
                    msgBuffer[2] = CONFIG_PARAM_BUILD_NUMBER_LOW;
                    break;                  
                case PARAM_BUILD_NUMBER_HIGH:
                    msgBuffer[2] = CONFIG_PARAM_BUILD_NUMBER_HIGH;
                    break;                  
                case PARAM_HW_VER:
                    msgBuffer[2] = CONFIG_PARAM_HW_VER;
                    break;                  
                case PARAM_SW_MAJOR:
                    msgBuffer[2] = CONFIG_PARAM_SW_MAJOR;
                    break;
                case PARAM_SW_MINOR:
                    msgBuffer[2] = CONFIG_PARAM_SW_MINOR;
                    break;              
                default:
                    msgBuffer[2] = 0;
                    break;
                }
                msgLength = 3;              
                msgBuffer[1] = STATUS_CMD_OK;
                break;
                
            case CMD_LEAVE_PROGMODE_ISP:
#ifdef ENABLE_LEAVE_BOOTLADER
                isLeave = 1;
#endif
            case CMD_ENTER_PROGMODE_ISP:                    
            case CMD_SET_PARAMETER:         
                msgLength = 2;              
                msgBuffer[1] = STATUS_CMD_OK;
                break;

            case CMD_READ_SIGNATURE_ISP:
                {
                    unsigned char signatureIndex = msgBuffer[4];
                    unsigned char signature;

                    if ( signatureIndex == 0 )
                        signature = SIGNATURE_0;
                    else if ( signatureIndex == 1 )
                        signature = SIGNATURE_1;
                    else
                        signature = SIGNATURE_2;

                    msgLength = 4;
                    msgBuffer[1] = STATUS_CMD_OK;
                    msgBuffer[2] = signature;
                    msgBuffer[3] = STATUS_CMD_OK;                   
                }
                break;

#ifndef REMOVE_READ_LOCK_FUSE_BIT_SUPPORT
            case CMD_READ_LOCK_ISP:            
                msgLength = 4;
                msgBuffer[1] = STATUS_CMD_OK;
                msgBuffer[2] = boot_lock_fuse_bits_get( GET_LOCK_BITS );
                msgBuffer[3] = STATUS_CMD_OK;                                                   
                break;
            
            case CMD_READ_FUSE_ISP:
                {                    
                    unsigned char fuseBits;                    
                    
                    if ( msgBuffer[2] == 0x50 )
                    {
                        if ( msgBuffer[3] == 0x08 )
                            fuseBits = boot_lock_fuse_bits_get( GET_EXTENDED_FUSE_BITS );                            
                        else
                            fuseBits = boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS );                            
                    }
                    else 
                    {
                        fuseBits = boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS );
                    }                    
                    msgLength = 4;    
                    msgBuffer[1] = STATUS_CMD_OK;
                    msgBuffer[2] = fuseBits;                    
                    msgBuffer[3] = STATUS_CMD_OK;                                       
                }
                break;
#endif                
#ifndef REMOVE_PROGRAM_LOCK_BIT_SUPPORT
            case CMD_PROGRAM_LOCK_ISP:
                {
                    unsigned char lockBits = msgBuffer[4];
                    
                    lockBits = (~lockBits) & 0x3C;  // mask BLBxx bits
                    boot_lock_bits_set(lockBits);   // and program it
                    boot_spm_busy_wait();
                    
                    msgLength = 3;
                    msgBuffer[1] = STATUS_CMD_OK;                   
                    msgBuffer[2] = STATUS_CMD_OK;                                                           
                }
                break;
#endif
            case CMD_CHIP_ERASE_ISP:
                eraseAddress = 0;
                msgLength = 2;
                msgBuffer[1] = STATUS_CMD_OK;
                break;

            case CMD_LOAD_ADDRESS:
#if defined(RAMPZ)
                address = ( ((address_t)(msgBuffer[1])<<24)|((address_t)(msgBuffer[2])<<16)|((address_t)(msgBuffer[3])<<8)|(msgBuffer[4]) )<<1;
#else
                address = ( ((msgBuffer[3])<<8)|(msgBuffer[4]) )<<1;  //convert word to byte address
#endif
                msgLength = 2;
                msgBuffer[1] = STATUS_CMD_OK;
                break;
                
            case CMD_PROGRAM_FLASH_ISP:
            case CMD_PROGRAM_EEPROM_ISP:                
                {
                    unsigned int  size = (((unsigned int)msgBuffer[1])<<8) | msgBuffer[2];
                    unsigned char *p = msgBuffer+10;
                    unsigned int  data;
                    unsigned char highByte, lowByte;                    
                    address_t     tempaddress = address;

                   
                    if ( msgBuffer[0] == CMD_PROGRAM_FLASH_ISP )
                    {
                        // erase only main section (bootloader protection)
                        if  (  eraseAddress < APP_END )
                        {
                            boot_page_erase(eraseAddress);  // Perform page erase
                            boot_spm_busy_wait();       // Wait until the memory is erased.
                            eraseAddress += SPM_PAGESIZE;    // point to next page to be erase
                        }
                        
                        /* Write FLASH */
                        do {
                            lowByte   = *p++;
                            highByte  = *p++;
                            
                            data =  (highByte << 8) | lowByte;
                            boot_page_fill(address,data);
                                                    
                            address = address + 2;      // Select next word in memory
                            size -= 2;          // Reduce number of bytes to write by two    
                        } while(size);          // Loop until all bytes written
                        
                        boot_page_write(tempaddress);
                        boot_spm_busy_wait();   
                        boot_rww_enable();              // Re-enable the RWW section                    
                    }
                    else
                    {
                        /* write EEPROM */
                        do {
                            EEARL = address;            // Setup EEPROM address
                            EEARH = (address >> 8);
                            address++;                  // Select next EEPROM byte
        
                            EEDR= *p++;                 // get byte from buffer
                            EECR |= (1<<EEMWE);         // Write data into EEPROM
                            EECR |= (1<<EEWE);
                        
                            while (EECR & (1<<EEWE));   // Wait for write operation to finish
                            size--;                     // Decrease number of bytes to write
                        } while(size);                  // Loop until all bytes written                     
                    }
                    msgLength = 2;
                    msgBuffer[1] = STATUS_CMD_OK;                   
                }
                break;
                
            case CMD_READ_FLASH_ISP:
            case CMD_READ_EEPROM_ISP:                                                
                {
                    unsigned int  size = (((unsigned int)msgBuffer[1])<<8) | msgBuffer[2];
                    unsigned char *p = msgBuffer+1;
                    msgLength = size+3;
                    
                    *p++ = STATUS_CMD_OK;                    
                    if (msgBuffer[0] == CMD_READ_FLASH_ISP )
                    {
                        unsigned int data;
                        
                        // Read FLASH
                        do {                            
#if defined(RAMPZ)
                            data = pgm_read_word_far(address);
#else
                            data = pgm_read_word_near(address);
#endif
                            *p++ = (unsigned char)data;         //LSB
                            *p++ = (unsigned char)(data >> 8);  //MSB  
                            address    += 2;     // Select next word in memory
                            size -= 2;
                        }while (size);
                    }
                    else
                    {
                        /* Read EEPROM */
                        do {
                            EEARL = address;            // Setup EEPROM address
                            EEARH = ((address >> 8));
                            address++;                  // Select next EEPROM byte
                            EECR |= (1<<EERE);          // Read EEPROM
                            *p++ = EEDR;                // Send EEPROM data
                            size--;                     
                        }while(size);
                    }
                    *p++ = STATUS_CMD_OK;
                }
                break;

            default:
                msgLength = 2;   
                msgBuffer[1] = STATUS_CMD_FAILED;
                break;
            }

            /*
             * Now send answer message back
             */
            sendchar(MESSAGE_START);     
            checksum = MESSAGE_START^0;
            
            sendchar(seqNum);
            checksum ^= seqNum;
            
            c = ((msgLength>>8)&0xFF);
            sendchar(c);
            checksum ^= c;
            
            c = msgLength&0x00FF;
            sendchar(c);
            checksum ^= c;
            
            sendchar(TOKEN);
            checksum ^= TOKEN;

            p = msgBuffer;
            while ( msgLength )
            {                
               c = *p++;
               sendchar(c);
               checksum ^=c;
               msgLength--;               
            }                   
            sendchar(checksum);         
            seqNum++;

        }//for

#ifndef REMOVE_BOOTLOADER_LED
        PROGLED_DDR  &= ~(1<<PROGLED_PIN);   // set to default     
#endif
    }
    
    /*
     * Now leave bootloader
     */
#ifndef REMOVE_PROG_PIN_PULLUP  
    PROG_PORT &= ~(1<<PROG_PIN);    // set to default
#endif  
    boot_rww_enable();              // enable application section
    
    // Jump to Reset vector in Application Section
    // (clear register, push this register to the stack twice = adress 0x0000/words, and return to this address) 
    asm volatile ( 
        "clr r1" "\n\t"
        "push r1" "\n\t"
        "push r1" "\n\t" 
        "ret"     "\n\t" 
    ::); 

     /*
     * Never return to stop GCC to generate exit return code
     * Actually we will never reach this point, but the compiler doesn't 
     * understand this
     */
    for(;;);
}
