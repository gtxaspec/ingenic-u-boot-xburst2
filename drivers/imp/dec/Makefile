include $(TOPDIR)/config.mk

LIB := $(obj)libdec.o

COBJS-y += hdec_reg.o
COBJS-y += hdec_hal.o
COBJS-y += hdec.o

COBJS   := $(COBJS-y)
SRCS    := $(COBJS:.o=.c)
OBJS    := $(addprefix $(obj),$(COBJS))

all:    $(LIB)

$(LIB): $(obj).depend $(OBJS)
		$(call cmd_link_o_target, $(OBJS))

#########################################################################

#defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

# #########################################################################
