#include <common.h>
#include <usb.h>

#define CPM_BASE_EDWIN               (0X10000000)

/* power and clock gate */
#define CPM_USB0PCR1		(0x5c)
#define CPM_USB1PCR1		(0x6c)
#define CPM_USB2PCR1		(0x7c)

#define CPM_USBVBFIL		(0x58)
#define CPM_USB1VBFIL		(0x68)
#define CPM_USB2VBFIL		(0x78)

#define CPM_USBRDT		(0x54)
#define CPM_USB1RDT		(0x64)
#define CPM_USB2RDT		(0x74)

#define CPM_USBPCR              (0x50)
#define CPM_USB1PCR              (0x60)
#define CPM_USB2PCR              (0x70)

#define CPM_CPAPCR              (0x10)
#define CPM_CLKGR0		(0x30)	/* clock gate register 0 */
#define CPM_SCR		        (0x34)	/* special control register, oscillator and power control register*/
#define CPM_SRBC0		(0xf0)	/* soft reset and bus control register */

#define USBRDT_VBFIL_LD_EN		25
#define USBRDT_UTMI_RST		        27
#define USBPCR_POR			22
#define USBPCR_USB_MODE			31
#define USBPCR_COMMONONN		25
#define USBPCR_VBUSVLDEXT		24
#define USBPCR_VBUSVLDEXTSEL		23
#define USBPCR_OTG_DISABLE		20
#define USBPCR_SIDDQ			21
#define USBPCR_IDPULLUP_MASK		28

#define OPCR_SPENDN0			7
#define OPCR_SPENDN1			6
#define OPCR_SPENDN2			5

#define USBPCR1_USB_SEL			28
#define USBPCR1_WORD_IF0		19
#define USBPCR1_WORD_IF1		18

#define SRBC_USB0_SR			17
#define SRBC_USB1_SR			12
#define SRBC_USB2_SR			16

#define CLKGR0_GATE_OTG0                11
#define CLKGR0_GATE_OTG1                12
#define CLKGR0_GATE_OTG2                13

/*USB Parameter Control Register1*/
#define USBPCR1_BVLD_REG                31
#define USBPCR1_IDPULLUP_ZEAR           30
#define USBPCR1_DPPULLDOWN		29
#define USBPCR1_DMPULLDOWN              28

#define reg_set_bit(bit,reg)           *(volatile unsigned int *)(CPM_BASE_EDWIN+reg) |= 1UL<<(bit)
#define reg_clr_bit(bit,reg)	       *(volatile unsigned int *)(CPM_BASE_EDWIN+reg) &= ~(1UL<<(bit))
#define reg_wr(val,reg)       	       *(volatile unsigned int *)(CPM_BASE_EDWIN+reg) = (val)
#define reg_rd(reg)		       *(volatile unsigned int *)(CPM_BASE_EDWIN+reg)
#define writeReg(reg,val)              *(volatile unsigned int *)(reg) = (val)

int board_usb_init(void)
{

	unsigned int usbpcr1,tmp;

        //printf("board_usb_init\n");

	/*feed clock to otg*/
	reg_clr_bit(CLKGR0_GATE_OTG0,CPM_CLKGR0);
	mdelay(100);
	/* softreset otg */
	reg_set_bit(SRBC_USB0_SR, CPM_SRBC0);
	udelay(40);
	reg_clr_bit(SRBC_USB0_SR, CPM_SRBC0);

	//reg_set_bit(8, CPM_USB0PCR1);
	//reg_set_bit(9, CPM_USB0PCR1);
	reg_set_bit(USBPCR1_DMPULLDOWN, CPM_USB0PCR1);
	reg_set_bit(USBPCR1_DPPULLDOWN, CPM_USB0PCR1);
	reg_set_bit(USBPCR1_IDPULLUP_ZEAR, CPM_USB0PCR1);

	reg_clr_bit(USBPCR1_WORD_IF0, CPM_USB0PCR1);
	usbpcr1 = reg_rd(CPM_USB0PCR1);
	usbpcr1 &= ~(0x7 << 23);
	usbpcr1 |= (5 << 23);
	reg_wr(usbpcr1, CPM_USB0PCR1);

	reg_wr(0, CPM_USBVBFIL);

	//reg_wr(0x96, CPM_USBRDT);

	reg_set_bit(USBRDT_VBFIL_LD_EN, CPM_USBRDT);

	//reg_wr(0x8380385a, CPM_USBPCR);

	tmp = reg_rd(CPM_USBPCR);
	tmp |= 1 << USBPCR_USB_MODE | 1 << USBPCR_COMMONONN;
	tmp &= ~(1 << USBPCR_OTG_DISABLE | 1 << USBPCR_SIDDQ |
			0x03 << USBPCR_IDPULLUP_MASK | 1 << USBPCR_VBUSVLDEXT |
			1 << USBPCR_VBUSVLDEXTSEL);
	reg_wr(tmp, CPM_USBPCR);

	reg_set_bit(USBPCR_POR, CPM_USBPCR);
	reg_clr_bit(USBRDT_UTMI_RST, CPM_USBRDT);
	reg_set_bit(SRBC_USB0_SR, CPM_SRBC0);
	udelay(10);
	reg_clr_bit(USBPCR_POR, CPM_USBPCR);

	udelay(20);
	reg_set_bit(OPCR_SPENDN0, CPM_SCR);     //edwin.wen@2022/03/29 if OPCR_SPENDN not set,dwc softrst will timeout.
	mdelay(10);

	udelay(950);
	reg_set_bit(USBRDT_UTMI_RST, CPM_USBRDT);

	udelay(20);
	reg_clr_bit(SRBC_USB0_SR, CPM_SRBC0);
#if 0
        writeReg(0x10060030,0xf);
        writeReg(0x10060124,0x1c);
        writeReg(0x10060040,0x69); //1

        writeReg(0x10060064,0x80); //2
#endif
	mdelay(10);
	return 0;
}


int board_usb1_init(void)
{

	unsigned int usbpcr1,tmp;

        //printf("board_usb1_init\n");
	/*feed clock to otg*/
	reg_clr_bit(CLKGR0_GATE_OTG1,CPM_CLKGR0);
	mdelay(100);
	/* softreset otg */
	reg_set_bit(SRBC_USB1_SR, CPM_SRBC0);
	udelay(40);
	reg_clr_bit(SRBC_USB1_SR, CPM_SRBC0);

	//reg_set_bit(8, CPM_USB0PCR1);
	//reg_set_bit(9, CPM_USB0PCR1);
	reg_set_bit(USBPCR1_DMPULLDOWN, CPM_USB1PCR1);
	reg_set_bit(USBPCR1_DPPULLDOWN, CPM_USB1PCR1);
	reg_set_bit(USBPCR1_IDPULLUP_ZEAR, CPM_USB1PCR1);

	reg_clr_bit(USBPCR1_WORD_IF0, CPM_USB1PCR1);
	usbpcr1 = reg_rd(CPM_USB1PCR1);
	usbpcr1 &= ~(0x7 << 23);
	usbpcr1 |= (5 << 23);
	reg_wr(usbpcr1, CPM_USB1PCR1);

	reg_wr(0, CPM_USB1VBFIL);

	//reg_wr(0x96, CPM_USBRDT);

	reg_set_bit(USBRDT_VBFIL_LD_EN, CPM_USB1RDT);

	//reg_wr(0x8380385a, CPM_USBPCR);

	tmp = reg_rd(CPM_USB1PCR);
	tmp |= 1 << USBPCR_USB_MODE | 1 << USBPCR_COMMONONN;
	tmp &= ~(1 << USBPCR_OTG_DISABLE | 1 << USBPCR_SIDDQ |
			0x03 << USBPCR_IDPULLUP_MASK | 1 << USBPCR_VBUSVLDEXT |
			1 << USBPCR_VBUSVLDEXTSEL);
	reg_wr(tmp, CPM_USB1PCR);

	reg_set_bit(USBPCR_POR, CPM_USB1PCR);
	reg_clr_bit(USBRDT_UTMI_RST, CPM_USB1RDT);
	reg_set_bit(SRBC_USB1_SR, CPM_SRBC0);
	udelay(10);
	reg_clr_bit(USBPCR_POR, CPM_USB1PCR);

	udelay(20);
	reg_set_bit(OPCR_SPENDN1, CPM_SCR);     //edwin.wen@2022/03/29 if OPCR_SPENDN not set,dwc softrst will timeout.
	mdelay(10);

	udelay(950);
	reg_set_bit(USBRDT_UTMI_RST, CPM_USB1RDT);

	udelay(20);
	reg_clr_bit(SRBC_USB1_SR, CPM_SRBC0);

#if 0
        writeReg(0x10060430,0xf);
        writeReg(0x10060524,0x1c);
        writeReg(0x10060440,0x69);

        writeReg(0x10060464,0x80);
#endif
	mdelay(10);
	return 0;
}

int board_usb2_init(void)
{

	unsigned int usbpcr1,tmp;

        //printf("board_usb2_init\n");
	/*feed clock to otg*/
	reg_clr_bit(CLKGR0_GATE_OTG2,CPM_CLKGR0);
	mdelay(100);
	/* softreset otg */
	reg_set_bit(SRBC_USB2_SR, CPM_SRBC0);
	udelay(40);
	reg_clr_bit(SRBC_USB2_SR, CPM_SRBC0);

	//reg_set_bit(8, CPM_USB0PCR1);
	//reg_set_bit(9, CPM_USB0PCR1);
	reg_set_bit(USBPCR1_DMPULLDOWN, CPM_USB2PCR1);
	reg_set_bit(USBPCR1_DPPULLDOWN, CPM_USB2PCR1);
	reg_set_bit(USBPCR1_IDPULLUP_ZEAR, CPM_USB2PCR1);

	reg_clr_bit(USBPCR1_WORD_IF0, CPM_USB2PCR1);
	usbpcr1 = reg_rd(CPM_USB2PCR1);
	usbpcr1 &= ~(0x7 << 23);
	usbpcr1 |= (5 << 23);
	reg_wr(usbpcr1, CPM_USB2PCR1);

	reg_wr(0, CPM_USB2VBFIL);

	//reg_wr(0x96, CPM_USBRDT);

	reg_set_bit(USBRDT_VBFIL_LD_EN, CPM_USB2RDT);

	//reg_wr(0x8380385a, CPM_USBPCR);

	tmp = reg_rd(CPM_USB2PCR);
	tmp |= 1 << USBPCR_USB_MODE | 1 << USBPCR_COMMONONN;
	tmp &= ~(1 << USBPCR_OTG_DISABLE | 1 << USBPCR_SIDDQ |
			0x03 << USBPCR_IDPULLUP_MASK | 1 << USBPCR_VBUSVLDEXT |
			1 << USBPCR_VBUSVLDEXTSEL);
	reg_wr(tmp, CPM_USB2PCR);

	reg_set_bit(USBPCR_POR, CPM_USB2PCR);
	reg_clr_bit(USBRDT_UTMI_RST, CPM_USB2RDT);
	reg_set_bit(SRBC_USB2_SR, CPM_SRBC0);
	udelay(10);
	reg_clr_bit(USBPCR_POR, CPM_USB2PCR);

	udelay(20);
	reg_set_bit(OPCR_SPENDN2, CPM_SCR);     //edwin.wen@2022/03/29 if OPCR_SPENDN not set,dwc softrst will timeout.
	mdelay(10);

	udelay(950);
	reg_set_bit(USBRDT_UTMI_RST, CPM_USB2RDT);

	udelay(20);
	reg_clr_bit(SRBC_USB2_SR, CPM_SRBC0);

#if 0
        writeReg(0x10060830,0xf);
        writeReg(0x10060924,0x1c);
        writeReg(0x10060840,0x69);

        writeReg(0x10060864,0x80);
#endif
	mdelay(10);
	return 0;
}
