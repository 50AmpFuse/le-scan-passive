### Bluetooth LE Passive Advertising monitor
Based on hcitool from BlueZ - Bluetooth protocol stack for Linux
---

### Usage:

```
le-scan-passive -i hci0
```

### Build:

```
make
```

### Output:

| MAC          | RSSI | Payload with \<human readable type fields>       |
| :----------: | ---- |---------------------------------------------- |
| A4:C1:38:90:34:F2 |  -70 | \<FLAGS>06<SERVICE_DATA_UUID16>D2FC40002E0100028309033719 |




