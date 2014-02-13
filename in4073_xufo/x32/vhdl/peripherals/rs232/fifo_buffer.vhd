library ieee;
use ieee.std_logic_1164.all;

entity fifo_buffer is
	generic (
		item_size		: in  positive;
		item_count	: in  positive
	);
	port (
		clk					: in  std_logic;
		reset				: in  std_logic;
		
		data_in			: in  std_logic_vector(item_size-1 downto 0);
		data_out		: out std_logic_vector(item_size-1 downto 0);
		
		push				: in  std_logic;
		pop					: in  std_logic;
		buffer_full : out std_logic;
		data_ready	: out std_logic
	);
end entity;

architecture behaviour of fifo_buffer is
	type ARRAY_TYPE is array (item_count-1 downto 0) of 
		std_logic_vector(item_size-1 downto 0);
	
	signal buff   			: ARRAY_TYPE;
	signal valid				: std_logic_vector(item_count-1 downto 0);
	
begin
	buffer_full <= valid(0);
	data_ready <= valid(item_count-1);
	data_out <= buff(item_count-1);

	process(clk, reset, valid, buff, push, pop) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				valid <= (others => '0');
			else
				
				for i in 0 to item_count-1 loop
					if (valid(i) = '0') then
						-- the current value is not valid, grab data from the
						--  previous value in the buffer
						if (i = 0) then
							valid(i) <= push;
							buff(i) <= data_in;
						else
							valid(i) <= valid(i-1);
							buff(i) <= buff(i-1);
						end if;
					else
						-- the current value is valid, if the next value is
						--  not valid, the current value will shift and will
						--  become invalid
						if (i = item_count-1) then
							valid(i) <= not pop;
						else
							valid(i) <= valid(i+1);
						end if;
						buff(i) <= buff(i);
					end if;
				end loop;
			end if;
		end if;
	end process;
	
end architecture;
