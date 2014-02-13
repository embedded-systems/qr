library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_ARITH.all;
use IEEE.STD_LOGIC_UNSIGNED.all;
use work.types.all;


entity psram_controller is
  generic (
    C_PSRAM_DQ_WIDTH : integer                       := 16;
    C_PSRAM_A_WIDTH  : integer                       := 23;
    C_PSRAM_LATENCY  : integer range 0 to 7      := 3;
    C_DRIVE_STRENGTH : integer range 0 to 3      := 1);
  port (
    clk                         : in    std_logic;
    reset                       : in    std_logic;
    
    mem_address                 : in    std_logic_vector(23 downto 0);
    mem_data_in                 : in    std_logic_vector(31 downto 0);
    mem_data_out                : out   std_logic_vector(31 downto 0);
    mem_data_size               : in    std_logic_vector(2 downto 0);
    mem_data_signed             : in    std_logic;
    mem_read                    : in    std_logic;
    mem_write                   : in    std_logic;
    mem_ready                   : out   std_logic;
    overflow                    : out   std_logic;
    -- 
    PSRAM_Mem_CLK_EN    : out std_logic;
    PSRAM_Mem_DQ_I_int  : in  std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
    PSRAM_Mem_DQ_O_int  : out std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
    PSRAM_Mem_DQ_OE_int : out std_logic;
    PSRAM_Mem_A_int     : out std_logic_vector(C_PSRAM_A_WIDTH-1 downto 0);
    PSRAM_Mem_BE_int    : out std_logic_vector(C_PSRAM_DQ_WIDTH/8-1 downto 0);
    PSRAM_Mem_WE_int    : out std_logic;
    PSRAM_Mem_OEN_int   : out std_logic;
    PSRAM_Mem_CEN_int   : out std_logic := '1';
    PSRAM_Mem_ADV_int   : out std_logic := '1';
    PSRAM_Mem_WAIT_int  : in  std_logic;
    PSRAM_Mem_CRE_int   : out std_logic);

end psram_controller;

architecture rtl of psram_controller is
  type state_t is (startup, config,
                   start_wait_ready,
                   start_write_pulse,
                   idle,
                   wr_setup, wr_1, wr_2, wr_3, wr_restart, wr_finish, wr_end,
                   rd_wait_ready, rd_1, rd_2, rd_3, rd_restart, rd_finish); 
  signal state      : state_t := startup;
  signal nextstate : integer range 1 to 3;
  signal cnt        : integer range 0 to 7500;
  signal is_32, is_16, is_8, is_valid : std_logic;
  signal overflow_16, overflow_8 : std_logic;
  signal overflow_16_signed, overflow_8_signed : std_logic;

  -- Sync burst acess mode[BCR[15],all other values default
  constant C_BCR_CONFIG     : std_logic_vector(15 downto 0) := ("00" &
                                                               conv_std_logic_vector( C_PSRAM_LATENCY,3) &
                                                               "10100" &
                                                               conv_std_logic_vector( C_DRIVE_STRENGTH,2) &
                                                               "1111");
  
begin  -- rtl
  is_32 <= 
      '1' when mem_data_size = VARTYPE_PTR else
      '1' when mem_data_size = VARTYPE_LNG else
      '1' when mem_data_size = VARTYPE_INT else
      '0';
  is_16 <=
      '1' when mem_data_size = VARTYPE_SHORT else
      '1' when mem_data_size = VARTYPE_INSTRUCTION else
      '0';
  is_8 <=
      '1' when mem_data_size = VARTYPE_CHAR else
      '0';
  is_valid <=
      '1' when is_32 = '1' or is_16 = '1' or is_8 = '1' else
      '0';

  overflow_16_signed <= '0' when mem_data_in(15) = mem_data_in(16) else '1';
  overflow_16 <= 
          '0' when mem_data_in(31 downto 16) = x"0000" else
          '0' when mem_data_in(31 downto 16) = x"ffff" else
          '1';

  overflow_8_signed <= '0' when mem_data_in(7) = mem_data_in(8) else '1';
  overflow_8 <= 
          '0' when mem_data_in(31 downto 8) = x"000000" else
          '0' when mem_data_in(31 downto 8) = x"ffffff" else
          '1';

  overflow <= ((overflow_16 and is_16) or (overflow_8 and is_8) or
          (overflow_16_signed and mem_data_signed and is_16) or
          (overflow_8_signed and mem_data_signed and is_8)) and mem_write;
  process (reset, Clk)
  begin
    if (reset = '1') then
      PSRAM_Mem_DQ_O_int  <= (others => '0');
      PSRAM_Mem_DQ_OE_int <= '0';       -- oe disable
      PSRAM_Mem_A_int     <= (others => '0');
      PSRAM_Mem_BE_int    <= (others => '1');
      PSRAM_Mem_WE_int    <= '1';
      PSRAM_Mem_OEN_int   <= '1';
      PSRAM_Mem_CEN_int   <= '1';
      PSRAM_Mem_ADV_int   <= '1';
      PSRAM_Mem_CRE_int   <= '0';
      PSRAM_Mem_CLK_EN    <= '0';
      state               <= startup;
    elsif (clk'event and clk = '1') then
      case state is
        when startup =>
          PSRAM_Mem_DQ_O_int  <= (others => '0');
          PSRAM_Mem_DQ_OE_int <= '0';       -- oe disable
          PSRAM_Mem_A_int     <= (others => '0');
          PSRAM_Mem_BE_int    <= (others => '1');
          PSRAM_Mem_WE_int    <= '1';
          PSRAM_Mem_OEN_int   <= '1';
          PSRAM_Mem_CEN_int   <= '1';
          PSRAM_Mem_ADV_int   <= '1';
          PSRAM_Mem_CRE_int   <= '0';
          PSRAM_Mem_CLK_EN    <= '0';
          cnt <= 7500;
          state <= config;

        when config =>
          if cnt = 0 then

            -- write BCR Register
            PSRAM_Mem_A_int   <= "000" & "10" & "00" & C_BCR_CONFIG;
            PSRAM_Mem_ADV_int <= '0';     -- adress strobe
            PSRAM_Mem_CEN_int <= '0';     -- chip enable
            PSRAM_Mem_CRE_int <= '1';
            state             <= start_wait_ready;
          else
            cnt <= cnt - 1;
            state <= config;
          end if;


        when start_wait_ready =>
          PSRAM_Mem_ADV_int <= '1';     -- adress strobe
          cnt               <= 5;
          state             <= start_write_pulse;

        when start_write_pulse =>
          PSRAM_Mem_A_int   <= (others => '0');
          PSRAM_Mem_CRE_int <= '0';     -- normal operation
          PSRAM_Mem_WE_int  <= '0';     -- write operation          
          if (cnt = 0) then
            PSRAM_Mem_WE_int  <= '1';   -- write operation
            PSRAM_Mem_CEN_int <= '1';   -- chip enable
            PSRAM_Mem_CLK_EN  <= '1';
            state             <= idle;
          else
            cnt   <= cnt -1;
            state <= start_write_pulse;
          end if;



        when idle =>
          --mem_data_out      <= (others => '0');
          cnt <= 2;
          mem_ready <= '0';
          PSRAM_Mem_DQ_OE_int <= '0';
          if mem_write = '1' and is_valid = '1' then
            PSRAM_Mem_CRE_int <= '0';              -- normal operation
            PSRAM_Mem_A_int   <= mem_address(23 downto 1);
            PSRAM_Mem_ADV_int <= '0';              -- adress strobe
            PSRAM_Mem_CEN_int <= '0';              -- chip enable
            PSRAM_Mem_WE_int  <= '0';              -- write operation
            state             <= wr_setup;
            nextstate         <= 1;
          elsif mem_read = '1' and is_valid = '1' then
            PSRAM_Mem_CRE_int <= '0';              -- normal operation
            PSRAM_Mem_A_int   <= mem_address(23 downto 1);
            PSRAM_Mem_BE_int(0) <= '0';
            PSRAM_Mem_BE_int(1) <= '0';
            PSRAM_Mem_ADV_int <= '0';              -- adress strobe
            PSRAM_Mem_CEN_int <= '0';              -- chip enable
            PSRAM_Mem_WE_int  <= '1';              -- read operation
            state             <= rd_wait_ready;
            nextstate         <= 1;
          elsif (mem_read = '1' or mem_write = '1') and is_valid = '0' then
            -- ignore invalid read/writes
            mem_ready <= '1';
            state <= rd_finish; -- misuse rd_finish here
          else
            state <= idle;
          end if;

          ---------------------------------------------------------------------
          -- write
        when wr_setup =>
          PSRAM_Mem_ADV_int   <= '1';   -- remove adress strobe
          PSRAM_Mem_A_int     <= (others => '0');
          if nextstate = 1 then
              state <= wr_1;
          elsif nextstate = 2 then
              state <= wr_2;
          else
              state <= wr_3;
          end if;

        when wr_1 =>
          if PSRAM_Mem_WAIT_int = '0' then
            PSRAM_Mem_DQ_OE_int <= '1'; 
            if is_8 = '1' and  mem_address(0) = '0' then
              PSRAM_Mem_DQ_O_int(15 downto 8)   <= mem_data_in(7 downto 0);
              PSRAM_Mem_BE_int(0) <= '1';
              PSRAM_Mem_BE_int(1) <= '0';
            elsif is_8 = '1' and mem_address(0) = '1' then
              PSRAM_Mem_DQ_O_int(7 downto 0)    <= mem_data_in(7 downto 0);
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '1';
            elsif is_16 = '1' and mem_address(0) = '0' then
              PSRAM_Mem_DQ_O_int                <= mem_data_in(15 downto 0);
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '0';
            elsif is_16 = '1' and mem_address(0) = '1' then
              PSRAM_Mem_DQ_O_int(7 downto 0)    <= mem_data_in(15 downto 8);
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '1';
            elsif is_32 = '1' and mem_address(0) = '0' then
              PSRAM_Mem_DQ_O_int                <= mem_data_in(31 downto 16);
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '0';
            elsif is_32 = '1' and mem_address(0) = '1' then
              PSRAM_Mem_DQ_O_int(7 downto 0)    <= mem_data_in(31 downto 24);
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '1';
            end if;

            if is_8 = '1' or (is_16 = '1' and mem_address(0) = '0') then
              state <= wr_finish;
            else
              state <= wr_2;
            end if;
          else
            state <= wr_1;
          end if;

        when wr_2 =>
          if PSRAM_Mem_WAIT_int = '0' then
            PSRAM_Mem_DQ_OE_int <= '1'; 
            if is_16 = '1' then
              PSRAM_Mem_BE_int(0) <= '1';
              PSRAM_Mem_BE_int(1) <= '0';
              PSRAM_Mem_DQ_O_int(15 downto 8)  <= mem_data_in(7 downto 0);
            elsif is_32 = '1' and mem_address(0) = '0' then
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '0';
              PSRAM_Mem_DQ_O_int  <= mem_data_in(15 downto 0);
            elsif is_32 = '1' and mem_address(0) = '1' then
              PSRAM_Mem_BE_int(0) <= '0';
              PSRAM_Mem_BE_int(1) <= '0';
              PSRAM_Mem_DQ_O_int  <= mem_data_in(23 downto 8);
            end if;

            if is_16 = '1' or (is_32 = '1' and mem_address(0) = '0') then
              state <= wr_finish;
            else
              state <= wr_3;
            end if;
          else
            state <= wr_2;
          end if;

        when wr_3 =>
          if PSRAM_Mem_WAIT_int = '0' then
            PSRAM_Mem_DQ_OE_int <= '1'; 
            PSRAM_Mem_BE_int(0) <= '1';
            PSRAM_Mem_BE_int(1) <= '0';
            PSRAM_Mem_DQ_O_int(15 downto 8)  <= mem_data_in(7 downto 0);
            state <= wr_finish;
          else
            -- If wait is low, we need to differentiate between a row boundary
            -- crossing, and a restarted write. In the first case, we
            -- re-initiate the write. In the second case, we just wait for
            -- wait to go high.
            if nextstate = 1 then
              -- We crossed the row boundary, re-initiate last write
              PSRAM_Mem_DQ_OE_int <= '0';
              PSRAM_Mem_CEN_int   <= '1';
              PSRAM_Mem_BE_int    <= (others => '1'); 
              PSRAM_Mem_WE_int    <= '1';
              PSRAM_Mem_A_int <= (mem_address(23 downto 1) + 1);
              nextstate <= 2;
              state <= wr_restart;
            else
              state <= wr_3;
            end if;
          end if;


        when wr_finish =>
          PSRAM_Mem_DQ_OE_int <= '0';  -- dq output disable
          PSRAM_Mem_CEN_int   <= '1';  -- chip disable
          PSRAM_Mem_BE_int    <= (others => '1');
          PSRAM_Mem_WE_int    <= '1';
          if PSRAM_Mem_WAIT_int = '0' then
            mem_ready <= '1';
            state               <= wr_end;
          else
            -- We crossed a row boundary on the last write, so we must 
            -- re-initiate it. If we are writing a 32 bits value to an
            -- odd address, the state before this one was wr_3. Other-
            -- wise the address was either even, or it was a 16 bit
            -- value and we need to go back to wr_2.
            if is_32 = '1' and mem_address(0) = '1' then
              PSRAM_Mem_A_int <= (mem_address(23 downto 1) + 2);
              nextstate <= 3;
            else
              PSRAM_Mem_A_int <= (mem_address(23 downto 1) + 1);
              nextstate <= 2;
            end if;
            state <= wr_restart;
          end if;

        when wr_end =>
          mem_ready <= '0';
          state <= idle;

        when wr_restart =>
          PSRAM_Mem_ADV_int <= '0';
          PSRAM_Mem_CEN_int <= '0';
          PSRAM_Mem_WE_int  <= '0';
          state             <= wr_setup;

          ---------------------------------------------------------------------
          -- read
        when rd_wait_ready =>
          PSRAM_Mem_ADV_int <= '1';
          PSRAM_Mem_A_int   <= (others => '0');
          PSRAM_Mem_OEN_int <= '0'; -- enable outputs

          if (cnt = 0 and PSRAM_Mem_WAIT_int = '0') then
            if nextstate = 1 then
              state <= rd_1;
            elsif nextstate = 2 then
              state <= rd_2;
            else
              state <= rd_3;
            end if;
          else
            if not (cnt = 0) then
              cnt <= cnt - 1;
            end if;
            state <= rd_wait_ready;
          end if;

        when rd_1 =>
          -- route data to correct output positions
          if is_8 = '1' and mem_address(0) = '0' then
            mem_data_out(31 downto 8)   <= (others => (mem_data_signed and PSRAM_Mem_DQ_I_int(15)));
            mem_data_out(7 downto 0)    <= PSRAM_Mem_DQ_I_int(15 downto 8);
          elsif is_8 = '1' and mem_address(0) = '1' then
            mem_data_out(31 downto 8)   <= (others => (mem_data_signed and PSRAM_Mem_DQ_I_int(7)));
            mem_data_out(7 downto 0)    <= PSRAM_Mem_DQ_I_int(7 downto 0);
          elsif is_16 = '1' and mem_address(0) = '0' then
            mem_data_out(31 downto 16)  <= (others => (mem_data_signed and PSRAM_Mem_DQ_I_int(15)));
            mem_data_out(15 downto 0)   <= PSRAM_Mem_DQ_I_int;
          elsif is_16 = '1' and mem_address(0) = '1' then
            mem_data_out(31 downto 16)  <= (others => (mem_data_signed and PSRAM_Mem_DQ_I_int(7)));
            mem_data_out(15 downto 8)   <= PSRAM_Mem_DQ_I_int(7 downto 0);
          elsif is_32 = '1' and mem_address(0) = '0' then
            mem_data_out(31 downto 16)  <= PSRAM_Mem_DQ_I_int;
          elsif is_32 = '1' and mem_address(0) = '1' then
            mem_data_out(31 downto 24)  <= PSRAM_Mem_DQ_I_int(7 downto 0);
          end if;

          -- determine next state
          if is_8 = '1' or (is_16 = '1' and mem_address(0) = '0') then
            PSRAM_Mem_CEN_int <= '1'; -- disable the chip
            PSRAM_Mem_OEN_int <= '1';
            PSRAM_Mem_BE_int <= (others => '1');
            mem_ready <= '1';
            state <= rd_finish;
          else
            if PSRAM_Mem_WAIT_int = '1' then
              -- row boundary crossing
              PSRAM_Mem_A_int <= (mem_address(23 downto 1) + 1);
              PSRAM_Mem_CEN_int <= '1'; -- disable chip
              PSRAM_Mem_OEN_int <= '1';
              PSRAM_Mem_BE_int <= (others => '1');
              nextstate <= 2;
              state <= rd_restart;
            else
              state <= rd_2;
            end if;
          end if;
        
        when rd_2 =>
          -- route data
          if is_16 = '1' then
            mem_data_out(7 downto 0) <= PSRAM_Mem_DQ_I_int(15 downto 8);
          elsif is_32 = '1' then
            if mem_address(0) = '0' then
              mem_data_out(15 downto 0) <= PSRAM_Mem_DQ_I_int;
            else
              mem_data_out(23 downto 8) <= PSRAM_Mem_DQ_I_int;
            end if;
          end if;

          -- determine next state
          if is_16 = '1' or (is_32 = '1' and mem_address(0) = '0') then
            mem_ready <= '1';
            PSRAM_Mem_CEN_int <= '1';              -- chip disable
            PSRAM_Mem_OEN_int <= '1';              -- chip disable
            PSRAM_Mem_BE_int  <= (others => '1');  -- byte disable
            state             <= rd_finish;
          else
            if PSRAM_Mem_WAIT_int ='1' then
              -- row boundary crossing: restart read
              PSRAM_Mem_A_int <= (mem_address(23 downto 1) + 2);
              PSRAM_Mem_CEN_int <= '1'; -- disable chip
              PSRAM_Mem_OEN_int <= '1';
              PSRAM_Mem_BE_int <= (others => '1');
              nextstate <= 3;
              state <= rd_restart;
            else
              state <= rd_3;
            end if;
          end if;

        when rd_3 =>
          mem_data_out(7 downto 0) <= PSRAM_Mem_DQ_I_int(15 downto 8);
          mem_ready <= '1';
          PSRAM_Mem_CEN_int <= '1';              -- chip disable
          PSRAM_Mem_OEN_int <= '1';              -- chip disable
          PSRAM_Mem_BE_int  <= (others => '1');  -- byte disable
          state             <= rd_finish;

        when rd_finish =>
          -- CPU drops read ctrl signal after mem_ready pulses, so we
          -- need to wait, otherwise a spurious read will occur.
          mem_ready <= '0';
          state <= idle;

        when rd_restart =>
          -- Restart after row boundary crossing. Address is already set.
          PSRAM_Mem_BE_int(0) <= '0';
          PSRAM_Mem_BE_int(1) <= '0';
          PSRAM_Mem_ADV_int <= '0';
          PSRAM_Mem_CEN_int <= '0';
          cnt <= 2;
          state <= rd_wait_ready;
          
        when others =>
          state <= startup;
      end case;
    end if;
  end process;
  

end rtl;
-- vim:ts=2:sts=2:sw=2
