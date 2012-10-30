#
# Makefile for the kernel block device drivers.
#
# 12 June 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

# for nand test
obj-$(CONFIG_SUN4I_NANDFLASH_TEST) += nand/
obj-m       += spi_test.o
obj-m       += iic_add_or_remove_dev.o
obj-m	    += spi_test_add_dev.o
obj-m	    += spi_nor.o