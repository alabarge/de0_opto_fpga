library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity com_regs is
   generic (
      C_DWIDTH             : integer   := 32;
      C_NUM_REG            : integer   := 16
   );
   port (
      clk                  : in    std_logic;
      reset_n              : in    std_logic;
      read_n               : in    std_logic;
      write_n              : in    std_logic;
      address              : in    std_logic_vector(11 downto 0);
      readdata             : out   std_logic_vector(31 downto 0);
      writedata            : in    std_logic_vector(31 downto 0);
      cpu_DIN              : in    std_logic_vector(31 downto 0);
      cpu_DOUT             : out   std_logic_vector(31 downto 0);
      cpu_ADDR             : out   std_logic_vector(11 downto 0);
      cpu_WE               : out   std_logic_vector(1 downto 0);
      cpu_RE               : out   std_logic_vector(2 downto 0);
      com_CONTROL          : out   std_logic_vector(31 downto 0);
      com_INT_REQ          : in    std_logic_vector(2 downto 0);
      com_INT_ACK          : out   std_logic_vector(2 downto 0);
      com_STATUS           : in    std_logic_vector(31 downto 0);
      com_ADR_BEG          : out   std_logic_vector(31 downto 0);
      com_ADR_END          : out   std_logic_vector(31 downto 0);
      com_PKT_CNT          : out   std_logic_vector(31 downto 0);
      com_TICKS            : out   std_logic_vector(15 downto 0);
      com_TEST_BIT         : out   std_logic
   );
end com_regs;

architecture rtl of com_regs is

--
-- CONSTANTS
--
constant C_COM_VERSION     : std_logic_vector(7 downto 0)  := X"0F";
constant C_COM_CONTROL     : std_logic_vector(31 downto 0) := X"00000000";
constant C_COM_TICKS       : std_logic_vector(15 downto 0) := X"0363";

--
-- SIGNAL DECLARATIONS
--

signal wrCE                : std_logic_vector(C_NUM_REG-1 downto 0);
signal rdCE                : std_logic_vector(C_NUM_REG-1 downto 0);

--
-- MAIN CODE
--
begin

   --
   -- COMBINATORIAL OUTPUTS
   --

   -- Read/Write BlockRAM
   cpu_DOUT             <= writedata;
   cpu_ADDR             <= address;

   cpu_WE(0)            <= '1' when (address(9)  = '1' and write_n = '0') else '0';
   cpu_WE(1)            <= '1' when (address(10) = '1' and write_n = '0') else '0';
   cpu_RE(0)            <= '1' when (address(9)  = '1' and read_n  = '0') else '0';
   cpu_RE(1)            <= '1' when (address(10) = '1' and read_n  = '0') else '0';
   cpu_RE(2)            <= '1' when (address(11) = '1' and read_n  = '0') else '0';

   --
   -- READ/WRITE REGISTER STROBES
   --
   process (all) begin
      for i in 0 to wrCE'length-1 loop
         if (address(4 downto 0) = std_logic_vector(to_unsigned(i, 5)) and
               address(11 downto 9) = "000" and write_n = '0') then
            wrCE(i) <= '1';
         else
            wrCE(i) <= '0';
         end if;
      end loop;
      for i in 0 to rdCE'length-1 loop
         if (address(4 downto 0) = std_logic_vector(to_unsigned(i, 5)) and
               address(11 downto 9) = "000" and read_n = '0') then
            rdCE(i) <= '1';
         else
            rdCE(i) <= '0';
         end if;
      end loop;
    end process;

   --
   -- WRITE REGISTERS
   --
   process (all) begin
      if (reset_n = '0') then
         com_CONTROL         <= C_COM_CONTROL;
         com_INT_ACK         <= (others => '0');
         com_ADR_BEG         <= (others => '0');
         com_ADR_END         <= (others => '0');
         com_PKT_CNT         <= (others => '0');
         com_TICKS           <= C_COM_TICKS;
         com_TEST_BIT        <= '0';
      elsif (rising_edge(clk)) then
         if (wrCE(0) = '1') then
            com_CONTROL      <= writedata;
         elsif (wrCE(2) = '1') then
            com_TEST_BIT     <= writedata(0);
         elsif (wrCE(3) = '1') then
            com_INT_ACK      <= writedata(2 downto 0);
         elsif (wrCE(5) = '1') then
            com_ADR_BEG      <= writedata;
         elsif (wrCE(6) = '1') then
            com_ADR_END      <= writedata;
         elsif (wrCE(7) = '1') then
            com_PKT_CNT      <= writedata;
         elsif (wrCE(8) = '1') then
            com_TICKS        <= writedata(15 downto 0);
         else
            com_CONTROL      <= com_CONTROL;
            com_TEST_BIT     <= com_TEST_BIT;
            com_INT_ACK      <= (others => '0');
            com_ADR_BEG      <= com_ADR_BEG;
            com_ADR_END      <= com_ADR_END;
            com_PKT_CNT      <= com_PKT_CNT;
            com_TICKS        <= com_TICKS;
         end if;
      end if;
   end process;

   --
   -- READ REGISTERS AND BLOCKRAM
   --
   process (all) begin
      if (rdCE(0) = '1') then
         readdata             <= com_CONTROL;
      elsif (rdCE(1) = '1') then
         readdata             <= X"000000" & C_COM_VERSION;
      elsif (rdCE(2) = '1') then
         readdata             <= X"0000000" & "000" & com_TEST_BIT;
      elsif (rdCE(3) = '1') then
         readdata             <= X"0000000" & '0' & com_INT_REQ;
      elsif (rdCE(4) = '1') then
         readdata             <= com_STATUS;
      elsif (rdCE(5) = '1') then
         readdata             <= com_ADR_BEG;
      elsif (rdCE(6) = '1') then
         readdata             <= com_ADR_END;
      elsif (rdCE(7) = '1') then
         readdata             <= com_PKT_CNT;
      elsif (rdCE(8) = '1') then
         readdata             <= X"0000" & com_TICKS;
      --
      -- READ BLOCKRAM
      --
      elsif (address(9)  = '1' and read_n = '0') then
         readdata             <= cpu_DIN;
      elsif (address(10) = '1' and read_n = '0') then
         readdata             <= cpu_DIN;
      elsif (address(11) = '1' and read_n = '0') then
         readdata             <= cpu_DIN;
      else
         readdata             <= (others => '0');
      end if;
   end process;

end rtl;
