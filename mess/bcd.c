int bcd_adjust(int value)
{
	if ((value&0xf)>=0xa) value=value+0x10-0xa;
	if ((value&0xf0)>=0xa0) value=value-0xa0+0x100;
	return value;
}

int dec_2_bcd(int a)
{
	return (a%10)|((a/10)<<4);
}
