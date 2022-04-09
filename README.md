# i2c
C++ I²C Bus

Models I²C bus controllers and targets in software.

This project supports a useful sub-set of the full I²C bus specification https://www.nxp.com/docs/en/user-guide/UM10204.pdf.
Notably, there may be only one controller on the bus, and 7-bit addressing is used.

## Node

Models a node connected to a I²C bus.

### ControllerBase

Models an I²C controller connected to a I²C bus.

### TargetBase

Models an I²C target at an address on the I²C bus.
