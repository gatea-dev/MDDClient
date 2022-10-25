
echo off

cls

PubCurve.exe ^
   -hp localhost:9985 -sp twap -tp SwapCurve ^
   -hs localhost:9998 -ss velocity -u SwapCurve ^
   -ts RATES.SWAP.USD.PAR.3M:3 ^
   -ts RATES.SWAP.USD.PAR.6M:6 ^
   -ts RATES.SWAP.USD.PAR.1Y:12 ^
   -ts RATES.SWAP.USD.PAR.2Y:24 ^
   -ts RATES.SWAP.USD.PAR.3Y:36 ^
   -ts RATES.SWAP.USD.PAR.5Y:60 ^
   -ts RATES.SWAP.USD.PAR.7Y:84 ^
   -ts RATES.SWAP.USD.PAR.10Y:120 ^
   -ts RATES.SWAP.USD.PAR.20Y:240 ^
   -ts RATES.SWAP.USD.PAR.30Y:360
