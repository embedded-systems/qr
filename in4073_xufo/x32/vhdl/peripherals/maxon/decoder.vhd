---------------------------------------------------------------------------
-- decoder 
-- decode Maxon encoder signals A, B
-- signals error condition through led-compatible one shot 
--
-- Author: Arjan van Gemund
---------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Sijmen edit: changed name from decoder to maxdecoder
entity maxdecoder is
  port(
    clk: in std_logic;
    reset: in std_logic;
    a: in std_logic;
    b: in std_logic;
    up: out std_logic;
    dn: out std_logic;
    err: out std_logic
  );
end maxdecoder;



architecture behavior of maxdecoder is


type statetype is (init, s00, s01, s10, s11, error);
signal state: statetype := init;
signal trigger: std_logic;

begin

  -- counting FSM

  process
    begin
      wait until rising_edge(clk);
      case state is

        when init => -- goto current state w/o error
	  up <= '0';
	  dn <= '0';
	  trigger <= '0';
          if (a = '0' and b = '0') then
	    state <= s00;
          end if;
          if (a = '0' and b = '1') then
	    state <= s01;
          end if;
          if (a = '1' and b = '0') then
	    state <= s10;
          end if;
          if (a = '1' and b = '1') then
	    state <= s11;
          end if;

        when s00 =>
	  if (reset = '1') then
	    state <= init;
	  end if;
	  if (a = '0' and b = '1') then
	    dn <= '0'; -- came from 10 - (dn <= 1) - 00 so reset dn
	    state <= s01;
	  end if;
	  if (a = '1' and b = '0') then
	    up <= '0'; -- came from 01 - (up <= 1) - 00 so reset up
	    state <= s10;
	  end if;
	  if (a = '1' and b = '1') then
	    up <= '0';
	    dn <= '0';
	    state <= error;
	end if;
  
        when s01 =>
	  if (reset = '1') then
	    state <= init;
	  end if;
	  if (a = '0' and b = '0') then
	    up <= '1'; -- assert up on 01 - 00 transition
	    dn <= '0';
	    state <= s00;
	  end if;
	  if (a = '1' and b = '1') then
	    state <= s11;
	  end if;
	  if (a = '1' and b = '0') then
	    state <= error;
	  end if;
  
        when s10 =>
	  if (reset = '1') then
	    state <= init;
	  end if;
	  if (a = '0' and b = '0') then
	    up <= '0';
	    dn <= '1'; -- assert dn on 10 - 00 transition
	    state <= s00;
	  end if;
	  if (a = '1' and b = '1') then
	    state <= s11;
	  end if;
	  if (a = '0' and b = '1') then
	    state <= error;
	  end if;

        when s11 =>
	  if (reset = '1') then
	    state <= init;
	  end if;
	  if (a = '0' and b = '1') then
	    state <= s01;
	  end if;
	  if (a = '1' and b = '0') then
	    state <= s10;
	  end if;
	    if (a = '0' and b = '0') then
	    state <= error;
	  end if;
  
        when error =>
	  state <= init; -- trigger on-shot and go back to init
	  trigger <= '1';
  
      end case;
    end process;

	-- Sijmen edit: removed oneshot
	err <= trigger;

end behavior;

