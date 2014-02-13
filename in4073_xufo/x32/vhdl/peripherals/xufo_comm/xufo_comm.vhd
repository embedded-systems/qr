library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity xufo_comm is
  Port( CLK:     in  std_logic;
        reset:    in std_logic;
        RC_XUFO: in  std_logic;
	TX_XUFO:    out std_logic;
	count:      out std_logic_vector(30 downto 0);
	timestamp:  out std_logic_vector(30 downto 0);
	s0:         out std_logic_vector(17 downto 0);
	s1:         out std_logic_vector(17 downto 0);
	s2:         out std_logic_vector(17 downto 0);
	s3:         out std_logic_vector(17 downto 0);
	s4:         out std_logic_vector(17 downto 0);
	s5:         out std_logic_vector(17 downto 0);
	s6:         out std_logic_vector(17 downto 0);
        int:        out std_logic;

	A0:	        in  std_logic_vector(9 downto 0);
	A1:	        in  std_logic_vector(9 downto 0);
	A2:	        in  std_logic_vector(9 downto 0);
	A3:	        in  std_logic_vector(9 downto 0);	
	trigger:        in  std_logic );
end xufo_comm;

architecture Behavioral of xufo_comm is

  component rs232in is
    Generic(clocks_per_bit    : in positive);
    Port( clk       : in std_logic;
          reset     : in std_logic;
          rx        : in std_logic;
          ready     : out std_logic;
          data      : out std_logic_vector(7 downto 0));
  end component;
  
  component rs232out is
    Generic(clocks_per_bit    : in positive);
    Port( clk       : in std_logic;
          reset     : in std_logic;
          data      : in std_logic_vector(7 downto 0);
          send      : in std_logic;
          tx        : out std_logic;
          ready     : out std_logic;
          busy      : out std_logic);
  end component;
  
  signal sig_data_in, sig_received, sig_tx, sig_send, sig_ready, sig_busy, sig_int : std_logic;
  signal sig_receivedbyte, sig_tx_data : std_logic_vector(7 downto 0);
  signal firstbyte : std_logic_vector(7 downto 0);
  signal secondbyte : std_logic_vector(7 downto 0);
  
  signal buf_s0, buf_s1, buf_s2, buf_s3, buf_s4, buf_s5, buf_s6 : std_logic_vector(17 downto 0); 
  signal buf_count : std_logic_vector(30 downto 0);
  signal timer, sig_timer : std_logic_vector(30 downto 0) := "0000000000000000000000000000000";
  
  type tx_state_type is (TX_IDLE, TX_SEND,TX_DELAY);
  signal tx_state : tx_state_type;
  signal tx_send_buf : std_logic_vector(7 downto 0);
  
  type COMNDARRAY is array (11 downto 0) of std_logic_vector (7 downto 0);
  signal commands : COMNDARRAY;
  signal delay_timer: std_logic_vector (31 downto 0);
  signal pos : integer := 0;
  
  
  type rc_state_type is (RC_FIRST,RC_SECOND,RC_THIRD,RC_NEXT);
  signal rc_state : rc_state_type;
 
begin
  rsin : rs232in
    generic map(
      clocks_per_bit    => 100)
    port map(
      clk       => CLK,
      reset     => '0',
      rx        => sig_data_in,
      ready     => sig_received,
      data      => sig_receivedbyte);
      
  rsout : rs232out
    generic map(
      clocks_per_bit    => 100)
    port map(
      clk       => CLK,
      reset     => reset,
      data      => sig_tx_data,
      send      => sig_send,
      tx        => sig_tx,
      ready     => sig_ready,
      busy      => sig_busy);
   
  sig_data_in <= RC_XUFO;
  TX_XUFO <= sig_tx;
  int <= sig_int;



  process(CLK,sig_received,sig_receivedbyte,firstbyte,buf_s1)
  begin
    if rising_edge(CLK) then
      timer <= timer+1;
      timestamp <= sig_timer;
      count <= buf_count;

      if reset='1' then
        rc_state <= RC_FIRST;
        tx_state <= TX_IDLE;
        sig_int <= '0';
        delay_timer <= X"00000000";
	pos <= 0; 
      else

        if sig_received='1' then
          case rc_state is
            when RC_FIRST =>
              if sig_receivedbyte(7)='1' then
                firstbyte <= sig_receivedbyte;
                rc_state <= RC_SECOND;
              end if;
            when RC_SECOND =>
              if sig_receivedbyte(7)='0' then
                secondbyte <= sig_receivedbyte;
                rc_state <= RC_THIRD;
              end if;
            when RC_THIRD =>
              if sig_receivedbyte(7)='0' then
                case firstbyte(6 downto 4) is
                  when "000" =>
                    buf_s0 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "001" =>
                    buf_s1 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "010" =>
                    buf_s2 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "011" =>
                    buf_s3 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "100" =>
                    buf_s4 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "101" =>
                    buf_s5 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                  when "110" =>
                    buf_s6 <= firstbyte(3 downto 0) & secondbyte(6 downto 0) & sig_receivedbyte(6 downto 0);
                    s0 <= buf_s0;
                    s1 <= buf_s1;
                    s2 <= buf_s2;
                    s3 <= buf_s3;
                    s4 <= buf_s4;
                    s5 <= buf_s5;
                    s6 <= buf_s6;
                    buf_count <= buf_count+1;
                    sig_timer <= timer;
                    sig_int <= '1';
                  when others =>
                end case;
                rc_state <= RC_NEXT;
              else
                rc_state <= RC_NEXT;
              end if;
            when others =>
          end case;
        else
          sig_int <= '0';
          case rc_state is
            when RC_NEXT =>
              rc_state <= RC_FIRST;
            when others =>
          end case;
        end if;

        case tx_state is
          when TX_IDLE =>
            sig_send <= '0'; 
            if trigger = '1' then
		commands(0) <= "10000000";
		commands(1) <= "00000" & A0(9 downto 7);
		commands(2) <= '0' & A0(6 downto 0);
		
		commands(3) <= "10000001";
		commands(4) <= "00000" & A1(9 downto 7);
		commands(5) <= '0' & A1(6 downto 0);
		
		commands(6) <= "10000010";
		commands(7) <= "00000" & A2(9 downto 7);
		commands(8) <= '0' & A2(6 downto 0);

		commands(9) <= "10000011";
		commands(10) <= "00000" & A3(9 downto 7);
		commands(11) <= '0' & A3(6 downto 0);
 
              tx_state <= TX_SEND;
            end if;
            
          when TX_SEND =>
            if sig_busy='1' then
              tx_state <= TX_SEND;
            else
              sig_tx_data <= commands(pos);
              sig_send <= '1';
              tx_state <= TX_DELAY;
              delay_timer <= X"00000000";              
            end if;
          when TX_DELAY =>
            sig_send <= '0';
            if(delay_timer < 3000 ) then 
              delay_timer <= delay_timer + X"00000001";
            else
              delay_timer <= X"00000000";

              pos <= pos + 1;              
              if(pos > 10) then
                pos <= 0;
                tx_state <= TX_IDLE;
              else
                tx_state <= TX_SEND;
              end if;

            end if;
            
        end case;
 
      end if;

     
    end if;
  end process;
  

end Behavioral;

