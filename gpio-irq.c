#include "gpio-irq.h"

XGpio Gpio;

XScuGic InterruptController;	     /* Instance of the Interrupt Controller */
static XScuGic_Config *GicConfig;    /* The configuration parameters of the */

/*
 * Create a shared variable to be used by the main thread of processing and
 * the interrupt processing
 */
volatile static int InterruptProcessed = 0;

volatile static int isr_id[8] = {121,122,123,
				 124,125,126,
				 127,128};

void DeviceDriverHandler(void *CallbackRef);
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr);
void trigger_irq (int irq_number);

int gpio_irq_test(void)
{
  int Status;

  /* Initialize the GPIO driver */
  Status = XGpio_Initialize(&Gpio, AXI_GPIO_DEVICE_ID);
  if (Status != XST_SUCCESS) {
    xil_printf("Gpio Initialization Failed\r\n");
    return XST_FAILURE;
  }

  // Make sure all IRQ request lines are low before configuring the GIC
  XGpio_DiscreteClear(&Gpio, GPIO_CHANNEL, 0xFF);

  // Configure GIC interrupts

  /*
   * Initialize the interrupt controller driver so that it is ready to
   * use.
   */
  GicConfig = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
  if (NULL == GicConfig) {
    return XST_FAILURE;
  }

  Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
				 GicConfig->CpuBaseAddress);
  if (Status != XST_SUCCESS) {
    return XST_FAILURE;
  }

  /*
   * Perform a self-test to ensure that the hardware was built
   * correctly
   */
  Status = XScuGic_SelfTest(&InterruptController);
  if (Status != XST_SUCCESS) {
    return XST_FAILURE;
  }

  /*
   * Setup the Interrupt System
   */
  Status = SetUpInterruptSystem(&InterruptController);
  if (Status != XST_SUCCESS) {
    return XST_FAILURE;
  }

  /*
   * Connect a device driver handler that will be called when an
   * interrupt for the device occurs, the device driver handler
   * performs the specific interrupt processing for the
   * device. Connecting all PL_PS_Group0 interrupts to the same
   * handler. The handler determines which interrupt was asserted
   * during the isr.
   */
  for (int i = 0; i <= 7; i++)
    {

	  // Using the same handler body, but associating with different callback data
	  // so that the handler will know which IRQ it is responsible for servicing
      Status = XScuGic_Connect(&InterruptController, isr_id[i],
			       (Xil_ExceptionHandler)DeviceDriverHandler,
			       (void*) &(isr_id[i]));

      if (Status != XST_SUCCESS) {
	return XST_FAILURE;
      }


    }

  /*
   * Wait for the interrupt to be processed, if the interrupt does not
   * occur this loop will wait forever
   */
  InterruptProcessed = 0;
  for (int i = 0; i <= 7; i++) {

    trigger_irq( isr_id[i] );

    /*
     * If the interrupt occurred which is indicated by the global
     * variable which is set in the device driver handler, then
     * stop waiting
     */
    while (InterruptProcessed == 0);

    xil_printf ("Processed Interrupt ID: %d\r\n", InterruptProcessed);

    InterruptProcessed = 0;
  }
  return XST_SUCCESS;
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{

  /*
   * Connect the interrupt controller interrupt handler to the hardware
   * interrupt handling logic in the ARM processor.
   */
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			       (Xil_ExceptionHandler) XScuGic_InterruptHandler,
			       XScuGicInstancePtr);

  /*
   * Enable interrupts in the ARM
   */
  Xil_ExceptionEnable();

  return XST_SUCCESS;
}

// Trigger a PL to PS interrupt via the AXI GPIO IP
// irq_number is 121-128 corresponding with PL_PS_Group0 from TRM
void trigger_irq (int irq_number)
{
  // Convert irq_num to GPIO bit
  int irq = 1 << (irq_number - 121);

  /*
   * Enable the interrupt
   */
  XScuGic_Enable(&InterruptController, irq_number);

  /* Set the IRQ to High */
  XGpio_DiscreteSet(&Gpio, GPIO_CHANNEL, irq);

}

void DeviceDriverHandler(void *CallbackRef)
{
  int gicp2_sts, gicp3_sts, PL_PS_Group0;

  // Determine which PL to PS line is asserted by reading GIC Proxy IRQ STS
  gicp2_sts    = (int) Xil_In32 (XLPD_SLCR_GICP2_IRQ_STS);
  PL_PS_Group0 = gicp2_sts >> 25;
  gicp3_sts    = (int) Xil_In32 (XLPD_SLCR_GICP3_IRQ_STS);
  PL_PS_Group0 |= (gicp3_sts << 7);
  PL_PS_Group0 &= 0xFF;

  /* Clear the IRQ bit */
  XGpio_DiscreteClear(&Gpio, GPIO_CHANNEL, PL_PS_Group0);

  // Grab the callback data that was setup when this handler was connected
  PL_PS_Group0 = *((int*)CallbackRef);

  // Disable the interrupt
  XScuGic_Disable(&InterruptController, PL_PS_Group0);

  // Clear the STS bits
  Xil_Out32 (XLPD_SLCR_GICP2_IRQ_STS, gicp2_sts);
  Xil_Out32 (XLPD_SLCR_GICP3_IRQ_STS, gicp3_sts);

  /*
   * Indicate the interrupt has been processed using a shared variable
   */
  InterruptProcessed = PL_PS_Group0;

}
