Application starts BLE tramsmitter test mode with specific frequncy, payload pattern and payload length.

BLE Packet structute:


P AAAA PDU CCC

P   - preamble, 0xAA
A   - Access address, for le_test is 0x71764129
PDU - packet data unit, variable length
CCC - CRC, 3 bytes

