/*
 *  Copyright 2007 b7 Spectrum Digital Incorporated.
 *  All rights reserved. Property of Spectrum Digital Incorporated.
 */

/*
 *  SPI ROM interface
 *
 */

#include "spirom.h"

static Uint8 spirombuf[SPIROM_PAGESIZE + 5];
static Uint8 statusbuf[8];
static Uint32 spidat1;

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  _wait( delay )                                                          *
 *      Wait in a software loop for 'x' delay                               *
 *                                                                          *
 * ------------------------------------------------------------------------ */
void DAVINCIHD_wait( Uint32 delay )
{
    volatile Uint32 i;
    for ( i = 0 ; i < delay ; i++ ){ };
}


/* ------------------------------------------------------------------------ *
 *  spirom_init( )                                                          *
 * ------------------------------------------------------------------------ */
void spirom_init( )
{
    /* Reset SPI */
    SPI_SPIGCR0 = 0;
    _wait( 1000 );

    /* Release SPI */
    SPI_SPIGCR0 = 1;

    /* SPI 4-Pin Mode setup */
    SPI_SPIGCR1 = 0
        | ( 0 << 24 )
        | ( 0 << 16 )
        | ( 1 << 1 )
        | ( 1 << 0 );

    SPI_SPIPC0 = 0
        | ( 1 << 11 )   // DI
        | ( 1 << 10 )   // DO
        | ( 1 << 9 )    // CLK
        | ( 1 << 1 )    // EN1
        | ( 1 << 0 );   // EN0

    SPI_SPIFMT0 = 0
        | ( 0 << 20 )   // SHIFTDIR
        | ( 0 << 17 )   // Polarity
        | ( 1 << 16 )   // Phase
        | ( 50 << 8 )    // Prescale
        | ( 8 << 0 );   // Char Len

    spidat1 = 0
        | ( 1 << 28 )   // CSHOLD
        | ( 0 << 24 )   // Format [0]
        | ( 2 << 16 )   // CSNR   [only CS0 enbled]
        | ( 0 << 0 );   //

    SPI_SPIDAT1 = spidat1;

    SPI_SPIDELAY = 0
        | ( 8 << 24 )   // C2TDELAY
        | ( 8 << 16 );  // T2CDELAY

    SPI_SPIDEF = 0
        | ( 1 << 1 )    // EN1 inactive high
        | ( 1 << 0 );   // EN0 inactive high

    SPI_SPIINT = 0
        | ( 0 << 16 )   //
        | ( 0 << 8 )    //
        | ( 0 << 6 )    //
        | ( 1 << 4 );   //

    SPI_SPILVL = 0
        | ( 0 << 8 )    // EN0
        | ( 0 << 6 )    // EN0
        | ( 0 << 4 );   // EN0


    /* Enable SPI */
    SPI_SPIGCR1 |= ( 1 << 24 );
}

/* ------------------------------------------------------------------------ *
 *  spirom_cycle( buf, len )                                                *
 *                                                                          *
 *  Execute a SPI spirom data transfer cycle.  Each byte in buf is shifted  *
 *  out and replaced with data coming back from the spirom.                 *
 * ------------------------------------------------------------------------ */
void spirom_cycle( Uint8 *buf, Uint16 len )
{
    Uint16 i;

    /* Clear any old data */
    SPI_SPIBUF;

    /* SPIROM access cycle */
    for ( i = 0 ; i <= len ; i++ )
    {
        /* Wait for transmit ready */
        while ( SPI_SPIBUF & 0x10000000 );

        if ( i == len )
            SPI_SPIDAT1 = ( spidat1 & 0x0ffcffff ) | buf[i];
        else
            SPI_SPIDAT1 = spidat1 | buf[i];

        /* Wait for receive data ready */
        while ( SPI_SPIBUF & 0x80000000 );

        /* Read 1 byte */
        buf[i] = SPI_SPIBUF;
    }
}

/* ------------------------------------------------------------------------ *
 *  spirom_status( )                                                        *
 * ------------------------------------------------------------------------ */
Uint8 spirom_status( )
{
    /* Issue read status command */
    statusbuf[0] = SPIROM_CMD_RDSR;
    statusbuf[1] = 0;

    spirom_cycle( statusbuf, 2 );

    return statusbuf[1];
}

/* ------------------------------------------------------------------------ *
 *  spirom_read( src, dst, length )                                         *
 * ------------------------------------------------------------------------ */
void spirom_read( Uint16 src, Uint32 dst, Uint32 length )
{
    Int32 i;
    Uint8 *psrc, *pdst;

    // Setup command
    spirombuf[0] = SPIROM_CMD_READ;
    spirombuf[1] = ( src >> 8 );
    spirombuf[2] = ( src >> 0 );

    // Execute spirom read cycle
    spirom_cycle( spirombuf, length + 3 );

    // Copy returned data
    pdst = ( Uint8 * )dst;
    psrc = spirombuf + 3;
    for ( i = 0 ; i < length ; i++ )
        *pdst++ = *psrc++;
}

/* ------------------------------------------------------------------------ *
 *  spirom_write( src, dst, length )                                        *
 * ------------------------------------------------------------------------ */
void spirom_write( Uint32 src, Uint16 dst, Uint32 length )
{
    Int32 i;
    Int32 bytes_left;
    Int32 bytes_to_program;
    Uint8 *psrc;

    /* Establish source */
    psrc = ( Uint8 * )src;
    bytes_left = length;

    while ( bytes_left > 0 )
    {
        bytes_to_program = bytes_left;

        /* Most to program is SPIROM_CMD_BLOCKSIZE */
        if ( bytes_to_program > SPIROM_PAGESIZE )
             bytes_to_program = SPIROM_PAGESIZE;

        /* Make sure you don't run off the end of a block */
        if ( ( dst & SPIROM_PAGEMASK ) != ( ( dst + bytes_to_program ) & SPIROM_PAGEMASK ) )
            bytes_to_program -= ( dst + bytes_to_program ) - ( ( dst + bytes_to_program ) & SPIROM_PAGEMASK );

        /* Issue WPEN */
        spirombuf[0] = SPIROM_CMD_WREN;
        spirom_cycle( spirombuf, 0 );

        /* Create command block for program operation */
        spirombuf[0] = SPIROM_CMD_WRITE;
        spirombuf[1] = ( Uint8 )( dst >> 8 );
        spirombuf[2] = ( Uint8 )( dst );

        for ( i = 0 ; i < bytes_to_program ; i++ )
            spirombuf[3+i] = *psrc++;

        /* Execute write command */
        spirom_cycle( spirombuf, bytes_to_program + 2 );

        /* Wait while busy */
        while( ( spirom_status( ) & 0x01 ) );

        /* Get ready for next iteration */
        bytes_left -= bytes_to_program;
        dst += bytes_to_program;
    }
}
