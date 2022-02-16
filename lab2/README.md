# SPOLKS lab 2

## Network protocol

UDP, port 12000

**Common header (CH):**

```
0       8              24      32
| type  |      len      |  rsv  |
```

Field | Type | Description
----- | ---- | -----------
type | uint8 | Message type
len | uint16 | Packet size (with CH)
rsv | | Zero-filled

**Discover request:**

```
CH (type=0)
```

**Discover response:**

```
CH (type=1)
```

**Rooms request:**

```
CH (type=8)
```

**Room announce:**

```
CH (type=9)

0         32     ...
| address  | name |
```

Field | Type | Description
----- | ---- | -----------
address | | Multicast address of this room. First 4 bits **have to** be equal to 1110
name | uchar[] | UTF-8 encoded text string

**Text message:**

```
CH (type=32)

0     ...
| text |
```

Field | Type | Description
----- | ---- | -----------
text | uchar[] | UTF-8 encoded text string
