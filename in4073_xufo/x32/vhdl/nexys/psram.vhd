library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_ARITH.all;
use IEEE.STD_LOGIC_UNSIGNED.all;
use work.all;

---- Uncomment the following library declaration if instantiating
---- any Xilinx primitives in this code.
library UNISIM;
use UNISIM.VComponents.all;

entity psram is
  generic
    (
      C_PSRAM_DQ_WIDTH : integer                   := 16;
      C_PSRAM_A_WIDTH  : integer                   := 23;
      C_PSRAM_LATENCY  : integer range 0 to 7      := 3;
      C_DRIVE_STRENGTH : integer range 0 to 3      := 1);
  port
    (
        -- system clock
        clk                         : in    std_logic;
        -- system reset
        reset                       : in    std_logic;
        
        -- memory address (unaligned)
        mem_address                 : in    std_logic_vector(23 downto 0);
        -- data to memory
        mem_data_in                 : in    std_logic_vector(31 downto 0);
        -- data from memory
        mem_data_out                : out   std_logic_vector(31 downto 0);
        -- size of data (see types.vhd)
        mem_data_size               : in    std_logic_vector(2 downto 0);
        -- data signedness (1=signed, 0=unsigned)
        mem_data_signed             : in    std_logic;
        -- start read operation (high = start read)
        mem_read                    : in    std_logic;
        -- start write operation (high = start write)
        mem_write                   : in    std_logic;
        -- pulses high when the operation finishes
        mem_ready                   : out   std_logic;
        overflow                    : out   std_logic;

      -- psram
      PSRAM_Mem_CLK_O : out std_logic;

      PSRAM_Mem_DQ   : inout std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);

      PSRAM_Mem_A    : out std_logic_vector(C_PSRAM_A_WIDTH-1 downto 0);
      PSRAM_Mem_BE   : out std_logic_vector(C_PSRAM_DQ_WIDTH/8-1 downto 0);
      PSRAM_Mem_WE   : out std_logic;
      PSRAM_Mem_OEN  : out std_logic;
      PSRAM_Mem_CEN  : out std_logic;
      PSRAM_Mem_ADV  : out std_logic;
      PSRAM_Mem_wait : in  std_logic;
      PSRAM_Mem_CRE  : out std_logic);


end psram;

architecture Behavioral of psram is
  component psram_clk_iob
    port (
      clk    : in  std_logic;
      clk_en : in  std_logic;
      clk_q  : out std_logic);
  end component;

  component psram_data_iob
    port (
      iff_d   : in  std_logic;
      iff_q   : out std_logic;
      iff_clk : in  std_logic;
      off_d   : in  std_logic;
      off_q   : out std_logic;
      off_clk : in  std_logic);
  end component;

  component psram_off_iob
    port (
      off_d   : in  std_logic;
      off_q   : out std_logic;
      off_clk : in  std_logic);
  end component;

  component psram_wait_iob
    port (
      iff_d   : in  std_logic;
      iff_q   : out std_logic;
      iff_clk : in  std_logic;
      iff_en  : in  std_logic);
  end component;

  component psram_controller
    generic (
      C_PSRAM_DQ_WIDTH : integer;
      C_PSRAM_A_WIDTH  : integer;
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
      PSRAM_Mem_CLK_EN    : out std_logic;
      PSRAM_Mem_DQ_I_int  : in  std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
      PSRAM_Mem_DQ_O_int  : out std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
      PSRAM_Mem_DQ_OE_int : out std_logic;
      PSRAM_Mem_A_int     : out std_logic_vector(C_PSRAM_A_WIDTH-1 downto 0);
      PSRAM_Mem_BE_int    : out std_logic_vector(C_PSRAM_DQ_WIDTH/8-1 downto 0);
      PSRAM_Mem_WE_int    : out std_logic;
      PSRAM_Mem_OEN_int   : out std_logic;
      PSRAM_Mem_CEN_int   : out std_logic;
      PSRAM_Mem_ADV_int   : out std_logic;
      PSRAM_Mem_WAIT_int  : in  std_logic;
      PSRAM_Mem_CRE_int   : out std_logic);
  end component;


  -- internal Signals
  signal PSRAM_Mem_CLK_EN    : std_logic;
  signal PSRAM_Mem_DQ_I      : std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
  signal PSRAM_Mem_DQ_O      : std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
  signal PSRAM_Mem_DQ_T      : std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
  signal PSRAM_Mem_DQ_I_int  : std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
  signal PSRAM_Mem_DQ_O_int  : std_logic_vector(C_PSRAM_DQ_WIDTH-1 downto 0);
  signal PSRAM_Mem_DQ_OE_int : std_logic;
  signal PSRAM_Mem_A_int     : std_logic_vector(C_PSRAM_A_WIDTH-1 downto 0);
  signal PSRAM_Mem_BE_int    : std_logic_vector(C_PSRAM_DQ_WIDTH/8-1 downto 0);
  signal PSRAM_Mem_WE_int    : std_logic;
  signal PSRAM_Mem_OEN_int   : std_logic;
  signal PSRAM_Mem_CEN_int   : std_logic;
  signal PSRAM_Mem_ADV_int   : std_logic;
  signal PSRAM_Mem_WAIT_int  : std_logic;
  signal PSRAM_Mem_CRE_int   : std_logic;

  signal OFF_Clk : std_logic;
  signal wait_en : std_logic;

  
begin


-------------------------------------------------------------------------------
  
  -- psram memory clock
  psram_clk_iob_1 : psram_clk_iob
    port map (
      clk    => clk,
      clk_en => PSRAM_Mem_CLK_EN,
      clk_q  => PSRAM_Mem_CLK_O);
 
  -- clock for output FFs
  OFF_CLK <= not clk;

  -- tristate outputs
  tri_gen : for n in 0 to C_PSRAM_DQ_WIDTH-1 generate
      PSRAM_Mem_DQ(n)   <= PSRAM_Mem_DQ_O(n) when PSRAM_Mem_DQ_T(n) = '1' else 'Z';
      PSRAM_Mem_DQ_I(n) <= PSRAM_Mem_DQ(n);
  end generate tri_gen;

  -- data in/out FFs
  u1 : for i in 0 to C_PSRAM_DQ_WIDTH-1 generate
    psram_dq_iob_1 : psram_data_iob
      port map (
        iff_d   => PSRAM_Mem_DQ_I(i),
        iff_q   => PSRAM_Mem_DQ_I_int(i),
        iff_clk => clk,
        off_d   => PSRAM_Mem_DQ_O_int(i),
        off_q   => PSRAM_Mem_DQ_O(i),
        off_clk => OFF_clk);

  -- tristate out enable FF
    psram_a_off_1 : psram_off_iob
      port map (
        off_d   => PSRAM_Mem_DQ_OE_int,
        off_q   => PSRAM_Mem_DQ_T(i),
        off_clk => OFF_clk);

  end generate u1;

  -- address output FFs
  u2 : for i in 0 to C_PSRAM_A_WIDTH-1 generate
    psram_a_off_1 : psram_off_iob
      port map (
        off_d   => PSRAM_Mem_A_int(i),
        off_q   => PSRAM_Mem_A(i),
        off_clk => OFF_Clk);
  end generate u2;

  -- byte enable outpuf FFs
  u3 : for i in 0 to C_PSRAM_DQ_WIDTH/8-1 generate
    psram_be_off_1 : psram_off_iob
      port map (
        off_d   => PSRAM_Mem_BE_int(i),
        off_q   => PSRAM_Mem_BE(i),
        off_clk => OFF_Clk);
  end generate u3;

  psram_we_off_1 : psram_off_iob
    port map (
      off_d   => PSRAM_Mem_WE_int,
      off_q   => PSRAM_Mem_WE,
      off_clk => OFF_Clk);  

  psram_oen_off_1 : psram_off_iob
    port map (
      off_d   => PSRAM_Mem_OEN_int,
      off_q   => PSRAM_Mem_OEN,
      off_clk => OFF_Clk);    

  psram_cen_off_1 : psram_off_iob
    port map (
      off_d   => PSRAM_Mem_CEN_int,
      off_q   => PSRAM_Mem_CEN,
      off_clk => OFF_Clk);

  psram_adv_off_1 : psram_off_iob
    port map (
      off_d   => PSRAM_Mem_ADV_int,
      off_q   => PSRAM_Mem_ADV,
      off_clk => OFF_Clk);  

  psram_cre_off_1 : psram_off_iob
    port map (
      off_d   => PSRAM_Mem_CRE_int,
      off_q   => PSRAM_Mem_CRE,
      off_clk => OFF_Clk);

  process(OFF_Clk)
  begin
    if rising_edge(OFF_Clk) then
      if ((PSRAM_Mem_ADV_int = '0') or (PSRAM_Mem_CEN_int = '1'))then
        wait_en <= '1';
      else
        wait_en <= '0';
      end if;

    end if;
  end process;

  psram_wait_iob_1 : psram_wait_iob
    port map (
      iff_d   => PSRAM_Mem_WAIT,
      iff_q   => PSRAM_Mem_WAIT_int,
      iff_clk => clk,
      iff_en  => wait_en);

  
  psram_1 : psram_controller
    generic map (
      C_PSRAM_DQ_WIDTH => C_PSRAM_DQ_WIDTH,
      C_PSRAM_A_WIDTH  => C_PSRAM_A_WIDTH,
      C_PSRAM_LATENCY  => C_PSRAM_LATENCY,
      C_DRIVE_STRENGTH => C_DRIVE_STRENGTH)
    port map (
      clk                 => clk,
      reset               => reset,
      mem_address         => mem_address,
      mem_data_in         => mem_data_in,
      mem_data_out        => mem_data_out,
      mem_data_size       => mem_data_size,
      mem_data_signed     => mem_data_signed,
      mem_read            => mem_read,
      mem_write           => mem_write,
      mem_ready           => mem_ready,
      overflow            => overflow,
      PSRAM_Mem_CLK_EN    => PSRAM_Mem_CLK_EN,
      PSRAM_Mem_DQ_I_int  => PSRAM_Mem_DQ_I,--_int,
      PSRAM_Mem_DQ_O_int  => PSRAM_Mem_DQ_O_int,
      PSRAM_Mem_DQ_OE_int => PSRAM_Mem_DQ_OE_int,
      PSRAM_Mem_A_int     => PSRAM_Mem_A_int,
      PSRAM_Mem_BE_int    => PSRAM_Mem_BE_int,
      PSRAM_Mem_WE_int    => PSRAM_Mem_WE_int,
      PSRAM_Mem_OEN_int   => PSRAM_Mem_OEN_int,
      PSRAM_Mem_CEN_int   => PSRAM_Mem_CEN_int,
      PSRAM_Mem_ADV_int   => PSRAM_Mem_ADV_int,
      PSRAM_Mem_WAIT_int  => PSRAM_Mem_WAIT,--_int,
      PSRAM_Mem_CRE_int   => PSRAM_Mem_CRE_int);

end Behavioral;

