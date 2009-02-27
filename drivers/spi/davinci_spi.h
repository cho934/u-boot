/*
 * Register definitions for the DaVinci SPI Controller
 */

/* Register offsets */
#define DAVINCI_SPI_GCR0		0x0000
#define DAVINCI_SPI_GCR1		0x0004
#define DAVINCI_SPI_INT0		0x0008
#define DAVINCI_SPI_LVL			0x000c
#define DAVINCI_SPI_FLG			0x0010
#define DAVINCI_SPI_PC0			0x0014
#define DAVINCI_SPI_PC1			0x0018
#define DAVINCI_SPI_PC2			0x001c
#define DAVINCI_SPI_PC3			0x0020
#define DAVINCI_SPI_PC4			0x0024
#define DAVINCI_SPI_PC5			0x0028
#define DAVINCI_SPI_DAT0		0x0038
#define DAVINCI_SPI_DAT1		0x003c
#define DAVINCI_SPI_BUF			0x0040
#define DAVINCI_SPI_EMU			0x0044
#define DAVINCI_SPI_DELAY		0x0048
#define DAVINCI_SPI_DEF			0x004c
#define DAVINCI_SPI_FMT0		0x0050
#define DAVINCI_SPI_FMT1		0x0054
#define DAVINCI_SPI_FMT2		0x0058
#define DAVINCI_SPI_FMT3		0x005c
#define DAVINCI_SPI_INTVEC0		0x0060
#define DAVINCI_SPI_INTVEC1		0x0064
	
struct davinci_spi_slave {
	struct spi_slave slave;
	void		*regs;
	u32		mr;
	unsigned int freq;
};

static inline struct davinci_spi_slave *to_davinci_spi(struct spi_slave *slave)
{
	return container_of(slave, struct davinci_spi_slave, slave);
}

#define spi_readl(as, reg)					\
	readl(CFG_SPI_BASE + DAVINCI_SPI_##reg)
#define spi_writel(as, reg, value)				\
	writel(value, CFG_SPI_BASE + DAVINCI_SPI_##reg)

