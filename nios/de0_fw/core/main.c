/*-----------------------------------------------------------------------------

   1  ABSTRACT

   1.1 Module Type

      DE0-I APPLICATION

   1.2 Functional Description

      This module is responsible for implementing the main embedded
      application for the TERASIC DE0-nano board.

      Requires --override=nios2-flash-override.txt when running the
      nios2-flash-programmer command.

   1.3 Specification/Design Reference

      See fw_cfg.h under the share directory.

   1.4 Module Test Specification Reference

      None

   1.5 Compilation Information

      See fw_cfg.h under the share directory.

   1.6 Notes

      NONE

   2  CONTENTS

      1 ABSTRACT
        1.1 Module Type
        1.2 Functional Description
        1.3 Specification/Design Reference
        1.4 Module Test Specification Reference
        1.5 Compilation Information
        1.6 Notes

      2 CONTENTS

      3 VOCABULARY

      4 EXTERNAL RESOURCES
        4.1  Include Files
        4.2  External Data Structures
        4.3  External Function Prototypes

      5 LOCAL CONSTANTS AND MACROS

      6 MODULE DATA STRUCTURES
        6.1  Local Function Prototypes
        6.2  Local Data Structures

      7 MODULE CODE
        7.1  main()
        7.2  timer()
        7.3  version()
        7.4  com_hwver()

-----------------------------------------------------------------------------*/

// 3 VOCABULARY

// 4 EXTERNAL RESOURCES

// 4.1  Include Files

#include "main.h"

// message string table
#include "msg_str.h"

// 4.2   External Data Structures

   // global control
   gc_t     gc;
   // configurable items
   ci_t     ci;

   // month table for date-time strings
   char  *month_table[] = {
            "JAN", "FEB", "MAR", "APR",
            "MAY", "JUN", "JUL", "AUG",
            "SEP", "OCT", "NOV", "DEC"
          };

   uint32_t   bootTable[32] __attribute__ ((section (".bootTable")));

// 4.3   External Function Prototypes

// 5 LOCAL CONSTANTS AND MACROS

// 6 MODULE DATA STRUCTURES

// 6.1  Local Function Prototypes

// 6.2  Local Data Structures

   // Heart Beat
   static   uint8_t  hb_led[] = {0, 0, 1, 1, 0, 0, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1};

   static   uint8_t  hb_cnt   =  0;
   static   uint8_t  led_cnt  =  0;

   static   char     clr_scrn[] = {0x1B, '[', '2', 'J', 0x00};
   static   char     cur_home[] = {0x1B, '[', 'H', 0x00};

// 7 MODULE CODE

// ===========================================================================

// 7.1

int main() {

/* 7.1.1   Functional Description

   This is the main entry point for the embedded application, it is called
   by the alt_main() function from HAL.

   7.1.2   Parameters:

   NONE

   7.1.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.1.4   Data Structures

   uint32_t       i;
   flash_region  *regions;
   int            num_regions;
   char           firstFile[64] = ALTERA_RO_ZIPFS_NAME;

// 7.1.5   Code

   // Open Debug Port
   xlprint_open(STDOUT_BASE);

   // Clear the Terminal Screen and Home the Cursor
   xlprint(clr_scrn);
   xlprint(cur_home);

   // Display the Startup Banner
   xlprint("\nDE0-I NIOS, %s\n\n", BUILD_HI);

   // Clear GC
   memset(&gc, 0, sizeof(gc_t));

   // Initialize GC
   gc.feature   = 0;
   gc.trace     = 0;
   gc.debug     = 0;
   gc.status    = CFG_STATUS_INIT;
   gc.error     = CFG_ERROR_CLEAR;
   gc.devid     = CM_DEV_DE0;
   gc.winid     = CM_DEV_WIN;
   gc.com_port  = CM_PORT_COM0;
   gc.int_flag  = FALSE;
   gc.sw_reset  = FALSE;
   gc.sys_time  = 0;
   gc.ping_time = alt_timestamp();
   gc.ping_cnt  = 0;
   gc.led_cycle = CFG_LED_CYCLE;
   gc.month     = month_table;
   gc.msg_table = msg_table;
   gc.msg_table_len = DIM(msg_table);

   sprintf(gc.dev_str, "DE0-I NIOS, %s", BUILD_STR);

   // Report Serial Flash Regions
   gc.fd = alt_flash_open_dev(EPCS_NAME);
   if (gc.fd) {
      // Retrieve the regions map
      alt_get_flash_info(gc.fd, &regions, &num_regions);
      // Report regions
      xlprint("flash: %s\n", EPCS_NAME);
      xlprint("flash.num_regions: %d\n", num_regions);
      for (i=0;i<num_regions;i++) {
         xlprint("flash.regions[%d].offset:     %d\n", i, regions[i].offset);
         xlprint("flash.regions[%d].region_size %d\n", i, regions[i].region_size);
         xlprint("flash.regions[%d].num_blocks  %d\n", i, regions[i].number_of_blocks);
         xlprint("flash.regions[%d].block_size  %d\n", i, regions[i].block_size);
      }

      // Report R/O Zip File System
      xlprint("\n%-13s base:offset %08X:%08X\n", ALTERA_RO_ZIPFS_NAME, ALTERA_RO_ZIPFS_BASE, ALTERA_RO_ZIPFS_OFFSET);

      // Report R/O Zip File System Directory
      strcat(firstFile, "/");
      alt_ro_zipfs_dir(firstFile);

      // Partial R/O Zip File System Dump
      xlprint("%-13s partial content ...\n\n", ALTERA_RO_ZIPFS_NAME);
      alt_epcs_flash_read(gc.fd, ALTERA_RO_ZIPFS_OFFSET, gc.buf, 256);
      dump(gc.buf, 64, LIB_OFFSET | LIB_ASCII, ALTERA_RO_ZIPFS_OFFSET);

      // Open Stimulus File and Dump
      gc.fp = fopen("/mnt/rozipfs/Cello.wav", "rb");
      if (gc.fp == NULL) gc.error |= CFG_ERROR_FILE;
      else {
         // File Size
         fstat(fileno(gc.fp), &gc.st);
         xlprint("%s, size:%d, partial ...\n", firstFile, gc.st.st_size);
         // Read Wave Header
         memset(&gc.wav, 0, sizeof(gc.wav));
         fread(&gc.wav, 1, sizeof(gc.wav), gc.fp);
         xlprint("samples:     %d\n", gc.wav.chunk2_size >> 1);
         xlprint("sample rate: %d\n", gc.wav.samp_rate);
         xlprint("bits:        %d\n\n", gc.wav.bits_per_samp);
         // Read Wave Data
         memset(gc.buf, 0, sizeof(gc.buf));
         i = fread(gc.buf, 1, 64, gc.fp);
         dump(gc.buf, i, LIB_ASCII, 0);
         fclose(gc.fp);
      }
   }

   // Initialize the Configurable Items DataBase
   // and read the stored CIs, flash must be initialized
   // before the CI file is read
   gc.error |= ci_init();
   gc.error |= ci_read();

   //
   // INIT THE HARDWARE
   //

   // GPIO Init
   gc.error |= gpio_init();

   // CM Init
   gc.error |= cm_init();

   // OPTO Port
   gc.error |= opto_init(CFG_BAUD_RATE, gc.com_port);

   // ADC Init
   gc.error |= adc_init();

   // Report Push Button 0-1 setting
   gc.key = gpio_key();
   xlprint("keys:  %02X\n", gc.key);

   // Report DIP Switch Setting
   gc.dip_sw = gpio_dip();
   xlprint("dipsw: %02X\n", gc.dip_sw);

   // STAMP Init
   gc.error |= stamp_init();

   // Report Ticks per Second
   xlprint("ticks/sec: %d\n", alt_ticks_per_second());

   // Report Timestamp Frequency
   xlprint("timestamp.freq: %d.%d MHz\n", alt_timestamp_freq() / 1000000,
         alt_timestamp_freq() % 1000000);

   // Report NIOS Frequency
   xlprint("nios.freq: %d.%d MHz\n", ALT_CPU_CPU_FREQ / 1000000,
         ALT_CPU_CPU_FREQ % 1000000);

   // System ID and Unique Build time stamp
   gc.sysid = stamp_sysid();
   gc.timestamp = (time_t)stamp_epoch();
   gc.fpga_time = stamp_time();
   gc.fpga_date = stamp_date();
   gc.fpga_ver  = stamp_version();

   // Check System IDs
   if (gc.sysid != FPGA_PID || gc.timestamp != FPGA_EPOCH) {
      gc.error |= CFG_ERROR_ID;
   }

   // Report Versions
   version();

   // Start the Periodic Timer
   alt_alarm_start(&gc.alarm, CFG_TIMER_CYCLE, timer, NULL);

   // NIOS-II Processor Registers
   NIOS2_READ_STATUS(i);
   xlprint("nios.status:  %08X\n", i);
   NIOS2_READ_IENABLE(i);
   xlprint("nios.ienable: %08X\n", i);
   NIOS2_READ_CPUID(i);
   xlprint("nios.cpuid:   %08X\n", i);

   // Partial SDRAM Dump
   xlprint("\nsdram partial ...\n\n");
   dump((uint8_t *)SDRAM_BASE, 64, LIB_ADDR | LIB_ASCII, 0);

   // Partial SDRAM_FIFO Dump
   xlprint("\nsdram_fifo partial ...\n\n");
   dump((uint8_t *)SDRAM_FIFO_REGION_BASE, 64, LIB_ADDR | LIB_ASCII, 0);

   // Partial EPCS Boot Dump
   xlprint("\nepcs boot partial ...\n\n");
   dump((uint8_t *)EPCS_BASE, 64, LIB_ADDR | LIB_ASCII, 0);

   // Power-On Self Test
   gc.error |= post_all();

   // Print Status and Error Results to Serial Port
   if (gc.trace & CFG_TRACE_POST) {
      xlprint("trace   :  %08X\n", gc.trace);
      xlprint("feature :  %08X\n", gc.feature);
      xlprint("status  :  %08X\n", gc.status);
      xlprint("error   :  %08X\n", gc.error);
   }

   //
   // START THE SERVICES
   //

   // Control Panel (CP)
   gc.error |= cp_hal_init();
   gc.error |= cp_init();

   // DAQ Controller (DAQ)
   gc.error |= daq_hal_init();
   gc.error |= daq_init();

   // H/W and F/W Mismatch
   if (gc.error & CFG_ERROR_ID) {
      gpio_set_val(GPIO_LED_ERR, GPIO_LED_ON);
      // slow down heart beat
      gc.led_cycle += (CFG_LED_CYCLE * 3);
   }

   // All LEDs Off
   gpio_set_val(0, GPIO_LED_ALL_OFF);

   // Initialization Finished so
   // start Running
   gc.status &= ~CFG_STATUS_INIT;
   gc.status |=  CFG_STATUS_RUN;

   // Start the Software Watchdog
   out32(WATCHDOG_BASE+4, 0x04);

   // Init the Command Line Interpreter
   cli_init();

   // Register the STDOUT interrupt ISRs for CLI
   alt_ic_isr_register(STDOUT_IRQ_INTERRUPT_CONTROLLER_ID,
                       STDOUT_IRQ, xlprint_isr, NULL, NULL);

   //
   // BACKGROUND PROCESSING
   //
   // NOTE: All Background thread operations begin
   //       from this for-loop! Further, all foreground
   //       processing not done in the interrupt must
   //       start through this for-loop!
   //
   for (;;) {
      //
      // CM THREAD
      //
      cm_thread();
      //
      // CP THREAD
      //
      cp_thread();
      //
      // DAQ THREAD
      //
      daq_thread();
      //
      // CLI THREAD
      //
      cli_process(&gc.cli);
      //
      // UPDATE QSYS WATCHDOG
      //
      if (gc.sw_reset != TRUE) out32(WATCHDOG_BASE+8, 0x00);
   }

   // Unreachable code
   return 0;

} // end main()

// ===========================================================================

// 7.2

alt_u32 timer(void *context) {

/* 7.2.1   Functional Description

   This is the main system timer callback function for handling background
   periodic events. The callback is registered with the HAL alarm facility.

   NOTE: Timing intervals are not precise for this callback and the
         return value sets the next timeout period in units of alt_nticks.

   7.2.2   Parameters:

   context  Callback parameter, unused

   7.2.3   Return Values:

   return   Next timeout period in units of alt_nticks.

-----------------------------------------------------------------------------
*/

// 7.2.4   Data Structures

   uint8_t     key;
   uint32_t    i;

// 7.2.5   Code

   // System Time Tick
   gc.sys_time++;

   // Period Service Ticks
   cp_tick();
   daq_tick();
   cm_tick();

   // Activity Indicator
   if (++led_cnt >= gc.led_cycle) {
      led_cnt = 0;
      // Heart Beat
      gpio_set_val(GPIO_LED_HB, hb_led[(hb_cnt++ & 0xF)]);
      // COM Indicator Off
      gpio_set_val(GPIO_LED_COM, GPIO_LED_OFF);
      // PIPE Indicator Off
      gpio_set_val(GPIO_LED_PIPE, GPIO_LED_OFF);
      // Set Fault LED for Errors
      gpio_set_val(GPIO_LED_ERR, gc.error ? GPIO_LED_ON : GPIO_LED_OFF);
      // Set Fault LED when Running
      gpio_set_val(GPIO_LED_ERR, (gc.status & CFG_STATUS_DAQ_RUN) ? GPIO_LED_ON : GPIO_LED_OFF);
   }

   // Read the User Pushbutton Switches, de-bounced by timer
   key = gc.key ^ gpio_key();
   gc.key = gpio_key();
   if (key != 0) {
      for (i=0;i<4;i++) {
         switch (key & (1 << i)) {
         case GPIO_KEY_0 :
            // PB Down
            if ((gc.key & GPIO_KEY_0) == 0) {
               xlprint("key0 pressed\n");
            }
            break;
         case GPIO_KEY_1 :
            // PB Down
            if ((gc.key & GPIO_KEY_1) == 0) {
               xlprint("key1 pressed\n");
            }
            break;
         }
      }
   }

   return CFG_TIMER_CYCLE;

} // end timer()


// ===========================================================================

// 7.3

void version(void) {

/* 7.3.1   Functional Description

   Report firmware and hardware version detail to STDOUT.

   7.3.2   Parameters:

   NONE

   7.3.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.3.4   Data Structures

   struct tm     *quartus;

// 7.3.5   Code

   // Hardware Devices
   xlprint("\n");
   xlprint("%-13s base:irq %08X:%d\n", SDRAM_NAME, SDRAM_BASE, SDRAM_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", EPCS_NAME, EPCS_BASE, EPCS_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", SYSCLK_NAME, SYSCLK_BASE, SYSCLK_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", SYSTIMER_NAME, SYSTIMER_BASE, SYSTIMER_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", WATCHDOG_NAME, WATCHDOG_BASE, WATCHDOG_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", GPX_NAME, GPX_BASE, GPX_IRQ);
   xlprint("%-13s base:irq %08X:%d\n", GPI_NAME, GPI_BASE, GPI_IRQ);
   xlprint("%-13s base:irq %08X:%d\n\n", STDOUT_NAME, STDOUT_BASE, STDOUT_IRQ);

   xlprint("%-13s base:rev:irq %08X:%08X:%d\n", STAMP_NAME, STAMP_BASE, stamp_version(), STAMP_IRQ);
   xlprint("%-13s base:rev:irq %08X:%d:%d\n", ADC_NAME, ADC_BASE, adc_version(), ADC_IRQ);
   xlprint("%-13s base:rev:irq %08X:%d:%d\n\n", OPTO_NAME, OPTO_BASE, com_hwver(), OPTO_IRQ);

   xlprint("hw/sw stamp.id: %d %d\n", gc.sysid, FPGA_PID);
   xlprint("hw/sw stamp.epoch: %d %d\n", (uint32_t)gc.timestamp, FPGA_EPOCH);
   xlprint("hw stamp.time:  %08X\n", stamp_time());
   xlprint("hw stamp.date:  %08X\n", stamp_date());
   xlprint("hw stamp.magic: %08X\n", stamp_magic());
   xlprint("fpga_ver: %d.%d.%d build %d\n",
        gc.fpga_ver >> 24 & 0xFF,
        gc.fpga_ver >> 12 & 0xFFF,
        gc.fpga_ver >>  0 & 0xFFF,
        stamp_inc());

   quartus = localtime(&gc.timestamp);
   xlprint("Quartus Build : %s", asctime(quartus));
   xlprint("\nDE0-I NIOS, %s\n\n", BUILD_HI);

   // Report warning if System ID and Time Stamp
   // do not match fpga_build.h entries.
   if (gc.error & CFG_ERROR_ID) {
      xlprint("\n");
      xlprint("****************************************\n");
      xlprint("*** Warning: H/W and F/W out of sync ***\n");
      xlprint("****************************************\n");
      xlprint("\n");
   }

   xlprint("\n");

} // end version()


// ===========================================================================

// 7.4

uint32_t com_hwver(void) {

/* 7.4.1   Functional Description

   Return CM communication hardware version.

   7.4.2   Parameters:

   NONE

   7.4.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.4.4   Data Structures

// 7.4.5   Code

   return opto_version();

} // end com_hwver()

