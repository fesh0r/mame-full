    int	x/*, i*/, c0, c1, l ;
    int	Incr0, Incr1, Incr2, Incr3, Incr4, Vol0, Vol1, Vol2, Vol3, Vol4 ;
#ifdef SCC_INTERPOLATE
    int	c2 ;
#endif
    DATATYPE *buf;

    buf = (DATATYPE*)buffer;

    x = ((int)SCC[n].Regs[SCC_1FINE] + (((int)SCC[n].Regs[SCC_1COARSE])<<8)) ;
    Incr0 = x ? (int)(SCC[n].UpdateStep / (double)x) : 0 ;

    x = ((int)SCC[n].Regs[SCC_2FINE] + (((int)SCC[n].Regs[SCC_2COARSE])<<8)) ;
    Incr1 = x ? (int)(SCC[n].UpdateStep / (double)x) : 0 ;

    x = ((int)SCC[n].Regs[SCC_3FINE] + (((int)SCC[n].Regs[SCC_3COARSE])<<8)) ;
    Incr2 = x ? (int)(SCC[n].UpdateStep / (double)x) : 0 ;

    x = ((int)SCC[n].Regs[SCC_4FINE] + (((int)SCC[n].Regs[SCC_4COARSE])<<8)) ;
    Incr3 = x ? (int)(SCC[n].UpdateStep / (double)x) : 0 ;

    x = ((int)SCC[n].Regs[SCC_5FINE] + (((int)SCC[n].Regs[SCC_5COARSE])<<8)) ;
    Incr4 = x ? (int)(SCC[n].UpdateStep / (double)x) : 0 ;

    Vol0 = (SCC[n].Regs[SCC_ENABLE] & 0x01) ? SCC[n].Regs[SCC_1VOL] : 0 ;
    Vol1 = (SCC[n].Regs[SCC_ENABLE] & 0x02) ? SCC[n].Regs[SCC_2VOL] : 0 ;
    Vol2 = (SCC[n].Regs[SCC_ENABLE] & 0x04) ? SCC[n].Regs[SCC_3VOL] : 0 ;
    Vol3 = (SCC[n].Regs[SCC_ENABLE] & 0x08) ? SCC[n].Regs[SCC_4VOL] : 0 ;
    Vol4 = (SCC[n].Regs[SCC_ENABLE] & 0x10) ? SCC[n].Regs[SCC_5VOL] : 0 ;

    if ( !(Vol0 || Vol1 || Vol2 || Vol3 || Vol4) ) {
        while (length--) *buf++ = DATACONV (0);
	return ;
    }

    while (length--)
    {
	l = 0;
	if (Vol0 && Incr0) {
	    c0 = (SCC[n].Counter0 + Incr0) & 0xffff ;
	    c1 = (signed char)SCC[n].Waves[0x00 + (c0>>11)] ;
#ifdef SCC_INTERPOLATE
	    c2 = (signed char)SCC[n].Waves[0x00 + ((c0+0x800)>>11)] ;
	    l += (Vol0*(c1+(c2-c1)*(c0&0x7ff)/0x800));
#else
	    l += (Vol0*c1);
#endif
	    SCC[n].Counter0 = c0 ;
	}

	if (Vol1 && Incr1) {
	    c0 = (SCC[n].Counter1 + Incr1) & 0xffff ;
	    c1 = (signed char)SCC[n].Waves[0x20 + (c0>>11)] ;
#ifdef SCC_INTERPOLATE
	    c2 = (signed char)SCC[n].Waves[0x20 + ((c0+0x800)>>11)] ;
	    l += (Vol1*(c1+(c2-c1)*(c0&0x7ff)/0x800));
#else
	    l += (Vol1*c1);
#endif
	    SCC[n].Counter1 = c0 ;
	}

	if (Vol2 && Incr2) {
	    c0 = (SCC[n].Counter2 + Incr2) & 0xffff ;
	    c1 = (signed char)SCC[n].Waves[0x40 + (c0>>11)] ;
#ifdef SCC_INTERPOLATE
	    c2 = (signed char)SCC[n].Waves[0x40 + ((c0+0x800)>>11)] ;
	    l += (Vol2*(c1+(c2-c1)*(c0&0x7ff)/0x800));
#else
	    l += (Vol2*c1);
#endif
	    SCC[n].Counter2 = c0 ;
	}

	if (Vol3 && Incr3) {
	    c0 = (SCC[n].Counter3 + Incr3) & 0xffff ;
	    c1 = (signed char)SCC[n].Waves[0x60 + (c0>>11)] ;
#ifdef SCC_INTERPOLATE
	    c2 = (signed char)SCC[n].Waves[0x60 + ((c0+0x800)>>11)] ;
	    l += (Vol3*(c1+(c2-c1)*(c0&0x7ff)/0x800));
#else
	    l += (Vol3*c1);
#endif
	    SCC[n].Counter3 = c0 ;
	}

	if (Vol4 && Incr4) {
	    c0 = (SCC[n].Counter4 + Incr4) & 0xffff ;
	    c1 = (signed char)SCC[n].Waves[0x80 + (c0>>11)] ;
#ifdef SCC_INTERPOLATE
	    c2 = (signed char)SCC[n].Waves[0x80 + ((c0+0x800)>>11)] ;
	    l += (Vol4*(c1+(c2-c1)*(c0&0x7ff)/0x800));
#else
	    l += (Vol4*c1);
#endif
	    SCC[n].Counter4 = c0 ;
	}
	*buf++ = DATACONV (l);
    }

