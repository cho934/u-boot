/*
 *  Copyright 2007 by Spectrum Digital Incorporated.
 *  All rights reserved. Property of Spectrum Digital Incorporated.
 */

/*
 *  SPI ROM header file
 *
 */

#ifndef SPIROM_
#define SPIROM_

#ifdef __cplusplus
extern "C" {
#endif

#include "davincihd.h"

/* ------------------------------------------------------------------------ *
 *  SPI ROM Definitions                                                     *
 * ------------------------------------------------------------------------ */
#define SPIROM_SIZE             0x00008000
#define SPIROM_BASE             0x00000000
#define SPIROM_PAGESIZE         32
#define SPIROM_PAGEMASK         0xffffffc0

/* ------------------------------------------------------------------------ *
 *  SPI ROM Commands                                                        *
 * ------------------------------------------------------------------------ */
#define SPIROM_CMD_WRSR         0x01
#define SPIROM_CMD_WRITE        0x02
#define SPIROM_CMD_READ         0x03
#define SPIROM_CMD_WRDI         0x04
#define SPIROM_CMD_RDSR         0x05
#define SPIROM_CMD_WREN         0x06

/* ------------------------------------------------------------------------ *
 *  SPI Controller                                                          *
 * ------------------------------------------------------------------------ */
#define SPI_BASE                0x01c66800
#define SPI_SPIGCR0             *( volatile Uint32* )( SPI_BASE + 0x0 )
#define SPI_SPIGCR1             *( volatile Uint32* )( SPI_BASE + 0x4 )
#define SPI_SPIINT              *( volatile Uint32* )( SPI_BASE + 0x8 )
#define SPI_SPILVL              *( volatile Uint32* )( SPI_BASE + 0xc )
#define SPI_SPIFLG              *( volatile Uint32* )( SPI_BASE + 0x10 )
#define SPI_SPIPC0              *( volatile Uint32* )( SPI_BASE + 0x14 )
#define SPI_SPIPC2              *( volatile Uint32* )( SPI_BASE + 0x1c )
#define SPI_SPIDAT1_TOP         *( volatile Uint16* )( SPI_BASE + 0x3c )
#define SPI_SPIDAT1             *( volatile Uint32* )( SPI_BASE + 0x3c )
#define SPI_SPIDAT1_PTR16       *( volatile Uint16* )( SPI_BASE + 0x3e )
#define SPI_SPIDAT1_PTR8        *( volatile Uint8* ) ( SPI_BASE + 0x3f )
#define SPI_SPIBUF              *( volatile Uint32* )( SPI_BASE + 0x40 )
#define SPI_SPIBUF_PTR16        *( volatile Uint16* )( SPI_BASE + 0x42 )
#define SPI_SPIBUF_PTR8         *( volatile Uint8* ) ( SPI_BASE + 0x43 )
#define SPI_SPIEMU              *( volatile Uint32* )( SPI_BASE + 0x44 )
#define SPI_SPIDELAY            *( volatile Uint32* )( SPI_BASE + 0x48 )
#define SPI_SPIDEF              *( volatile Uint32* )( SPI_BASE + 0x4c )
#define SPI_SPIFMT0             *( volatile Uint32* )( SPI_BASE + 0x50 )
#define SPI_SPIFMT1             *( volatile Uint32* )( SPI_BASE + 0x54 )
#define SPI_SPIFMT2             *( volatile Uint32* )( SPI_BASE + 0x58 )
#define SPI_SPIFMT3             *( volatile Uint32* )( SPI_BASE + 0x5c )
#define SPI_INTVEC0             *( volatile Uint32* )( SPI_BASE + 0x60 )
#define SPI_INTVEC1             *( volatile Uint32* )( SPI_BASE + 0x64 )

/* ------------------------------------------------------------------------ *
 *  Prototype                                                               *
 * ------------------------------------------------------------------------ */
void spirom_init( );
void spirom_read( Uint16 src, Uint32 dst, Uint32 length );
void spirom_write( Uint32 src, Uint16 dst, Uint32 length );

#ifdef __cplusplus
}
#endif

#endif
