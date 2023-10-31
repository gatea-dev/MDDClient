# MDDClient

Market Data Direct (MD-Direct or MDD) Client API:

Library | Description
--- | ---
libmddWire | Wire Protocol Marshalling / Demarshalling
librtEdge | Provides Access to all the MD-Direct Features
quant | Quant Calcs : Cubic Splines, Surfaces; Greeks

### libmddWire
-# Wire Protocol Marshalling / Demarshalling
-# Used by librtEdge to get to / from the wire
-# Isolates wire representation from the caller (librtEdge) 

### librtEdge
-# Full access to the MD-Direct platform
-# Publish, Subscribe, Snap from LVC, Access Tape
-# Create your own pub/sub apps (e.g., Real-Time P&L, Calculation, etc.)
-# Available on WIN64 in C, C++, .NET
-# Available on Linux64 in C, C++

### quant
-# Numerical Recipes in C 
   - Cubic Splines / Surfaces
   - LU Decomposition
   - FFT
-# Options Book - Hull
   - Greeks
   - Implied Volatility
