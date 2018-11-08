#ifndef _COMPLEX_H
#define _COMPLEX_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER
//
// Visual Studio Doesn't support the attribute syntax for _Complex. But it can support the data type. So we typedef 
// to a neutral type and everything looks the same.

#pragma message("Visual C")

#ifndef _C_COMPLEX_T
    #define _C_COMPLEX_T
    typedef struct _C_double_complex
    {
        double _Val[2];
    } _C_double_complex;

    typedef struct _C_float_complex
    {
        float _Val[2];
    } _C_float_complex;

    typedef struct _C_ldouble_complex
    {
        long double _Val[2];
    } _C_ldouble_complex;
#endif

typedef _C_double_complex  _Dcomplex;
typedef _C_float_complex   _Fcomplex;
typedef _C_ldouble_complex _Lcomplex;


#else

typedef double _Complex  _Dcomplex;
typedef float _Complex   _Fcomplex;
typedef long double _Complex   _Lcomplex;

#endif

#define complex _Complex

#ifdef __GNUC__
#define _Complex_I (__extension__ (0.0f+1.0fi))
#else
#define _Complex_I (0.0f+1.0fi)
#endif
#define I _Complex_I


_Dcomplex cacos(_Dcomplex);
_Fcomplex cacosf(_Fcomplex);
_Lcomplex cacosl(_Lcomplex);

_Dcomplex casin(_Dcomplex);
_Fcomplex casinf(_Fcomplex);
_Lcomplex casinl(_Lcomplex);

_Dcomplex catan(_Dcomplex);
_Fcomplex catanf(_Fcomplex);
_Lcomplex catanl(_Lcomplex);

_Dcomplex ccos(_Dcomplex);
_Fcomplex ccosf(_Fcomplex);
_Lcomplex ccosl(_Lcomplex);

_Dcomplex csin(_Dcomplex);
_Fcomplex csinf(_Fcomplex);
_Lcomplex csinl(_Lcomplex);

_Dcomplex ctan(_Dcomplex);
_Fcomplex ctanf(_Fcomplex);
_Lcomplex ctanl(_Lcomplex);

_Dcomplex cacosh(_Dcomplex);
_Fcomplex cacoshf(_Fcomplex);
_Lcomplex cacoshl(_Lcomplex);

_Dcomplex casinh(_Dcomplex);
_Fcomplex casinhf(_Fcomplex);
_Lcomplex casinhl(_Lcomplex);

_Dcomplex catanh(_Dcomplex);
_Fcomplex catanhf(_Fcomplex);
_Lcomplex catanhl(_Lcomplex);

_Dcomplex ccosh(_Dcomplex);
_Fcomplex ccoshf(_Fcomplex);
_Lcomplex ccoshl(_Lcomplex);

_Dcomplex csinh(_Dcomplex);
_Fcomplex csinhf(_Fcomplex);
_Lcomplex csinhl(_Lcomplex);

_Dcomplex ctanh(_Dcomplex);
_Fcomplex ctanhf(_Fcomplex);
_Lcomplex ctanhl(_Lcomplex);

_Dcomplex cexp(_Dcomplex);
_Fcomplex cexpf(_Fcomplex);
_Lcomplex cexpl(_Lcomplex);

_Dcomplex clog(_Dcomplex);
_Fcomplex clogf(_Fcomplex);
_Lcomplex clogl(_Lcomplex);

double cabs(_Dcomplex);
float cabsf(_Fcomplex);
long double cabsl(_Lcomplex);

_Dcomplex cpow(_Dcomplex, _Dcomplex);
_Fcomplex cpowf(_Fcomplex, _Fcomplex);
_Lcomplex cpowl(_Lcomplex, _Lcomplex);

_Dcomplex csqrt(_Dcomplex);
_Fcomplex csqrtf(_Fcomplex);
_Lcomplex csqrtl(_Lcomplex);

double carg(_Dcomplex);
float cargf(_Fcomplex);
long double cargl(_Lcomplex);

double cimag(_Dcomplex);
float cimagf(_Fcomplex);
long double cimagl(_Lcomplex);

_Dcomplex conj(_Dcomplex);
_Fcomplex conjf(_Fcomplex);
_Lcomplex conjl(_Lcomplex);

_Dcomplex cproj(_Dcomplex);
_Fcomplex cprojf(_Fcomplex);
_Lcomplex cprojl(_Lcomplex);

double creal(_Dcomplex);
float crealf(_Fcomplex);
long double creall(_Lcomplex);

#ifndef __cplusplus
#define __CIMAG(x, t) \
	(+(union { _Complex t __z; t __xy[2]; }){(_Complex t)(x)}.__xy[1])

#define creal(x) ((double)(x))
#define crealf(x) ((float)(x))
#define creall(x) ((long double)(x))

#define cimag(x) __CIMAG(x, double)
#define cimagf(x) __CIMAG(x, float)
#define cimagl(x) __CIMAG(x, long double)
#endif

#if __STDC_VERSION__ >= 201112L
#if defined(_Imaginary_I)
#define __CMPLX(x, y, t) ((t)(x) + _Imaginary_I*(t)(y))
#elif defined(__clang__)
#define __CMPLX(x, y, t) (+(_Complex t){ (t)(x), (t)(y) })
#else
#define __CMPLX(x, y, t) (__builtin_complex((t)(x), (t)(y)))
#endif
#define CMPLX(x, y) __CMPLX(x, y, double)
#define CMPLXF(x, y) __CMPLX(x, y, float)
#define CMPLXL(x, y) __CMPLX(x, y, long double)
#endif

#ifdef __cplusplus
}
#endif
#endif
