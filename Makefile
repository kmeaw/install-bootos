.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT must be set in the environment.")
endif

include $(PSL1GHT)/Makefile.base

SED		?=	sed
SELFNPDRM355	:=	make_self_npdrm
PKG355		:=	package_finalize

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCE		:=	source
INCLUDE		:=	include
DATA		:=	data
LIBS		:=	-lnet -lgcm_sys -lreality -lsysutil -lio -lm -lsysmodule

APPEGO		:=	KM0001
APPTITLE	:=	BootOS installer
APPVERSION	:=	0000200
APPID		:=	LNX000001
APPRAND		:=	$(shell openssl rand -hex 4 | tr [:lower:] [:upper:])
CONTENTID	:=	$(APPEGO)-$(APPID)_00-0$(APPVERSION)$(APPRAND)
SFOXML		:=	sfo.xml
ICON_PNG	:=	ICON0.PNG
PIC1_PNG	:=	pkg/pic1.png
#PIC2_PNG	:=	pkg/pic2.png

CFLAGS		+= -g -O2 -Wall --std=gnu99 -D__POWERPC__ -DDEBUG=0
ifdef FIRMWARE
CFLAGS          += -DFIRMWARE=$(FIRMWARE)
endif
CXXFLAGS	+= -g -O2 -Wall -D__POWERPC__ -DDEBUG=0

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCE),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export BUILDDIR	:=	$(CURDIR)/$(BUILD)
export DEPSDIR	:=	$(BUILDDIR)

CFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CXXFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))

export OFILES	:=	$(CFILES:.c=.o) \
					$(CXXFILES:.cpp=.o) \
					$(SFILES:.S=.o)

export BINFILES	:=	$(BINFILES:.bin=.bin.h)

export INCLUDES	:=	$(foreach dir,$(INCLUDE),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD)
export LIBPATHS :=      $(foreach dir,$(LIBDIRS),-L$(CURDIR)/$(dir))

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	make_self_npdrm $(TARGET).elf $(TARGET).self $(CONTENTID)

clean:
	@echo Clean...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT).pkg

bin: $(BUILD)
	make_self_npdrm $(OUTPUT).elf EBOOT.BIN $(CONTENTID)

indent:
	indent -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8 -il1 source/*.c

pkg: $(BUILD)
	@echo Creating PSL1GHT PKG... $(CONTENTID)
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON_PNG) $(BUILD)/pkg/ICON0.PNG
	@cp bootos.bin $(BUILD)/pkg/USRDIR
#	@cp $(PIC1_PNG) $(BUILD)/pkg/PIC1.PNG
#	@cp $(PIC2_PNG) $(BUILD)/pkg/PIC2.PNG
	@$(SELFNPDRM355) $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN $(CONTENTID)
	@$(SFO) --title "$(APPTITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).pkg
	@$(PKG355) $(OUTPUT).pkg

run: pkg
	ncftpput -E 192.168.2.130 /dev_hdd0/game/$(APPID)/USRDIR $(BUILD)/pkg/USRDIR/EBOOT.BIN
	#@$(PS3LOADAPP) $(OUTPUT).self

restart: $(BUILD)
	@expect restart
	@sleep 1
	@$(PS3LOADAPP) $(OUTPUT).self

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(BINFILES)

-include $(DEPENDS)

endif
