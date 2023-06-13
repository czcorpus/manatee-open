// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#ifndef BITIO_HH
#define BITIO_HH

#include <config.hh>
#include <stdint.h>
#include <assert.h>

#define SHL(V,N) ((((long long) (sizeof(V) * 8)) <= (N)) ? (((void)(V)), 0) : ((V) << (N)))
#define SHR(V,N) ((((long long) (sizeof(V) * 8)) <= (N)) ? (((void)(V)), 0) : ((V) >> (N)))
#define ULL_BITLENGTH (8 * sizeof (unsigned long long))

template <class ValueType>
inline int_fast32_t ceilog_2 (ValueType x)
{
#ifdef DEBUG
    assert (x);
#endif
#ifdef HAVE___BUILTIN_CLZLL
    return (x > 1) ? (ULL_BITLENGTH - __builtin_clzll (x - 1)) : 0;
#else // assumes x > 0
    int_fast32_t v = 0;
    for (x--; x; v++)
        x >>= 1;
    return v;
#endif
}

template <class ValueType>
inline int_fast32_t floorlog_2 (ValueType x)
{
#ifdef DEBUG
    assert (x);
#endif
#ifdef HAVE___BUILTIN_CLZLL
    return x ? (ULL_BITLENGTH - 1 - __builtin_clzll (x)) : -1;
#else
    int_fast32_t v = -1;
    for (; x; v++)
        x >>= 1;
    return v;
#endif
}

/**
 * write_bits
 * AtomType and ValueType should be unsigned types
 */

template <class Iterator, class AtomType = unsigned char, 
          class LoadIterator = Iterator, class ValueType = unsigned int>
class write_bits
{
protected:
    Iterator mem;
    static const int_fast8_t atomsize = sizeof (AtomType) * 8;
    static const AtomType atommask = AtomType (~0ULL);
    
    int_fast8_t bits;        /* number of free bits in *mem */

    void next_atom ()
        {
            if (bits == 0) {
                *(++mem) = 0;
                bits = atomsize;
            }
        }
public:
    write_bits (Iterator memory) : mem (memory), bits (atomsize) {*mem = 0;}
    ~write_bits () {if (bits < atomsize) ++mem;}

    /**
     * bitslen() returns number of bits occupied by data in the current
     * atom
     */
    int bitslen () const {return atomsize - bits;}

    void load (LoadIterator src, int_fast32_t srcbits)
        {
            if (bits == 0) {
                bits = srcbits % atomsize;
                for (; srcbits > 0; srcbits -= atomsize, ++mem, ++src)
                    *mem = *src;
            } else {
                while (srcbits > bits) {
                    *mem |= (((*src) << (atomsize - bits)) & atommask);
                    *(++mem) = SHR ((*src), bits);
                    srcbits -= atomsize;
                    ++src;
                }
                if (srcbits > 0) {
                    *mem |= (((*src) << (atomsize - bits)) & atommask);
                    bits -= srcbits;
                }
            }
        }
    void set_bit (bool x)
        {
            next_atom();
            bits--;
            if (x)
                *mem |= AtomType(1) << (atomsize - bits -1);
        }
    void unary (ValueType val)
        {
            next_atom();
            if (val > (ValueType)bits) {
                val -= bits;
                *(++mem) = 0;
                while (val > (ValueType)atomsize) {
                    val -= atomsize;
                    *(++mem) = 0;
                }
                bits = atomsize;
            }
            bits -= val;
            *mem |= AtomType(1) << (atomsize - bits -1);
        }
    void binary_fix (ValueType x, int_fast32_t len)
        {
            next_atom();
            *mem |= ((x << (atomsize - bits)) & atommask);
            len -= bits;
            x = SHR (x, bits);
            while (len > 0) {
                len -= atomsize;
                *(++mem) = x & atommask;
                x = SHR (x, atomsize);
            }
            bits = -len;
        }
    void binary (ValueType x, ValueType b)
        {
            int_fast32_t len = ceilog_2 (b);
            ValueType thresh = (SHL(ValueType(1), len)) - b;
            len--;
            if (--x < thresh)
                binary_fix (x, len);
            else {
                x += thresh;
                binary_fix (x >> 1, len);
                set_bit (x & 1);
            }
        }
    void gamma (ValueType x)
        {
#ifdef DEBUG
    assert (x);
#endif
            int_fast32_t len = floorlog_2 (x);
            unary (len +1);
            binary_fix (x ^ (SHL(ValueType(1), len)), len);
        }
    void delta (ValueType x)
        {
#ifdef DEBUG
    assert (x);
#endif
            int_fast32_t len = floorlog_2 (x);
            gamma (len +1);
            binary_fix (x ^ (SHL(ValueType(1), len)), len);
        }
    void pdelta (ValueType x, int_fast32_t minlen)
        {
            int_fast32_t len = floorlog_2 (--x);
            if (len < minlen) {
                delta (1); // XXX here should probably be gamma?
                binary_fix (x, minlen);
            } else {
                delta (len - minlen +2); // XXX here should probably be gamma?
                binary_fix (x ^ (SHL(ValueType(1), len)), len);
            }
        }
    void golomb (ValueType x, ValueType b)
        {
            x--;
            ValueType xdiv = x / b;
            x = x % b;
            unary (xdiv +1);
            binary (x +1, b);
        }
    void golomb_log (ValueType x, ValueType b)
        {
            x--;
            ValueType xdiv = x / b;
            x = x % b;
            gamma (xdiv +1);
            binary (x +1, b);
        }
    void new_block ()
        {
            if (bits < atomsize) {
                *(++mem) = 0;
                bits = atomsize;
            }
        }
            
};



template <class Iterator, class AtomType = unsigned char,
          class ValueType = unsigned int>
class read_bits
{
protected:
    Iterator mem;
    int_fast32_t bits;  /* number of unread bits in curr */
    AtomType curr;

    static const int_fast32_t atomsize = sizeof (AtomType) * 8;
    static const AtomType atommask = AtomType (~0ULL);

    void next_atom ()
        {
            if (bits == 0) {
                curr = *(++mem);
                bits = atomsize;
            }
        }
public:
    read_bits (Iterator memory, int_fast32_t skipbits=0)
        : mem (memory), bits (atomsize)
        {
            while (skipbits >= atomsize) {
                skipbits -= atomsize;
                ++mem;
            }
            curr = *mem;
            if (skipbits > 0) {
                bits -= skipbits;
                curr >>= skipbits;
            }
        }
    bool operator== (const read_bits<Iterator,AtomType> &x) const
        {
            return mem == x.mem && bits == x.bits;
        }
    /**
     * bitslen() returns number of bits read so far from the current atom
     */
    int bitslen () const {return atomsize - bits;}
    bool get_bit ()
        {
            next_atom();
            bool x = curr & 1;
            curr >>= 1;
            bits--;
            return x;
        }
    ValueType unary ()
        {
            ValueType x = 1;
            next_atom();
            if (! curr) {
                for (x += bits, ++mem; !(curr = *mem); ++mem)
                    x += atomsize;
                bits = atomsize;
            }
            // now assuming curr != 0
#ifdef HAVE___BUILTIN_CTZLL
            int_fast32_t trail_zeros = __builtin_ctzll (curr);
            bits -= trail_zeros + 1;
            curr >>= trail_zeros + 1;
            return x + trail_zeros;
#else
            while (! (curr & 1)) {
                bits--; x++;
                curr >>= 1;
            }
            bits--;
            curr >>= 1;
            return x;
#endif
        }
    ValueType binary_fix (int_fast32_t len)
        {
            if (len == 0)
                return 0;
            ValueType x = 0;
            int_fast32_t shift = 0;
            next_atom();
            if (len > bits) {
                x = curr;
                ++mem;
                len -= bits;
                shift = bits;
                while (len > atomsize) {
                    len -= atomsize;
                    x |= SHL(ValueType(*mem), shift);
                    ++mem;
                    shift += atomsize;
                }
                bits = atomsize;
                curr = *mem;
            }
            x |= SHL(ValueType(curr & SHR(atommask,(atomsize - len))), shift);
            curr = SHR(curr, len);
            bits -= len;
            return x;
        }
    ValueType binary (ValueType b)
        {
            ValueType x = 0;
            if (b != 1) {
                int_fast32_t len = ceilog_2 (b);
                ValueType thresh = (SHL(ValueType(1), len)) - b;
                x = binary_fix (len -1);
                if (x >= thresh) {
                    x *= 2;
                    x += get_bit();
                    x -= thresh;
                }
                return x +1;
            } else
                return 1;
        }
    ValueType gamma ()
        {
            int_fast32_t len = unary() -1;
            return binary_fix (len) ^ (SHL(ValueType(1), len));
        }
    ValueType delta ()
        {
            int_fast32_t len = gamma() -1;
            return binary_fix (len) ^ (SHL(ValueType(1), len));
        }
    ValueType pdelta (int_fast32_t minlen)
        {
            int_fast32_t len = gamma();
            if (len == 1)
                return binary_fix (minlen);
            else
                return binary_fix (len + minlen -2) ^ (SHL(ValueType(1), len));
        }
    ValueType golomb (ValueType b)
        {
            ValueType xdiv = unary() -1;
            return binary (b) + b * xdiv;
        }
    void new_block ()
        {
            if (bits < atomsize) {
                curr = *(++mem);
                bits = atomsize;
            }
        }
};


#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
