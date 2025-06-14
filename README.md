### Bluetooth LE Passive Advertising monitor
Based on hcitool from BlueZ - Bluetooth protocol stack for Linux
---

### Usage:

```
le-scan-passive -i hci0
```
If errors occur as "Set scan parameters failed: Input/output error", try:
```
systemctl restart bluetooth.service
```

### Build:

```
make
```
### Dependencies:

A few files from the BlueZ Bluetooth package are needed:
* bluetooth.c
* bluetooth.h
* hci.c
* hci.h
* hci_lib.h

### Output:

| MAC          | RSSI | Payload with \<human readable type fields>       |
| :----------: | ---- |---------------------------------------------- |
| A4:C1:38:90:34:F2 |  -70 | \<FLAGS>06<SERVICE_DATA_UUID16>D2FC40002E0100028309033719 |




