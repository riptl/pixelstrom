# Pixelflut for Solana

## Rules

- Free-for-all
- Canvas consisting of 32x32 chunk (2048 bytes)
- RGB565 bitmap
- Each transaction sets one pixel

## Program

The `pixelstrom` program maintains the virtual canvas.

### Instruction: SetPixel

Sets a single pixel in the canvas.

Will only execute when it is the only instruction in a transaction.

Account inputs:
1. Chunk account (writable)
2. Instructions sysvar

Instruction data (big-endian)
1. `x: i32`
2. `y: i32`
3. `rgb565: u16`

### Account: Chunk

Chunk accounts are PDAs.

The PDA seeds list looks like this:
- ASCII string `Chunk`
- Big-endian 0x43484e4b and the big-endian i32 chunk coordinates (y, x).

```python
def chunk_seed(y, x):
    return [b'Chunk', struct.pack(">i", y), struct.pack(">i", x)]
```

```
>>> list([b.hex() for b in chunk_seed(-4, 3)])
['4368756e6b', 'fffffffc', '00000003']
```
