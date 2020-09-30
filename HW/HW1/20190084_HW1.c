unsigned rotate_left(unsigned x, int n) {
    unsigned w = sizeof(unsigned) << 3;
    return (x << n) | (x >> (w-n-1) >> 1);
}

float_bits float_half(float_bits f) {
    unsigned sign = f & 0x80000000;
    unsigned expo = f & 0x7f800000;
    unsigned frac = f & 0x007fffff;

    if(expo == 0x7f800000){
        return f;
    }

    if(expo > 0x00800000){
        return f - 0x00800000;
    }

    return sign | (((expo | frac) >> 1) + ((f & 3) == 3));

}