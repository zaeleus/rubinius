#include "shotgun.h"
#include "tommath.h"
#include "string.h"
#include <ctype.h>
#include "math.h"

#define NMP mp_int *n; OBJECT n_obj; \
  NEW_STRUCT(n_obj, n, BASIC_CLASS(bignum), mp_int); \
  mp_init(n)

#define MP(k) DATA_STRUCT(k, mp_int*)

OBJECT bignum_new(STATE, int num) {
  mp_int *a;
  OBJECT o;
  o = object_memory_new_opaque(state, BASIC_CLASS(bignum), sizeof(mp_int));
  a = (mp_int*)BYTES_OF(o);
  mp_init(a);
  mp_set_int(a, (unsigned int)num);
  if(num < 0) {
    a->sign = MP_NEG;
  }
  return o;
}

OBJECT bignum_new_unsigned(STATE, unsigned int num) {
  mp_int *a;
  OBJECT o;
  o = object_memory_new_opaque(state, BASIC_CLASS(bignum), sizeof(mp_int));
  a = (mp_int*)BYTES_OF(o);
  mp_init(a);
  mp_set_int(a, num);
  return o;
}

OBJECT bignum_normalize(STATE, OBJECT b) {
  if(mp_count_bits(MP(b)) <= (32 - 3)) {
    int val;
    val = (int)mp_get_int(MP(b));
    if(MP(b)->sign == MP_NEG) {
      val = -val;
    }
    return I2N(val);
  }
  return b;
}

OBJECT bignum_add(STATE, OBJECT a, OBJECT b) {
  NMP;
  
  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }
    
  mp_add(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_sub(STATE, OBJECT a, OBJECT b) {
  NMP;
  
  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }
  
  mp_sub(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj); 
}

OBJECT bignum_mul(STATE, OBJECT a, OBJECT b) {
  NMP;
  
  if(FIXNUM_P(b)) {
    if(b == I2N(2)) {
      mp_mul_2(MP(a), n);
      return n_obj;
    }
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }
  
  mp_mul(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_div(STATE, OBJECT a, OBJECT b) {
  NMP;
  
  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }
  
  mp_div(MP(a), MP(b), n, NULL);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_and(STATE, OBJECT a, OBJECT b) {
  NMP;

  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }

  mp_and(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_or(STATE, OBJECT a, OBJECT b) {
  NMP;

  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }

  mp_or(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_xor(STATE, OBJECT a, OBJECT b) {
  NMP;

  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }

  mp_xor(MP(a), MP(b), n);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_invert(STATE, OBJECT self) {
  NMP;

  mp_int a;  mp_init(&a);
  mp_int b;  mp_init_set_int(&b, 1);

  /* inversion by -(a)-1 */
  mp_neg(MP(self), &a);
  mp_sub(&a, &b, n);

  mp_clear(&a); mp_clear(&b);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_neg(STATE, OBJECT self) {
  NMP;

  mp_neg(MP(self), n);
  return bignum_normalize(state, n_obj);
}

/* I don't think these are the right functions
OBJECT bignum_left_shift(STATE, OBJECT self, int width) {
  NMP;

  if(FIXNUM_P(self)) {
    int j;
    j = FIXNUM_TO_INT(self);
    mp_set_int(n, (unsigned int)j);
  } else {
    mp_copy(MP(self), n);
  }
  mp_lshd(n, width);
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_right_shift(STATE, OBJECT self, int width) {
  NMP;

  if(FIXNUM_P(self)) {
    mp_set_int(n, FIXNUM_TO_INT(self));
  } else {
    mp_copy(MP(self), n);
  }

  mp_rshd(n, width);
  return bignum_normalize(state, n_obj);
}
*/

OBJECT bignum_equal(STATE, OBJECT a, OBJECT b) {
  
  if(FIXNUM_P(b)) {
    b = bignum_new(state, FIXNUM_TO_INT(b));
  }
  
  if(mp_cmp(MP(a), MP(b)) == MP_EQ) {
    return Qtrue;
  }
  return Qfalse;
}

unsigned long bignum_to_int(STATE, OBJECT self) {
  return mp_get_int(MP(self));
}

OBJECT bignum_to_s(STATE, OBJECT self, OBJECT radix) {
  char buffer[1024];
  int k;
  mp_toradix_nd(MP(self), buffer, FIXNUM_TO_INT(radix), 1024, &k);
  if(k >= 1022) {
    return Qnil;
  } else {
    return string_new(state, buffer);
  }
}

OBJECT bignum_from_string_detect(STATE, char *str) {
  char *s;
  int radix;
  int sign;
  NMP;
  s = str;
  sign = 1;
  while(isspace(*s)) { s++; }
  if(*s == '+') {
    s++;
  } else if(*s == '-') {
    sign = 0;
    s++;
  }
  radix = 10;
  if(*s == '0') {
    switch(s[1]) {
      case 'x': case 'X':
        radix = 16; s += 2;
        break;
      case 'b': case 'B':
        radix = 2; s += 2;
        break;
      case 'o': case 'O':
        radix = 8; s += 2;
        break;
      case 'd': case 'D':
        radix = 10; s += 2;
        break;
      default:
        radix = 8; s += 1;
    }
  }
  mp_read_radix(n, s, radix);
  
  if(!sign) { 
    n->sign = MP_NEG;
  }
  
  return bignum_normalize(state, n_obj);
}

OBJECT bignum_from_string(STATE, char *str, int radix) {
  NMP;
  mp_read_radix(n, str, radix);
  return bignum_normalize(state, n_obj);
}

void bignum_into_string(STATE, OBJECT self, int radix, char *buf, int sz) {
  int k;
  mp_toradix_nd(MP(self), buf, radix, sz, &k);
}

#define DIGIT_RADIX (1 << DIGIT_BIT)
double bignum_to_double(STATE, OBJECT self) {
  int i;
  double res;
  double m;
  mp_int *a;

  a = MP(self);
  
  if (a->used == 0) {
     return 0;
  }

  /* get number of digits of the lsb we have to read */
  i = a->used;
  m = DIGIT_RADIX;

  /* get most significant digit of result */
  res = DIGIT(a,i);

  while (--i >= 0) {
    res = (res * m) + DIGIT(a,i);
  }

  if(isinf(res)) {
    /* Bignum out of range */
    res = HUGE_VAL;
  }

  if(a->sign == MP_NEG) res = -res;

  return res;
}

OBJECT bignum_from_double(STATE, double d)
{
  NMP;

  long i = 0;
  unsigned int c;
  double value;
  value = (d < 0) ? -d : d;

  /*
  if (isinf(d)) {
    rb_raise(rb_eFloatDomainError, d < 0 ? "-Infinity" : "Infinity");
  }
  if (isnan(d)) {
    rb_raise(rb_eFloatDomainError, "NaN");
  }
  */

  while (!(value <= (LONG_MAX >> 1)) || 0 != (long)value) {
    value = value / (double)(DIGIT_RADIX);
    i++;
  }

  mp_grow(n, i);
 
  while (i--) {
    value *= DIGIT_RADIX;
    c = (unsigned int)value;
    value -= c;
    DIGIT(n,i) = c;
    n->used += 1;
  }

  if (d < 0) {
    mp_neg(n, n);
  }

  return n_obj;
}
