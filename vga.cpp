#include "vga.h"
#include "asm.h"
#include "stdlib.h"

namespace Kernel
{

namespace Core
{

VgaTerm::VgaTerm()
    : Buf(reinterpret_cast<u16*>(BufAddr))
    , Row(0)
    , Column(0)
    , Width(80)
    , Height(25)
    , ColorCode(MakeColor(ColorWhite, ColorBlack))
{
	Shared::AutoLock lock(Lock);

	ClsLockHeld();
    Cursor();
}

VgaTerm::~VgaTerm()
{
}

u8 VgaTerm::MakeColor(Color fg, Color bg)
{
    return fg | bg << 4;
}

u16 VgaTerm::MakeEntry(char c, u8 color)
{
	return (u16)c | (u16)color << 8;
}

void VgaTerm::SetColor(Color fg, Color bg)
{
	Shared::AutoLock lock(Lock);

    ColorCode = MakeColor(fg, bg);
}

void VgaTerm::PutCharAt(char c, u8 color, u8 x, u8 y)
{
	const size_t index = y * Width + x;

	Buf[index] = MakeEntry(c, color);
}

void VgaTerm::Overflow()
{
	if (Column == Width)
    {
		Column = 0;
		Row++;
	}

	if (Row == Height)
		Row = 0;
}

void VgaTerm::PutChar(char c)
{
	if (c == '\n')
	{
		while (Column < Width)
				PutCharAt(' ', ColorCode, Column++, Row);
		Column = 0;
		Row++;
	}
	else
	{
	    PutCharAt(c, ColorCode, Column, Row);
	    Column++;
    }
	Overflow();
	Cursor();
}

void VgaTerm::PutsLockHeld(const char *str)
{
	for (;;)
	{
		char c = *str++;
		if (c == '\0')
			break;

		PutChar(c);
	}
}

void VgaTerm::Puts(const char *str)
{
	Shared::AutoLock lock(Lock);

	PutsLockHeld(str);
}

void VgaTerm::ClsLockHeld()
{
	for (u8 x = 0; x < Width; x++)
		for (u8 y = 0; y < Height; y++)
			PutCharAt('\0', MakeColor(ColorBlack, ColorBlack), x, y);
}

void VgaTerm::Cls()
{
	Shared::AutoLock lock(Lock);

	ClsLockHeld();
	Row = 0;
	Column = 0;
	Cursor();
}

void VgaTerm::Vprintf(const char *fmt, va_list args)
{
	Shared::AutoLock lock(Lock);

	char str[256];

	if (Shared::VsnPrintf(str, sizeof(str), fmt, args) < 0)
		return;

	PutsLockHeld(str);
}

void VgaTerm::Printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	Vprintf(fmt, args);
	va_end(args);
}

void VgaTerm::Cursor()
{
	u16 offset = ((Row % Height) * Width + (Column % Width)) % (Width * Height);
	Outb(VgaBase, VgaIndex + 1);
	Outb(VgaBase + 1, offset & 0xFF);
	Outb(VgaBase, VgaIndex);
	Outb(VgaBase + 1, offset >> 8);
}

}
}
