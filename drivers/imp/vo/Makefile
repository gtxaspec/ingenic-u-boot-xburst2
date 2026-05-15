include $(TOPDIR)/config.mk

LIB := $(obj)libvo.o

COBJS-y += vde.o
COBJS-y += vde_hal.o

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
