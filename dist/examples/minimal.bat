@echo off
:: Licensed under conditions stated in
:: the documentation you recieved with this software

:: set discordappid=1035630279416610856
set discordstate=Hello world
set discorddetails=Test
set discordlargeimg=canary-large
set discordlargeimgtxt=Test1
set discordsmallimg=ptb-small
set discordsmallimgtxt=Test2
for %%x in (getinput discord) do  ..\batch_native.exe ..\%%x.dll
:a
title %keypressed% %click% %mousexpos% %mouseypos% %wheeldelta%
goto :a