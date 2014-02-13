library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity trex_comm is
  Port
	( 
		CLK:     	in  std_logic;
        reset:    	in std_logic;
        RC_TREX: 	in  std_logic;
		TX_TREX:    out std_logic;
		count:      out std_logic_vector(30 downto 0);
		timestamp:  out std_logic_vector(30 downto 0);
		s0:         out std_logic_vector(10 downto 0);
		s1:         out std_logic_vector(10 downto 0);
		s2:         out std_logic_vector(10 downto 0);
		s3:         out std_logic_vector(10 downto 0);
		s4:         out std_logic_vector(10 downto 0);
		int:        out std_logic;
		a:	        in  std_logic_vector(7 downto 0);
		send:	    in  std_logic
	);
		
end trex_comm;

architecture Behavioral of trex_comm is

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
signal buf_s0, buf_s1, buf_s2, buf_s3, buf_s4 : std_logic_vector(10 downto 0); 
signal buf_count : std_logic_vector(30 downto 0);
signal timer, sig_timer : std_logic_vector(30 downto 0) := "0000000000000000000000000000000";

type tx_state_type is (TX_IDLE, TX_SEND);
signal tx_state : tx_state_type;
signal tx_send_buf : std_logic_vector(7 downto 0);

type rc_state_type is (RC_FIRST,RC_SECOND,RC_NEXT);
signal rc_state : rc_state_type;

begin
rsin : rs232in
generic map(clocks_per_bit    => 100)
port map(
		clk       => CLK,
		reset     => '0',
		rx        => sig_data_in,
		ready     => sig_received,
		data      => sig_receivedbyte
		);
  
rsout : rs232out
generic map(clocks_per_bit    => 100)
	port map(
		clk       => CLK,
		reset     => '0',
		data      => sig_tx_data,
		send      => sig_send,
		tx        => sig_tx,
		ready     => sig_ready,
		busy      => sig_busy
		);

sig_data_in <= RC_TREX;
TX_TREX <= sig_tx;
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
					case firstbyte(6 downto 4) is
						when "000" =>
							buf_s0 <= firstbyte(3 downto 0) & sig_receivedbyte(6 downto 0);
						when "001" =>
							buf_s1 <= firstbyte(3 downto 0) & sig_receivedbyte(6 downto 0);
						when "010" =>
							buf_s2 <= firstbyte(3 downto 0) & sig_receivedbyte(6 downto 0);
						when "011" =>
							buf_s3 <= firstbyte(3 downto 0) & sig_receivedbyte(6 downto 0);
						when "100" =>
							buf_s4 <= firstbyte(3 downto 0) & sig_receivedbyte(6 downto 0);
				
							s0 <= buf_s0;
							s1 <= buf_s1;
							s2 <= buf_s2;
							s3 <= buf_s3;
							s4 <= buf_s4;
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
				if send='1' then
					tx_state <= TX_SEND;
					sig_tx_data <= a;
				end if;
			when TX_SEND =>
				sig_send <= '1';
				if sig_busy='1' then
					tx_state <= TX_IDLE;
				end if;
		end case;
	end if;
end if;
end process;
end Behavioral;

