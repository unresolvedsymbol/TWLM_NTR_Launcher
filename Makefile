#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	NTR_Launcher
export TOPDIR		:=	$(CURDIR)

export VERSION_MAJOR	:= 1
export VERSION_MINOR	:= 99
export VERSTRING	:=	$(VERSION_MAJOR).$(VERSION_MINOR)

#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: cardengine_arm7 bootloader clean arm7/$(TARGET).elf arm9/$(TARGET).elf

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: cardengine_arm7 bootloader $(TARGET).nds $(TARGET).cia title

$(TARGET).nds:	$(TARGET).arm7 $(TARGET).arm9
	ndstool	-c $(TARGET).nds -7 $(TARGET).arm7.elf -9 $(TARGET).arm9.elf \
			-b $(CURDIR)/icon.bmp "NTR Launcher;Enhanced cart loader;Chishm, Apache Thunder" \
			-g KKGP 01 "NTR LAUNCHER" -z 80040000 -u 00030004

title: $(TARGET).nds
	mkdir -p title/00030004/4b4b4750/content
	cp $(TARGET).nds title/00030004/4b4b4750/content/00000000.app
	maketmd $(TARGET).nds title/00030004/4b4b4750/content/title.tmd

$(TARGET).cia: $(TARGET).nds
	make_cia --srl=$(TARGET).nds

$(TARGET).arm7	: arm7/$(TARGET).elf
	cp arm7/$(TARGET).elf $(TARGET).arm7.elf
$(TARGET).arm9	: arm9/$(TARGET).elf
	cp arm9/$(TARGET).elf $(TARGET).arm9.elf

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9
	
#---------------------------------------------------------------------------------
cardengine_arm7: data
	@$(MAKE) -C cardengine_arm7

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr data
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).nds.orig.nds
	@rm -fr $(TARGET).arm7
	@rm -fr $(TARGET).arm9
	@rm -fr $(TARGET).arm7.elf
	@rm -fr $(TARGET).arm9.elf
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C cardengine_arm7 clean

data:
	@mkdir -p data

bootloader: data
	@$(MAKE) -C bootloader
