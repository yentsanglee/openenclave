#ifdef weak_alias
#undef weak_alias
#endif

#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((weak, alias(#old)))
