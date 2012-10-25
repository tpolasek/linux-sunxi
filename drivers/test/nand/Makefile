#obj-y		+= nfd/

#obj-$(CONFIG_SUN4I_NANDFLASH_TEST)		+= nfd/
#obj-$(CONFIG_SUN4I_NANDFLASH_TEST)		+= nandtest/

obj-$(CONFIG_SUN4I_NANDFLASH_TEST)  += nand.o
nand-y := nfd/nand_blk.o nfd/mbr.o nfd/nand_osal_for_linux.o\
src/format/nand_format.o\
src/logic/bad_manage.o src/logic/logic_ctl.o src/logic/mapping.o src/logic/mapping_base.o src/logic/merge.o src/logic/read_reclaim.o src/logic/wear_levelling.o src/logic/logic_cache.o\
src/scan/nand_scan.o src/scan/nand_id.o\
src/physic/nand_phy.o src/physic/nand_simple_r.o src/physic/nand_simple_w.o\
src/nfc/nfc_r.o src/nfc/nfc_w.o\
nandtest/nand_test.o
