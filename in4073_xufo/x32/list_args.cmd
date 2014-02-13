@echo off
:LOOP
IF "%1"=="" GOTO DONE
ECHO vhdl work ../%1
SHIFT
GOTO LOOP
:DONE
